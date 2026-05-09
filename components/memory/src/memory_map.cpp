#include "memory_map.hpp"

#include <algorithm>

/**
 * Constructs a zero-initialized memory map.
 */
MemoryMap::MemoryMap() {
    memory_.fill(0);
    rom_page_.fill(false);
}

/**
 * Reads one byte from the given address.
 */
uint8_t MemoryMap::read(uint16_t address) const {
    return memory_[address];
}

/**
 * Writes one byte unless the destination page is ROM-protected.
 */
void MemoryMap::write(uint16_t address, uint8_t value) {
    if (rom_page_[pageIndex(address)]) {
        return;
    }
    memory_[address] = value;
}

/**
 * Loads a ROM blob at base and marks covered pages read-only.
 */
void MemoryMap::loadROM(const uint8_t* data, uint16_t base, std::size_t size) {
    if ((data == nullptr) || (size == 0)) {
        return;
    }

    const std::size_t start = base;
    const std::size_t end = std::min<std::size_t>(memory_.size(), start + size);

    for (std::size_t i = start; i < end; ++i) {
        memory_[i] = data[i - start];
        rom_page_[pageIndex(static_cast<uint16_t>(i))] = true;
    }
}

/**
 * Computes the 1 KB page index for an address.
 */
std::size_t MemoryMap::pageIndex(uint16_t address) {
    return static_cast<std::size_t>(address) / 1024U;
}
