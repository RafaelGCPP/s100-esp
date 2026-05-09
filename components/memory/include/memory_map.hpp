#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

/**
 * Provides a flat 64 KB memory map with per-1 KB ROM protection.
 */
class MemoryMap {
public:
    /**
     * Constructs a zero-initialized memory map.
     */
    MemoryMap();

    /**
     * Reads one byte from the given address.
     */
    uint8_t read(uint16_t address) const;

    /**
     * Writes one byte unless the destination page is ROM-protected.
     */
    void write(uint16_t address, uint8_t value);

    /**
     * Loads a ROM blob at base and marks covered pages read-only.
     */
    void loadROM(const uint8_t* data, uint16_t base, std::size_t size);

private:
    /**
     * Computes the 1 KB page index for an address.
     */
    static std::size_t pageIndex(uint16_t address);

    std::array<uint8_t, 65536> memory_{};
    std::array<bool, 64> rom_page_{};
};
