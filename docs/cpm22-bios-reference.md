# CP/M 2.2 BIOS Reference

## Overview

The CP/M 2.2 BIOS (Basic I/O System) is the hardware-dependent layer of the operating system. It sits at the top of the 64 KB address space, immediately above the BDOS. The BIOS presents a fixed jump table that the BDOS and CCP call to perform all I/O. Every entry is a 3-byte `JP` instruction, so the table can be relocated simply by changing the base address.

```
BIOS base (BOOT):  JP  cold_start
BIOS base + 03h:   JP  warm_start
BIOS base + 06h:   JP  const
BIOS base + 09h:   JP  conin
BIOS base + 0Ch:   JP  conout
BIOS base + 0Fh:   JP  list
BIOS base + 12h:   JP  punch
BIOS base + 15h:   JP  reader
BIOS base + 18h:   JP  home
BIOS base + 1Bh:   JP  seldsk
BIOS base + 1Eh:   JP  settrk
BIOS base + 21h:   JP  setsec
BIOS base + 24h:   JP  setdma
BIOS base + 27h:   JP  read
BIOS base + 2Ah:   JP  write
BIOS base + 2Dh:   JP  listst
BIOS base + 30h:   JP  sectran
```

Seventeen entries total (51 bytes). The BDOS locates the jump table via the system base pointer at address `0001h`; the byte at `0003h` holds the CCP page address.

---

## Register Conventions

CP/M passes arguments to BIOS functions via Z80 registers. Unless noted otherwise:

- **Input parameters** arrive in `BC` (16-bit) or `C` (8-bit, low byte of `BC`).
- **Return values** are placed in `A` (8-bit status) or `HL` (16-bit address/value).
- All other registers (`DE`, `IX`, `IY`, shadow registers) must be **preserved** by the BIOS; the BDOS and CCP do not save them around BIOS calls.

---

## Jump Table Entries

### 0 — BOOT (Cold Start)
**Offset:** `00h`

Called once, immediately after hardware power-on or hard reset. The boot ROM loads the BIOS image into memory and jumps here.

| | Register | Value |
|---|---|---|
| **Input** | — | No parameters |
| **Output** | — | Does not return |

**Expected behaviour:**
1. Initialize all hardware (UART, disk controller, memory map).
2. Set up the CP/M memory layout: CCP at the base of the transient area (`0100h` is the TPA base; CCP sits just below BDOS).
3. Zero the first 3 bytes of page 0 (`0000h`–`0002h`): `NOP`, `JP`, then the warm-boot vector.
4. Place the BDOS entry point at `0005h` (page 0 BDOS vector).
5. Load the CCP+BDOS from the system tracks of drive A if they are not already in RAM (some implementations keep a ROM copy).
6. Jump to the CCP entry point. Never returns.

---

### 1 — WBOOT (Warm Start)
**Offset:** `03h`

Called when a user program terminates (`RET` or `JP 0000h`), or when the user presses `Ctrl-C` at the console.

| | Register | Value |
|---|---|---|
| **Input** | — | No parameters |
| **Output** | — | Does not return |

**Expected behaviour:**
1. Reload the CCP (and optionally the BDOS) from the system tracks of the currently logged disk.
2. Restore the page-0 vectors (`0000h` warm-boot jump, `0005h` BDOS entry).
3. Re-initialise hardware as needed (e.g. restore UART settings, re-home the disk head if required).
4. Jump to the CCP entry point, passing the current default drive in register `C` (0=A, 1=B, …). Never returns.

---

### 2 — CONST (Console Status)
**Offset:** `06h`

Non-blocking poll of the console input device.

| | Register | Value |
|---|---|---|
| **Input** | — | No parameters |
| **Output** | `A` | `FFh` — character ready; `00h` — no character available |

**Expected behaviour:**  
Return `FFh` immediately if a character is waiting in the UART receive buffer (or stdin), `00h` otherwise. Must never block.

---

### 3 — CONIN (Console Input)
**Offset:** `09h`

Blocking read of one character from the console.

| | Register | Value |
|---|---|---|
| **Input** | — | No parameters |
| **Output** | `A` | ASCII character (bit 7 = 0); parity bit is stripped |

**Expected behaviour:**  
Block until a byte is available from the console UART. Strip the high bit before returning. Do not echo the character — the BDOS handles echo when appropriate.

