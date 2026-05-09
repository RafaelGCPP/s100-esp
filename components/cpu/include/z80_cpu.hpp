#pragma once

#include <cstdint>

#include <chips/z80.h>

/**
 * Represents a decoded bus request for one CPU tick.
 */
struct Z80BusRequest {
    /**
     * Classifies the active request type from the pin mask.
     */
    enum class Type {
        None,
        MemoryRead,
        MemoryWrite,
        IoRead,
        IoWrite,
        InterruptAcknowledge
    };

    Type type = Type::None;
    uint16_t address = 0;
    uint8_t data = 0;
};

/**
 * Wraps chips/z80.h and exposes cycle-stepped bus requests.
 */
class Z80Cpu {
public:
    /**
     * Initializes and resets the CPU.
     */
    Z80Cpu();

    /**
     * Resets the CPU to its power-on state.
     */
    void reset();

    /**
     * Executes one CPU cycle and decodes the resulting bus request.
     */
    Z80BusRequest tick();

    /**
     * Places read data onto the current pin state for the next cycle.
     */
    void provideReadData(uint8_t data);

    /**
     * Returns the current program counter.
     */
    uint16_t getPC() const;

private:
    /**
     * Converts the current pin mask to a structured bus request.
     */
    Z80BusRequest decodeRequest() const;

    z80_t cpu_{};
    uint64_t pins_ = 0;
};
