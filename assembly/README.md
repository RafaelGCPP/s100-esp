# Assembly Artifacts

This directory contains standalone Z80 assembly sources and binaries for firmware bring-up.

## Files

- BIOS.ASM: CP/M 2.2 BIOS skeleton with jump table, boot/console routines, and staged stubs.

## Build Policy

Assembly binaries are intentionally built outside CMake.

## Build Commands

Run from repository root.

### z80asm

```bash
z80asm -b -o assembly/BIOS.BIN assembly/BIOS.ASM
```

### pasmo

```bash
pasmo --bin assembly/BIOS.ASM assembly/BIOS.BIN
```

## Current BIOS Scope

Implemented and usable now:

- BOOT
- WBOOT
- CONST
- CONIN
- CONOUT

Stubbed by design for staged bring-up:

- LIST, PUNCH: drop character and return
- READER: returns 1Ah
- LISTST: returns FFh
- HOME, SELDSK, SETTRK, SETSEC, SETDMA, READ, WRITE, SECTRAN: placeholders for later disk integration

## Next Steps

1. Replace placeholder CCP/BDOS addresses in BIOS.ASM with the finalized memory layout.
2. Implement SELDSK through WRITE against your WD1793 emulation contract.
3. Add a small script (optional) to automate binary regeneration when BIOS.ASM changes.