---

### 4 — CONOUT (Console Output)
**Offset:** `0Ch`

Write one character to the console.

| | Register | Value |
|---|---|---|
| **Input** | `C` | ASCII character to output |
| **Output** | — | None |

**Expected behaviour:**  
Transmit the character in `C` to the console UART. Block until the transmit register is empty if necessary. Only the low 7 bits are significant; the high bit may be ignored.

---

### 5 — LIST (List / Printer Output)
**Offset:** `0Fh`

Write one character to the list device (typically a printer).

| | Register | Value |
|---|---|---|
| **Input** | `C` | ASCII character to output |
| **Output** | — | None |

**Expected behaviour:**  
Transmit the character to the printer port. If no printer is implemented, this function may silently discard the character. Block until ready if necessary.

---

### 6 — PUNCH (Punch Output)
**Offset:** `12h`

Write one byte to the punch device (historically a paper-tape punch).

| | Register | Value |
|---|---|---|
| **Input** | `C` | Byte to output |
| **Output** | — | None |

**Expected behaviour:**  
Transmit `C` to the punch device. May be mapped to a second serial port or silently discarded if not implemented.

---

### 7 — READER (Reader Input)
**Offset:** `15h`

Read one byte from the reader device (historically a paper-tape reader).

| | Register | Value |
|---|---|---|
| **Input** | — | No parameters |
| **Output** | `A` | Byte read, or `1Ah` (^Z, EOF) if not implemented |

**Expected behaviour:**  
Return the next byte from the reader device. If no reader is present, return `1Ah` (`Ctrl-Z`), which CP/M interprets as end-of-file.

---

### 8 — HOME (Move Head to Track 0)
**Offset:** `18h`

Seek the currently selected drive's head to track 0.

| | Register | Value |
|---|---|---|
| **Input** | — | No parameters |
| **Output** | — | None |

**Expected behaviour:**  
Issue a Restore or Seek-to-0 command on the selected drive. Equivalent to calling `SETTRK` with track number 0. The BDOS calls `HOME` before scanning the directory.

---

### 9 — SELDSK (Select Disk)
**Offset:** `1Bh`

Select the active disk drive and return its parameter header.

| | Register | Value |
|---|---|---|
| **Input** | `C` | Drive number: 0 = A, 1 = B, 2 = C, 3 = D |
| | `E` | `0` = first select (log in); `1` = subsequent select (already logged) |
| **Output** | `HL` | Address of the **Disk Parameter Header** (DPH) for the drive, or `0000h` if the drive does not exist |

**Expected behaviour:**  
Validate the drive number. If invalid, return `HL = 0000h`. Otherwise record the selected drive for subsequent `READ`/`WRITE` calls, and return a pointer to the drive's static DPH structure. The `E` flag may be used to skip physical head seeks on reselects; at minimum it must be accepted without error.

#### Disk Parameter Header (DPH) — 16 bytes

| Offset | Size | Name | Description |
|---|---|---|---|
| `+00h` | 2 | `XLT` | Address of sector translation table; `0000h` if no translation |
| `+02h` | 2 | `SCC` | Scratch — initialize to `0000h` |
| `+04h` | 2 | `INF` | Scratch — initialize to `0000h` |
| `+06h` | 2 | `DVR` | Scratch — initialize to `0000h` |
| `+08h` | 2 | `DIRBUF` | Address of a shared 128-byte scratch buffer for directory operations |
| `+0Ah` | 2 | `DPB` | Address of the **Disk Parameter Block** for this drive |
| `+0Ch` | 2 | `CSV` | Address of the check-sum vector (`(DRM+1)/4` bytes, or `0000h` for fixed media) |
| `+0Eh` | 2 | `ALV` | Address of the allocation vector (`(DSM/8)+1` bytes) |

#### Disk Parameter Block (DPB) — 15 bytes

