#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

/**
 * Provides a 64 KB active window backed by 256 physical 16 KB banks.
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
     * Writes one byte unless the mapped bank is read-only.
     */
    void write(uint16_t address, uint8_t value);

    /**
     * Loads a ROM blob into the currently mapped banks at a logical base.
     */
    void loadROM(const uint8_t* data, uint16_t base, std::size_t size);

    /**
     * Remaps one 16 KB slot to a physical bank using active-window swapping.
     */
    void mapSlot(uint8_t slot, uint8_t bank);

    /**
     * Returns the currently mapped bank for one slot.
     */
    uint8_t getSlotBank(uint8_t slot) const;

private:
    /**
     * Computes the 16 KB slot index for a logical address.
     */
    static std::size_t slotIndex(uint16_t address);

    /**
     * Flushes one slot from active window to its currently mapped bank.
     */
    void flushSlot(std::size_t slot);

    /**
     * Loads one slot from its currently mapped bank into active window.
     */
    void loadSlot(std::size_t slot);

    /**
     * Returns the byte offset in active memory for a slot.
     */
    static std::size_t slotOffset(std::size_t slot);

    std::array<uint8_t, 65536> memory_{};
    std::vector<uint8_t> physical_{};
    std::array<uint8_t, 4> slot_bank_{};
    std::array<bool, 256> bank_read_only_{};
};
