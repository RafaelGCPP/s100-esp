#pragma once

#include <cstdint>

/**
 * Abstract console I/O used by the 8250 emulation.
 */
class UartPlatform {
public:
    /**
     * Virtual destructor for interface cleanup.
     */
    virtual ~UartPlatform() = default;

    /**
     * Sends one character to the host output.
     */
    virtual void putChar(uint8_t value) = 0;

    /**
     * Returns true if one input character is available.
     */
    virtual bool hasChar() = 0;

    /**
     * Receives one character from host input.
     */
    virtual uint8_t getChar() = 0;
};
