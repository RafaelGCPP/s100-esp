#pragma once

#include <array>
#include <cstdint>

#include "uart_platform.hpp"

/**
 * Emulates a polled 8250 UART register set.
 */
class UART8250 {
public:
    /**
     * Constructs a UART at a given base port.
     */
    UART8250(UartPlatform& platform, uint8_t base_port);

    /**
     * Reads one register from an absolute I/O port.
     */
    uint8_t ioRead(uint8_t port);

    /**
     * Writes one register through an absolute I/O port.
     */
    void ioWrite(uint8_t port, uint8_t value);

    /**
     * Returns true if the absolute port belongs to this UART.
     */
    bool handlesPort(uint8_t port) const;

private:
    /**
     * Computes register index from absolute port.
     */
    uint8_t regIndex(uint8_t port) const;

    /**
     * Builds line status bits based on current host I/O state.
     */
    uint8_t lineStatus() const;

    UartPlatform& platform_;
    uint8_t base_port_ = 0;

    uint8_t ier_ = 0;
    uint8_t iir_ = 0x01;
    uint8_t lcr_ = 0;
    uint8_t mcr_ = 0;
    uint8_t msr_ = 0;
    uint8_t scr_ = 0;
    mutable uint8_t lsr_cache_ = 0x60;
};
