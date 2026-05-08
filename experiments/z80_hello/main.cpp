#include <cstdint>
#include <iostream>

#include <chips/z80.h>

/**
 * Feeds default values into the Z80 data bus during read operations.
 */
static uint64_t feed_bus(uint64_t pins) {
    if ((pins & Z80_MREQ) && (pins & Z80_RD)) {
        Z80_SET_DATA(pins, 0x00);
    }
    if ((pins & Z80_IORQ) && (pins & Z80_RD)) {
        Z80_SET_DATA(pins, 0xFF);
    }
    return pins;
}

/**
 * Executes 100 CPU ticks and prints the final program counter.
 */
int main() {
    z80_t cpu{};
    uint64_t pins = z80_init(&cpu);

    for (int i = 0; i < 100; ++i) {
        pins = z80_tick(&cpu, pins);
        pins = feed_bus(pins);
    }

    std::cout << "z80_hello PC=" << cpu.pc << '\n';
    return 0;
}
