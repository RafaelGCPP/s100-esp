#pragma once

#include <cstddef>
#include <cstdint>

/**
 * Minimal monitor placeholder ROM for Phase 1 machine bring-up.
 */
inline constexpr uint8_t monitor_rom[] = {
    0x3E, 0x3E,       // LD A, '>'
    0xD3, 0x00,       // OUT (00h), A
    0xDB, 0x00,       // IN A, (00h)
    0xD3, 0x00,       // OUT (00h), A
    0xC3, 0x04, 0x00  // JP 0004h
};

/**
 * Size of the Phase 1 monitor ROM.
 */
inline constexpr std::size_t monitor_rom_size = sizeof(monitor_rom);
