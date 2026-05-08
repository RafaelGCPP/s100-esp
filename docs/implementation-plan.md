# Implementation Plan: S-100 Z80 System — CP/M 2.2 Boot

## Decisions

- **Platforms**: Symmetric builds — `linux/CMakeLists.txt` (standard CMake) and `esp32/CMakeLists.txt` (IDF project). Shared code in `components/` using a dual `if(ESP_PLATFORM)` CMakeLists pattern.
- **Monitor ROM**: Existing public-domain Z80 monitor (e.g., SCM by Stephen C Cousins, or RC2014 monitor). Must use polled 8250 UART I/O.
- **8250 UART base port**: `00h` — DATA=00h, IER=01h, IIR=02h, LCR=03h, MCR=04h, LSR=05h, MSR=06h, SCR=07h.
- **Disk format**: Parametrized `DiskGeometry` struct. Default = **Kaypro II 5.25" DSDD** (2 sides, 40 tracks, 10 sectors/track, 512 B/sector, 400 KB). Single-sided variant is also supported. WD1793 sector-size field and DRQ transfer count are derived from the geometry at runtime.
- **CP/M BIOS**: Custom minimal BIOS written in Z80 assembly targeting our WD1793 + 8250.
- **Timing**: Phases 0–3 run at full speed (no throttle). 4 MHz throttle is deferred until after CP/M boots if needed.

---

## Directory Structure

```
s100-esp/
├── linux/
│   ├── CMakeLists.txt       # Standard CMake — FetchContent for floooh/chips
│   └── main.cpp
├── esp32/
│   ├── CMakeLists.txt       # IDF project; EXTRA_COMPONENT_DIRS=../components
│   └── main/
│       ├── CMakeLists.txt
│       └── main.cpp
├── components/              # Shared — dual IDF/non-IDF CMakeLists pattern
│   ├── cpu/                 # Z80Cpu wrapper around z80.h
│   ├── memory/              # MemoryMap (flat → banked)
│   ├── disk/                # WD1793 emulator + DiskGeometry + DiskImage hierarchy
│   ├── uart/                # UART8250 emulator + platform drivers
│   └── machine/             # Machine orchestrator + I/O bus dispatch
├── roms/
│   ├── monitor/             # Z80 monitor source + binary blob embedded as C header
│   └── boot/                # Boot ROM Z80 assembly + assembled binary
├── experiments/             # Proof-of-concept explorations
└── docs/
```

---

## Phase 0: Build System & Scaffolding

1. Create `linux/CMakeLists.txt` with `FetchContent` pulling `floooh/chips` (header-only; provides `chips/z80.h`).
2. Create `esp32/CMakeLists.txt` as an IDF project with `EXTRA_COMPONENT_DIRS` pointing to `../components`.
3. Create a stub `CMakeLists.txt` in each `components/` subdirectory using the `if(ESP_PLATFORM)` dual-build guard.
4. Create `experiments/z80_hello/` — minimal smoke-test: init `z80_t`, tick 100 NOP cycles, print final PC. Validates the entire toolchain.

**Verification**: `cmake -B build -S linux && make` produces a running binary that prints a PC value.

---

## Phase 1: CPU Core + Flat Memory + 8250 UART + Monitor ROM

Steps 1–4 are independent and can be developed in parallel. Steps 5–6 depend on all four.

1. **`components/cpu/`** — `Z80Cpu` class:
   - Holds a `z80_t` instance.
   - `tick()`: calls `z80_tick(pins)` and decodes MREQ+RD/WR and IORQ+RD/WR pin masks.
   - `reset()`, `getPC()`, and other register accessors.

2. **`components/memory/`** — `MemoryMap` class:
   - Flat `uint8_t[65536]` internal buffer.
   - `read(addr)`, `write(addr, val)`.
   - ROM-protection flag per 1 KB page; writes to protected pages are silently dropped.
   - `loadROM(data, base, size)` — copies binary blob and marks pages read-only.

