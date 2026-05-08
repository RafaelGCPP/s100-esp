# S-100 Clone Project: Engineering Directives

## Architectural Vision
Building a high-fidelity Z80 system (S-100 style) for ESP32-WROVER (8MB PSRAM) and Linux. 
The proposed architecture is in docs/architecture.md

## Core Rules
1. **CPU & Fidelity:** - Use `z80.h`. It is the "gold standard" for modeling WZ (MEMPTR) and Q-flag behavior.
   - Execution must be cycle-stepped in an "Active Window" to avoid PSRAM latency.

2. **Memory Strategy:**
   - 64KB Logical space divided into 4 slots of 16KB each.
   - Active execution happens in 64KB Internal SRAM.
   - 4MB PSRAM acts as "Physical Storage" (256 banks).
   - Context switches trigger a `memcpy` between PSRAM and SRAM.

3. **Disk Controller:**
   - Emulate the WD1793 at the register level (Ports 10h-13h).
   - Support raw .dsk images mapped linearly: Offset = (Track * Sectors * 128) + (Sector * 128).
   - It will use files in a SD card as disk images. 
   - Support for multiple drives.

4. **Build System:**
   - Use Modern CMake. No submodules for headers; use FetchContent to pull `z80.h` directly from floooh/chips.

## Coding rules

* English language for code and comments
* All functions, classes, methods must be commented
* It will use C++23 as glue language, but it must still interact with both plain C in z80.h and ESP-IDF
* Documentation must go in docs directory
* After writing some code, do a second pass reviewing it.
* Code must build without errors
* Use experiments directory for proof-of-concept, small explorations.