| Offset | Size | Name | Description |
|---|---|---|---|
| `+00h` | 2 | `SPT` | Sectors per track (in 128-byte logical records) |
| `+02h` | 1 | `BSH` | Block shift factor: `BSH = log2(block_size / 128)` |
| `+03h` | 1 | `BLM` | Block mask: `BLM = (1 << BSH) - 1` |
| `+04h` | 1 | `EXM` | Extent mask |
| `+05h` | 2 | `DSM` | Maximum block number on disk (`total_blocks - 1`) |
| `+07h` | 2 | `DRM` | Maximum directory entry number (`total_dir_entries - 1`) |
| `+09h` | 1 | `AL0` | Directory allocation bitmap, byte 0 |
| `+0Ah` | 1 | `AL1` | Directory allocation bitmap, byte 1 |
| `+0Bh` | 2 | `CKS` | Checksum vector size: `(DRM+1)/4`; use `0` for non-removable media |
| `+0Dh` | 2 | `OFF` | Number of reserved (system) tracks at the start of the disk |

**DPB values for Kaypro II 5.25" DSDD (2 sides × 40 tracks × 10 sectors × 512 B, 2 system tracks):**

Logical tracks = 80 (side 0: tracks 0–39, side 1: tracks 40–79). The BIOS translates a logical track number to a physical (cylinder, side) pair: `cylinder = track % 40`, `side = track / 40`.

| Field | Value | Derivation |
|---|---|---|
| SPT | 40 | 10 physical sectors × 512 B / 128 B per logical record |
| BSH | 4 | 2 KB blocks → log₂(2048/128) = 4 |
| BLM | 15 | (1 << 4) − 1 |
| EXM | 1 | DSM < 256 and block size = 2 KB → (2048/1024) − 1 |
| DSM | 194 | 78 data tracks × 40 records / 16 records per block − 1 |
| DRM | 63 | 64 directory entries |
| AL0 | 80h | Block 0 reserved for directory (64 × 32 B = 2048 B = 1 block) |
| AL1 | 00h | |
| CKS | 16 | (DRM+1)/4 = 64/4 — removable media |
| OFF | 2 | 2 system tracks |

---

### 10 — SETTRK (Set Track)
**Offset:** `1Eh`

Record the track number for the next disk operation.

| | Register | Value |
|---|---|---|
| **Input** | `BC` | Track number (0-based) |
| **Output** | — | None |

**Expected behaviour:**  
Store `BC` in a BIOS-private variable. The WD1793 seek is deferred until `READ` or `WRITE` is called; alternatively, the seek may be performed immediately. Track numbers range from 0 to `DPB.OFF + (data tracks − 1)`.

---

### 11 — SETSEC (Set Sector)
**Offset:** `21h`

Record the sector number for the next disk operation.

| | Register | Value |
|---|---|---|
| **Input** | `BC` | Logical sector number (after translation by `SECTRAN`) |
| **Output** | — | None |

**Expected behaviour:**  
Store `BC` in a BIOS-private variable. Sector numbers passed here are already translated from logical to physical if a translation table is in use.

---

### 12 — SETDMA (Set DMA Address)
**Offset:** `24h`

Set the memory address for the next sector transfer.

| | Register | Value |
|---|---|---|
| **Input** | `BC` | Address of a 128-byte buffer in RAM |
| **Output** | — | None |

**Expected behaviour:**  
Store `BC` as the current DMA address. All subsequent `READ` and `WRITE` calls transfer exactly 128 bytes to/from this address until `SETDMA` is called again. The default DMA address at warm boot is `0080h`.

> **Note:** CP/M 2.2 always transfers exactly **128 bytes** per sector at the BIOS interface, regardless of the physical sector size. For 512-byte physical sectors (as on the Kaypro II), the BIOS must manage the four 128-byte sub-sectors internally. The physical sector holding a given logical record is `physical_sector = logical_sector / 4` (0-based) and the sub-sector offset within it is `(logical_sector % 4) * 128`. `WRITE` must perform a read-modify-write when updating a sub-sector to avoid corrupting the other three 128-byte portions of the same physical sector.

---

### 13 — READ (Read Sector)
**Offset:** `27h`

Read one 128-byte logical sector into the DMA buffer.

| | Register | Value |
|---|---|---|
| **Input** | — | Uses previously set drive, track, sector, DMA address |
| **Output** | `A` | `00h` — success; `01h` — unrecoverable read error; `FFh` — media changed (not used in CP/M 2.2) |

