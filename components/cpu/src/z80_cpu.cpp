#include "z80_cpu.hpp"

/**
 * Initializes and resets the CPU.
 */
Z80Cpu::Z80Cpu() {
    pins_ = z80_init(&cpu_);
    pins_ = z80_reset(&cpu_);
}

/**
 * Resets the CPU to its power-on state.
 */
void Z80Cpu::reset() {
    pins_ = z80_reset(&cpu_);
}

/**
 * Executes one CPU cycle and decodes the resulting bus request.
 */
Z80BusRequest Z80Cpu::tick() {
    pins_ = z80_tick(&cpu_, pins_);
    return decodeRequest();
}

/**
 * Places read data onto the current pin state for the next cycle.
 */
void Z80Cpu::provideReadData(uint8_t data) {
    Z80_SET_DATA(pins_, data);
}

/**
 * Returns the current program counter.
 */
uint16_t Z80Cpu::getPC() const {
    return cpu_.pc;
}

/**
 * Converts the current pin mask to a structured bus request.
 */
Z80BusRequest Z80Cpu::decodeRequest() const {
    Z80BusRequest request{};

    const bool has_memory_request = (pins_ & Z80_MREQ) != 0;
    const bool has_io_request = (pins_ & Z80_IORQ) != 0;
    const bool is_read = (pins_ & Z80_RD) != 0;
    const bool is_write = (pins_ & Z80_WR) != 0;
    const bool is_interrupt_ack = has_io_request && ((pins_ & Z80_M1) != 0);

    request.address = Z80_GET_ADDR(pins_);
    request.data = Z80_GET_DATA(pins_);

    if (is_interrupt_ack) {
        request.type = Z80BusRequest::Type::InterruptAcknowledge;
    } else if (has_memory_request && is_read) {
        request.type = Z80BusRequest::Type::MemoryRead;
    } else if (has_memory_request && is_write) {
        request.type = Z80BusRequest::Type::MemoryWrite;
    } else if (has_io_request && is_read) {
        request.type = Z80BusRequest::Type::IoRead;
    } else if (has_io_request && is_write) {
        request.type = Z80BusRequest::Type::IoWrite;
    }

    return request;
}
