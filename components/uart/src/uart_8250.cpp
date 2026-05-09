#include "uart_8250.hpp"

/**
 * Constructs a UART at a given base port.
 */
UART8250::UART8250(UartPlatform& platform, uint8_t base_port)
    : platform_(platform), base_port_(base_port) {
}

/**
 * Reads one register from an absolute I/O port.
 */
uint8_t UART8250::ioRead(uint8_t port) {
    const uint8_t reg = regIndex(port);
    switch (reg) {
        case 0:
            if (platform_.hasChar()) {
                return platform_.getChar();
            }
            return 0;
        case 1:
            return ier_;
        case 2:
            return iir_;
        case 3:
            return lcr_;
        case 4:
            return mcr_;
        case 5:
            return lineStatus();
        case 6:
            return msr_;
        case 7:
            return scr_;
        default:
            return 0xFF;
    }
}

/**
 * Writes one register through an absolute I/O port.
 */
void UART8250::ioWrite(uint8_t port, uint8_t value) {
    const uint8_t reg = regIndex(port);
    switch (reg) {
        case 0:
            platform_.putChar(value);
            break;
        case 1:
            ier_ = value;
            break;
        case 2:
            iir_ = value;
            break;
        case 3:
            lcr_ = value;
            break;
        case 4:
            mcr_ = value;
            break;
        case 6:
            msr_ = value;
            break;
        case 7:
            scr_ = value;
            break;
        default:
            break;
    }
}

/**
 * Returns true if the absolute port belongs to this UART.
 */
bool UART8250::handlesPort(uint8_t port) const {
    return (port >= base_port_) && (port < static_cast<uint8_t>(base_port_ + 8));
}

/**
 * Computes register index from absolute port.
 */
uint8_t UART8250::regIndex(uint8_t port) const {
    return static_cast<uint8_t>(port - base_port_);
}

/**
 * Builds line status bits based on current host I/O state.
 */
uint8_t UART8250::lineStatus() const {
    constexpr uint8_t kDataReady = 0x01;
    constexpr uint8_t kThre = 0x20;
    constexpr uint8_t kTemt = 0x40;

    lsr_cache_ = static_cast<uint8_t>(kThre | kTemt);
    if (platform_.hasChar()) {
        lsr_cache_ = static_cast<uint8_t>(lsr_cache_ | kDataReady);
    }
    return lsr_cache_;
}
