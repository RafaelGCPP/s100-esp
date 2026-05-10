#include "machine.hpp"

/**
 * Registers one read callback for a specific port.
 */
void IOBus::registerReadHandler(uint8_t port, ReadHandler handler) {
    read_handlers_[port] = std::move(handler);
}

/**
 * Registers one write callback for a specific port.
 */
void IOBus::registerWriteHandler(uint8_t port, WriteHandler handler) {
    write_handlers_[port] = std::move(handler);
}

/**
 * Executes a read callback for a port, returning 0xFF when unmapped.
 */
uint8_t IOBus::read(uint8_t port) const {
    const auto it = read_handlers_.find(port);
    if (it == read_handlers_.end()) {
        return 0xFF;
    }
    return it->second(port);
}

/**
 * Executes a write callback for a port when mapped.
 */
void IOBus::write(uint8_t port, uint8_t value) const {
    const auto it = write_handlers_.find(port);
    if (it == write_handlers_.end()) {
        return;
    }
    it->second(port, value);
}

/**
 * Builds a machine from memory and UART devices.
 */
Machine::Machine(MemoryMap& memory, UART8250& uart)
    : memory_(memory), uart_(uart) {
    initializeIoBus();
}

/**
 * Resets the machine CPU.
 */
void Machine::reset() {
    cpu_.reset();
}

/**
 * Executes one CPU cycle and handles bus activity.
 */
void Machine::step() {
    const Z80BusRequest request = cpu_.tick();

    switch (request.type) {
        case Z80BusRequest::Type::MemoryRead:
            cpu_.provideReadData(memory_.read(request.address));
            break;
        case Z80BusRequest::Type::MemoryWrite:
            memory_.write(request.address, request.data);
            break;
        case Z80BusRequest::Type::IoRead:
            cpu_.provideReadData(io_bus_.read(static_cast<uint8_t>(request.address & 0xFFU)));
            break;
        case Z80BusRequest::Type::IoWrite:
            io_bus_.write(static_cast<uint8_t>(request.address & 0xFFU), request.data);
            break;
        case Z80BusRequest::Type::InterruptAcknowledge:
            cpu_.provideReadData(0xFF);
            break;
        case Z80BusRequest::Type::None:
            break;
    }
}

/**
 * Runs the machine for a fixed number of cycles.
 */
void Machine::run(std::uint64_t cycles) {
    for (std::uint64_t i = 0; i < cycles; ++i) {
        step();
    }
}

/**
 * Returns the current CPU program counter.
 */
uint16_t Machine::getPC() const {
    return cpu_.getPC();
}

/**
 * Returns true when the CPU is halted.
 */
bool Machine::isHalted() const {
    return cpu_.isHalted();
}

/**
 * Binds device port handlers into the I/O bus.
 */
void Machine::initializeIoBus() {
    for (uint8_t port = 0; port < 8; ++port) {
        io_bus_.registerReadHandler(port, [this](uint8_t io_port) {
            return uart_.ioRead(io_port);
        });

        io_bus_.registerWriteHandler(port, [this](uint8_t io_port, uint8_t value) {
            uart_.ioWrite(io_port, value);
        });
    }
}
