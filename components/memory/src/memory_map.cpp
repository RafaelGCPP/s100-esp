#include "memory_map.hpp"

#include <algorithm>
#include <cstring>

namespace {
constexpr std::size_t kSlotSize = 16U * 1024U;
constexpr std::size_t kSlotCount = 4U;
constexpr std::size_t kBankCount = 256U;
constexpr std::size_t kWindowSize = kSlotSize * kSlotCount;
constexpr uint8_t kBootBank = 0xFFU;
}

/**
 * Constructs a zero-initialized memory map.
 */
MemoryMap::MemoryMap() {
    memory_.fill(0);
    physical_.assign(kBankCount * kSlotSize, 0);
    slot_bank_ = {kBootBank, 0U, 0U, 0U};
    bank_read_only_.fill(false);
    bank_read_only_[kBootBank] = true;

    for (std::size_t slot = 0; slot < kSlotCount; ++slot) {
        loadSlot(slot);
    }
}

/**
 * Reads one byte from the given address.
 */
uint8_t MemoryMap::read(uint16_t address) const {
    return memory_[address];
}

/**
 * Writes one byte unless the mapped bank is read-only.
 */
void MemoryMap::write(uint16_t address, uint8_t value) {
    const std::size_t slot = slotIndex(address);
    if (bank_read_only_[slot_bank_[slot]]) {
        return;
    }

    memory_[address] = value;
}

/**
 * Loads a ROM blob into the currently mapped banks at a logical base.
 */
void MemoryMap::loadROM(const uint8_t* data, uint16_t base, std::size_t size) {
    if ((data == nullptr) || (size == 0)) {
        return;
    }

    const std::size_t start = base;
    const std::size_t end = std::min<std::size_t>(kWindowSize, start + size);

    for (std::size_t i = start; i < end; ++i) {
        const std::size_t slot = i / kSlotSize;
        const std::size_t offset_in_slot = i % kSlotSize;
        const std::size_t bank = slot_bank_[slot];
        const std::size_t bank_base = bank * kSlotSize;

        memory_[i] = data[i - start];
        physical_[bank_base + offset_in_slot] = data[i - start];
    }
}

/**
 * Remaps one 16 KB slot to a physical bank using active-window swapping.
 */
void MemoryMap::mapSlot(uint8_t slot, uint8_t bank) {
    if (slot >= kSlotCount) {
        return;
    }

    const std::size_t slot_index = static_cast<std::size_t>(slot);
    if (slot_bank_[slot_index] == bank) {
        return;
    }

    flushSlot(slot_index);
    slot_bank_[slot_index] = bank;
    loadSlot(slot_index);
}

/**
 * Returns the currently mapped bank for one slot.
 */
uint8_t MemoryMap::getSlotBank(uint8_t slot) const {
    if (slot >= kSlotCount) {
        return 0xFFU;
    }
    return slot_bank_[slot];
}

/**
 * Computes the 16 KB slot index for a logical address.
 */
std::size_t MemoryMap::slotIndex(uint16_t address) {
    return static_cast<std::size_t>(address) / kSlotSize;
}

/**
 * Flushes one slot from active window to its currently mapped bank.
 */
void MemoryMap::flushSlot(std::size_t slot) {
    const std::size_t bank = slot_bank_[slot];
    if (bank_read_only_[bank]) {
        return;
    }

    const std::size_t bank_base = bank * kSlotSize;
    const std::size_t window_base = slotOffset(slot);
    std::memcpy(&physical_[bank_base], &memory_[window_base], kSlotSize);
}

/**
 * Loads one slot from its currently mapped bank into active window.
 */
void MemoryMap::loadSlot(std::size_t slot) {
    const std::size_t bank = slot_bank_[slot];
    const std::size_t bank_base = bank * kSlotSize;
    const std::size_t window_base = slotOffset(slot);
    std::memcpy(&memory_[window_base], &physical_[bank_base], kSlotSize);
}

/**
 * Returns the byte offset in active memory for a slot.
 */
std::size_t MemoryMap::slotOffset(std::size_t slot) {
    return slot * kSlotSize;
}
