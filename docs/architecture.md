# ARCHITECTURE.md: The "Generic S-100" Virtual Machine

## 1. System Overview
This project implements a high-fidelity Z80-based microcomputer inspired by the S-100 bus architecture. It is designed as a dual-platform C++ application, running on **Linux** (for rapid development/debugging) and **ESP32-WROVER** (for hardware deployment).

## 2. Processor (CPU)
* **Core:** `chips/z80.h`.
* **Fidelity:** The core accurately models undocumented internal states, including the **MEMPTR** ($WZ$ register) and **Q-flag** behavior, ensuring compatibility with pedantic software.
* **Target Clock:** 4.0 MHz, synchronized via high-resolution timers.

## 3. Memory Architecture: 16K Banked Mapper
The system employs a 4-slot paging system to bridge the Z80's 64KB logical limit with the modern 8MB PSRAM.

### 3.1 Logical Slots
The Z80's 64KB address space is divided into four 16KB slots:
* **Slot 0:** $0000h$ – $3FFFh$
* **Slot 1:** $4000h$ – $7FFFh$
* **Slot 2:** $8000h$ – $BFFFh$
* **Slot 3:** $C000h$ – $FFFFh$

### 3.2 Paging Control (I/O Ports $90h$-$93h$)
Each 8-bit I/O port controls the mapping of one page. They work as registers allowing read and write operations.

* **Port $90h$:** Slot 0
* **Port $91h$:** Slot 1
* **Port $92h$:** Slot 2
* **Port $93h$:** Slot 3

The effective address for each slot will be comprised the 8 bits of the corresponding register and the remaining 14 bits of the address for a total of 22 address bits. 

### 3.3 The "Active Window" Performance Strategy
To maintain 4MHz execution speed and avoid ESP32 PSRAM cache thrashing (SPI latency), the system uses an **Internal Execution Buffer**:
* Z80 instructions and stack operations execute exclusively in 64KB of fast **Internal SRAM**.
* When a new physical bank is mapped into a slot, the ESP32 performs a `memcpy` to sync the current internal slot data back to PSRAM and load the new bank into the internal buffer.

## 4. Disk Subsystem (WD1793)
The system emulates the Western Digital **WD1793** floppy controller at the register level on ports $10h$–$13h$.

| Port | Register (Read) | Register (Write) | Function |
| :--- | :--- | :--- | :--- |
| **$10h$** | Status | Command | Chip state / Opcode entry  |
| **$11h$** | Track | Track | Cylinder position (0–39)  |
| **$12h$** | Sector | Sector | Target sector (physical, 1-based)  |
| **$13h$** | Data | Data | 8-bit data window for transfer  |

**Drive Select:** Port $14h$ carries the drive number (bits 0–1) and the active head/side (bit 2).

**Disk Geometry:** All disk parameters are held in a `DiskGeometry` struct (tracks, sectors-per-track, sector size in bytes, number of sides). The default format is the **Kaypro II 5.25" DSDD**: 2 sides, 40 tracks, 10 sectors/track, 512 bytes/sector (400 KB per disk).

**Sector Mapping:** Disk images (`.dsk`) are stored on the SD card or local filesystem. Byte offsets into the image are calculated as:
$$
Offset = \bigl(Track \times Sides \times SectorsPerTrack + Side \times SectorsPerTrack + Sector\bigr) \times SectorSize
$$
where $Track$ and $Side$ are the physical cylinder and head (both 0-based) and $Sector$ is the 0-based sector index within the track.

## 5. Boot & Reset Sequence
* **Deterministic Power-On:** 
    * On hardware reset, the paging hardware defaults **Slot 0** to **Physical Bank 255**, which contains the Boot ROM.

* **ROM Vanishing:** 
    * The bootloader BIOS in ROM, will: 
        * Disable Interrupts
        * Configure memory mapping as follows
            * **Slot 1** points to **Physical Bank 1**
            * **Slot 2** points to **Physical Bank 2**
            * **Slot 3** points to **Physical Bank 254**
        * Configure the stack pointer to $FFFFh$
        * Write the instructions to map **Physical Bank 0** to **Slot 0** at $C000h$ (Slot 3 lower limit)
        ```asm
        ORG C000H
            XOR A ; AF
            OUT (90H), A ; D3 90
        ```
        * Load Sector 1, Track 0 from disk 0 starting at $C003H$
    * Jump to $C000H$ 

## 6. Console I/O
* I/O must use emulated 8250 UART
    * In development mode (Linux) it will default to the console
    * In ESP32, we will send output to either the console (USB Serial) or a Serial port, so two drivers will be implemented.