**Expected behaviour:**
1. Address the disk using the current drive, track, and sector values.
2. Issue a WD1793 Read Sector command.
3. Poll DRQ; transfer `sector_size` bytes from the WD1793 Data register to a temporary buffer (or directly to DMA if `sector_size` == 128).
4. For 512-byte physical sectors, calculate the physical sector as `logical_sector / 4` and the sub-sector byte offset as `(logical_sector % 4) * 128`; copy the correct 128-byte slice to the DMA address.
5. Copy the 128 bytes to the DMA address.
6. Return `00h` on success, `01h` if the WD1793 reports an error after retries.

---

### 14 — WRITE (Write Sector)
**Offset:** `2Ah`

Write one 128-byte logical sector from the DMA buffer to disk.

| | Register | Value |
|---|---|---|
| **Input** | `C` | Write type: `0` = normal; `1` = directory sector; `2` = first sector of a newly allocated block |
| **Output** | `A` | `00h` — success; `01h` — unrecoverable write error; `02h` — write protected |

**Expected behaviour:**
1. Check write-protect status; return `02h` immediately if protected.
2. For 512-byte physical sectors, read the existing physical sector into a 512-byte temporary buffer (read-modify-write), then overwrite the 128-byte slice at offset `(logical_sector % 4) * 128` with data from the DMA address.
3. Issue a WD1793 Write Sector command; transfer the full physical sector to the controller.
4. The `C` parameter (write type) may be used to optimize block pre-clearing for type `2` (new block allocation), but a minimal implementation may ignore it.
5. Return `00h` on success, `01h` on error.

---

### 15 — LISTST (List Device Status)
**Offset:** `2Dh`

Non-blocking poll of the list (printer) output device.

| | Register | Value |
|---|---|---|
| **Input** | — | No parameters |
| **Output** | `A` | `FFh` — list device ready; `00h` — not ready |

**Expected behaviour:**  
Return `FFh` if the printer port transmit register is empty (ready to accept a character), `00h` otherwise. If no printer is implemented, return `FFh` unconditionally so that `LIST` does not deadlock.

---

### 16 — SECTRAN (Sector Translate)
**Offset:** `30h`

Translate a logical sector number to a physical sector number.

| | Register | Value |
|---|---|---|
| **Input** | `BC` | Logical sector number (0-based) |
| | `DE` | Address of the sector translation table, or `0000h` |
| **Output** | `HL` | Physical sector number |

**Expected behaviour:**  
If `DE` is non-zero, use it as a base address into a byte array: `HL = (uint8_t*)(DE)[BC]`. If `DE` is `0000h` (no translation table in the DPH), return `HL = BC` unchanged. The Kaypro II geometry does not require software sector interleaving at the BIOS level, so the DPH should set `XLT = 0000h` and this function will return `HL = BC` for all calls.

---

## BIOS Entry Point Summary

| # | Name | Offset | Input | Output | Blocks? |
|---|---|---|---|---|---|
| 0 | BOOT | `00h` | — | — (no return) | — |
| 1 | WBOOT | `03h` | — | — (no return) | — |
| 2 | CONST | `06h` | — | A = `FFh`/`00h` | No |
| 3 | CONIN | `09h` | — | A = char | Yes |
| 4 | CONOUT | `0Ch` | C = char | — | Yes |
| 5 | LIST | `0Fh` | C = char | — | Yes |
| 6 | PUNCH | `12h` | C = byte | — | Yes |
| 7 | READER | `15h` | — | A = byte | Yes |
| 8 | HOME | `18h` | — | — | Yes |
| 9 | SELDSK | `1Bh` | C = drive, E = flag | HL = DPH or `0000h` | No |
| 10 | SETTRK | `1Eh` | BC = track | — | No |
| 11 | SETSEC | `21h` | BC = sector | — | No |
| 12 | SETDMA | `24h` | BC = address | — | No |
| 13 | READ | `27h` | — | A = status | Yes |
| 14 | WRITE | `2Ah` | C = type | A = status | Yes |
| 15 | LISTST | `2Dh` | — | A = `FFh`/`00h` | No |
| 16 | SECTRAN | `30h` | BC = logical sec, DE = table | HL = physical sec | No |

---

## References

- *CP/M 2.2 System Alteration Guide*, Digital Research, 1979.
- *CP/M 2.2 Interface Guide*, Digital Research, 1979.
- Unofficial CP/M Web Site: http://www.cpm.z80.de
