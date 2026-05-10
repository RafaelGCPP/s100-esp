#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>

#include "memory_map.hpp"
#include "uart_8250.hpp"
#include "z80_cpu.hpp"

/**
 * Routes I/O port accesses to registered device callbacks.
 */
class IOBus {
public:
    /**
     * Callback type for port reads.
     */
    using ReadHandler = std::function<uint8_t(uint8_t)>;

    /**
     * Callback type for port writes.
     */
    using WriteHandler = std::function<void(uint8_t, uint8_t)>;

    /**
     * Registers one read callback for a specific port.
     */
    void registerReadHandler(uint8_t port, ReadHandler handler);

    /**
     * Registers one write callback for a specific port.
     */
    void registerWriteHandler(uint8_t port, WriteHandler handler);

    /**
     * Executes a read callback for a port, returning 0xFF when unmapped.
     */
    uint8_t read(uint8_t port) const;

    /**
     * Executes a write callback for a port when mapped.
     */
    void write(uint8_t port, uint8_t value) const;

private:
    std::unordered_map<uint8_t, ReadHandler> read_handlers_{};
    std::unordered_map<uint8_t, WriteHandler> write_handlers_{};
};

/**
 * Coordinates CPU execution and dispatches memory and I/O accesses.
 */
class Machine {
public:
    /**
     * Builds a machine from memory and UART devices.
     */
    Machine(MemoryMap& memory, UART8250& uart);

    /**
     * Resets the machine CPU.
     */
    void reset();

    /**
     * Executes one CPU cycle and handles bus activity.
     */
    void step();

    /**
     * Runs the machine for a fixed number of cycles.
     */
    void run(std::uint64_t cycles);

    /**
     * Returns the current CPU program counter.
     */
    uint16_t getPC() const;

    /**
     * Returns true when the CPU is halted.
     */
    bool isHalted() const;

private:
    /**
     * Binds device port handlers into the I/O bus.
     */
    void initializeIoBus();

    Z80Cpu cpu_{};
    MemoryMap& memory_;
    UART8250& uart_;
    IOBus io_bus_{};
};