3. **`components/uart/`** — `UART8250` class:
   - Polled registers: THR/RBR, IER, IIR, LCR, MCR, LSR, MSR at offsets 0–6 from base port.
   - `ioRead(reg)`, `ioWrite(reg, val)`.
   - `Platform` abstract interface: `putChar(c)`, `getChar()`, `hasChar()`.
   - `LinuxPlatform`: raw-mode stdin/stdout.
   - `ESP32Platform`: ESP-IDF UART driver stub (filled in later).

4. **`components/machine/`** — `Machine` class:
   - `IOBus` dispatch table mapping port ranges to device handlers.
   - `step()` calls `cpu.tick()` and routes resulting memory/IO pin state to `MemoryMap` or `IOBus`.
   - `run()` loop.

5. **`roms/monitor/`** — integrate a suitable public-domain Z80 monitor (SCM or Grant Searle's design) as a pre-assembled binary embedded in a C header (`const uint8_t monitor_rom[]`). Loaded at address `0x0000`.

6. **`linux/main.cpp`** — instantiate `Machine`, load monitor ROM, call `run()`.

**Verification**: Linux build compiles cleanly; monitor prompt appears over stdin/stdout; hex-examine and jump commands work.

---

## Phase 2: 16 KB Memory Banking

Depends on Phase 1.

1. Extend `MemoryMap`:
   - Add `uint8_t slot_bank[4]` — one bank index per 16 KB slot.
   - Physical storage: `uint8_t physical[256][16384]` allocated on the Linux heap; on ESP32, allocated from PSRAM via `heap_caps_malloc(MALLOC_CAP_SPIRAM)`.
2. **Active Window**: the internal 64 KB buffer remains the CPU's working memory. `mapSlot(slot, bank)` performs a `memcpy` to flush the current slot content to its physical bank and loads the new bank into the buffer.
3. I/O ports `90h`–`93h` routed through `Machine::IOBus` to `MemoryMap::mapSlot()`.
4. Power-on defaults: slot 0 → Bank 255 (Boot ROM / monitor); slots 1–3 → Bank 0.
5. Load monitor binary into physical Bank 255, offset `0x0000`–`0x3FFF`.
6. Update `linux/main.cpp` and ESP32 main to initialise the new memory model.

**Verification**: Z80 code writes ports `90h`–`93h`, different memory banks become visible at the same logical addresses; monitor continues to function normally.

---

## Phase 3: WD1793 Disk I/O (Fixed In-Memory Data)

Depends on Phase 2.

1. **`DiskGeometry` struct** (`components/disk/include/disk_geometry.hpp`):
   - Fields: `uint8_t tracks`, `uint8_t sectors_per_track`, `uint16_t sector_size`, `uint8_t sides`.
   - Predefined constant `KAYPRO2_DSDD = {40, 10, 512, 2}` — Kaypro II 5.25" double-sided.
   - Single-sided variant `KAYPRO2_SSDD = {40, 10, 512, 1}`.
   - Sector byte offset:
     $$\text{offset} = \bigl(track \times sides \times spt + side \times spt + sector\_index\bigr) \times sector\_size$$

2. **`WD1793`** state machine (`components/disk/`):
   - Registers: Status/Command (`10h`), Track (`11h`), Sector (`12h`), Data (`13h`).
   - Type I commands: Restore (`00h`), Seek (`10h`), Step (`20h`/`30h`), Step-In (`40h`/`50h`), Step-Out (`60h`/`70h`).
   - Type II commands: Read Sector (`80h`/`88h`), Write Sector (`A0h`/`A8h`) — polled DRQ; transfer `geometry.sector_size` bytes (512 B for Kaypro II) per operation.
   - Type IV: Force Interrupt (`D0h`).
   - Status register bits: BUSY, DRQ, SEEK_ERROR, CRC_ERROR, NOT_READY, WRITE_PROTECT.
   - Sector-size encoding (command bits 0–1: `00`=128 B, `01`=256 B, `10`=512 B, `11`=1024 B) matched automatically to the attached `DiskGeometry`.

3. **`DiskImage`** abstract interface (`components/disk/include/disk_image.hpp`):
   - `readSector(track, side, sector, buf)`
   - `writeSector(track, side, sector, buf)`
   - `getGeometry() → DiskGeometry`

4. **`MemoryDiskImage`**: `KAYPRO2_DSDD` geometry; sector 1 of track 0, side 0 pre-filled with known test bytes.

5. Drive-select port `14h`: bits 0–1 = drive number (0–3), bit 2 = side (0 or 1). `WD1793` tracks the current drive and side internally. Holds `DiskImage* drives[4]`.

6. Dispatch ports `10h`–`14h` in `Machine::IOBus`.

**Verification**: Z80 code sequence — Restore → Seek track 0 → Read Sector 1 side 0 → poll DRQ → read 512 bytes — produces data matching the fixed `MemoryDiskImage` bytes.

---

## Phase 4: Real Disk Images + Boot ROM + CP/M 2.2

Depends on Phase 3.

1. **`FileDiskImage`** (Linux): opens a `.dsk` file; `readSector`/`writeSector` seek to the parametrized offset:
   `(track * geom.sides * geom.sectors_per_track + side * geom.sectors_per_track + sector_index) * geom.sector_size`

2. **`SDCardDiskImage`** (ESP32): same `DiskImage` interface backed by ESP-IDF FATFS over SD card.

3. **`roms/boot/boot.asm`** — Boot ROM assembled into physical Bank 255:
   ```asm
   DI
   LD   A, 1
   OUT  (91H), A        ; Slot 1 → Bank 1
   LD   A, 2
   OUT  (92H), A        ; Slot 2 → Bank 2
   LD   A, 254
   OUT  (93H), A        ; Slot 3 → Bank 254 (stack)
   LD   SP, 0FFFFh      ; Stack at top of Slot 3
   ; Write bank-switch snippet at C000h
   LD   HL, 0C000h
   LD   (HL), 0AFh      ; XOR A
   INC  HL
   LD   (HL), 0D3h      ; OUT (90H), A
   INC  HL
   LD   (HL), 090h
   ; Load Track 0, Sector 1 from Drive 0 → C003h via WD1793
   ; ... (WD1793 command sequence)
   JP   0C000h
   ```

4. **CP/M 2.2 disk image** for Kaypro II DSDD geometry:
   - Source: public-domain CCP+BDOS binary (Digital Research / Unofficial CP/M Web Site).
   - Custom `roms/bios.asm`: BIOS jump table; CONIN/CONOUT/CONST → 8250; SELDSK/SETTRK/SETSEC/SETDMA/READ/WRITE → WD1793.
   - Logical tracks = 80 (side 0: tracks 0–39, side 1: tracks 40–79); BIOS translates `cylinder = track % 40`, `side = track / 40`.
   - **Disk Parameter Block** for DSDD 512 B/sector, 2 × 40 tracks, 2 system tracks:

     | DPB Field | Value | Notes |
     |---|---|---|
     | SPT | 40 | 128-byte logical records per track (10 × 512 / 128) |
     | BSH | 4 | 2 KB block shift |
     | BLM | 15 | 2 KB block mask |
     | EXM | 1 | DSM < 256 and 2 KB blocks → (2048/1024) − 1 |
     | DSM | 194 | 78 data tracks × 40 records / 16 records per block − 1 |
     | DRM | 63 | 64 directory entries |
     | AL0 | 0x80 | 1 directory allocation block of 2 KB |
     | AL1 | 0x00 | |
     | CKS | 16 | (DRM+1)/4 — removable media |
     | OFF | 2 | 2 reserved system tracks |

   - Assemble and pack into a `.dsk` image using `cpmtools` or a Python script.

5. Wire `FileDiskImage` into `WD1793` in `Machine`; load Boot ROM into Bank 255.

**Verification**: `A>` prompt appears on the Linux terminal. `DIR`, `TYPE`, `ERA`, and `STAT` commands run correctly.

---

## Phase 5: ESP32 Port

Depends on Phase 4. No changes to any file in `components/` are required; this phase only adds concrete implementations of the three abstract interfaces and wires up the IDF build.

1. **`esp32/CMakeLists.txt`** — IDF project root with `EXTRA_COMPONENT_DIRS = ../components`.
   **`esp32/main/CMakeLists.txt`** — registers the main component.

2. **`ESP32Console`** (`esp32/main/esp32_platform_factory.hpp`):
   - Implements `IConsole` using `uart_write_bytes()` / `uart_read_bytes()` from ESP-IDF `driver/uart.h`.
   - Constructor configures the hardware UART (baud rate, pins) via `uart_param_config()`.

3. **`ESP32Allocator`**:
   - Implements `IAllocator` using `heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM)` and `heap_caps_free()`.
   - Provides the 256 × 16 KB = 4 MB physical bank storage in PSRAM.

4. **`SDCardDiskImage`**:
   - Implements `DiskImage` using ESP-IDF FATFS (`esp_vfs_fat_sdmmc_mount`, `fopen`, `fseek`, `fread`, `fwrite`).
   - Image files located at e.g. `/sdcard/drive_a.dsk`. Path and geometry passed at construction.

5. **`ESP32PlatformFactory`**:
   - `createConsole()` → `ESP32Console`.
   - `createAllocator()` → `ESP32Allocator`.
   - `createDiskImage(drive)` → `SDCardDiskImage` with per-drive path.

6. **`esp32/main/main.cpp`** — `app_main()`:
   - Creates `ESP32PlatformFactory`.
   - Constructs `Machine` with the factory.
   - Calls `machine.run()`.

**Verification**: ESP32 firmware builds cleanly under IDF. After flashing, the system boots CP/M 2.2 and an `A>` prompt appears on the configured UART. `DIR` and basic CP/M commands work correctly.

---

## Relevant Files

| Path | Role |
|---|---|
| `linux/CMakeLists.txt` | Linux standalone build |
| `linux/linux_platform_factory.hpp/.cpp` | `LinuxPlatformFactory`, `LinuxConsole`, `LinuxAllocator` |
| `linux/file_disk_image.hpp/.cpp` | `FileDiskImage` — Linux file-backed disk |
| `linux/main.cpp` | Linux entry point |
| `esp32/CMakeLists.txt` | ESP-IDF project root (Phase 5) |
| `esp32/main/esp32_platform_factory.hpp/.cpp` | `ESP32PlatformFactory`, `ESP32Console`, `ESP32Allocator`, `SDCardDiskImage` (Phase 5) |
| `esp32/main/main.cpp` | ESP32 entry point (Phase 5) |
| `components/machine/include/iconsole.hpp` | `IConsole` interface |
| `components/machine/include/iallocator.hpp` | `IAllocator` interface |
| `components/machine/include/platform_factory.hpp` | `PlatformFactory` abstract factory |
| `components/cpu/` | `Z80Cpu` — z80.h wrapper |
| `components/memory/` | `MemoryMap` — flat + banked; takes `IAllocator&` |
| `components/disk/include/disk_geometry.hpp` | `DiskGeometry` struct + `KAYPRO2_DSDD/SSDD` |
| `components/disk/include/disk_image.hpp` | `DiskImage` abstract interface |
| `components/disk/` | `WD1793`, `MemoryDiskImage` |
| `components/uart/` | `UART8250` — takes `IConsole&` |
| `components/machine/` | `Machine`, `IOBus` |
| `roms/monitor/` | Embedded monitor ROM blob |
| `roms/boot/boot.asm` | Boot ROM Z80 source |
| `roms/bios.asm` | CP/M 2.2 custom BIOS source |
| `experiments/z80_hello/` | Build system smoke test |

---

## Toolchain & External Dependencies

| Dependency | Source / Notes |
|---|---|
| `chips/z80.h` | `floooh/chips` via CMake `FetchContent` (header-only) |
| Z80 assembler | `sjasmplus` or `z88dk` for `boot.asm` and `bios.asm` |
| CP/M 2.2 binary | Public domain — Unofficial CP/M Web Site / Digital Research archives |
| Z80 monitor ROM | SCM (github.com/RC2014Z80/RC2014) or Grant Searle's Z80 design |
| Disk image tools | `cpmtools` package or a custom Python script |
| ESP-IDF | v5.x — required for Phase 5 only |
