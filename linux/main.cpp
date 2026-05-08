#include <cstdint>
#include <iostream>

#include <chips/z80.h>

/**
 * Provide data for memory reads and I/O reads while the CPU is ticking.
 */
static uint64_t service_bus(uint64_t pins) {
    if ((pins & Z80_MREQ) && (pins & Z80_RD)) {
        Z80_SET_DATA(pins, 0x00);
    }
    if ((pins & Z80_IORQ) && (pins & Z80_RD)) {
        Z80_SET_DATA(pins, 0xFF);
    }
    return pins;
}

/**
 * Runs a basic Z80 smoke test and prints the resulting program counter.
 */
int main() {
    z80_t cpu{};
    uint64_t pins = z80_init(&cpu);

    for (int i = 0; i < 100; ++i) {
        pins = z80_tick(&cpu, pins);
        pins = service_bus(pins);
    }

    std::cout << "Phase 0 smoke test complete. PC=" << cpu.pc << '\n';
    return 0;
}
