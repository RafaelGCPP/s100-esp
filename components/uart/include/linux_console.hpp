#pragma once

#include <cstdint>

#include "uart_platform.hpp"

/**
 * Provides raw terminal console I/O for Linux hosts.
 */
class LinuxConsole final : public UartPlatform {
public:
    /**
     * Enters raw non-blocking terminal mode.
     */
    LinuxConsole();

    /**
     * Restores the original terminal mode.
     */
    ~LinuxConsole() override;

    /**
     * Sends one character to stdout.
     */
    void putChar(uint8_t value) override;

    /**
     * Returns true if one character can be read from stdin.
     */
    bool hasChar() override;

    /**
     * Receives one character from stdin.
     */
    uint8_t getChar() override;

private:
    /**
     * Saves and applies terminal settings.
     */
    void configureTerminal();

    /**
     * Restores terminal settings and file descriptor flags.
     */
    void restoreTerminal();

    bool terminal_configured_ = false;
    int original_flags_ = 0;
    struct termios* original_termios_ = nullptr;
    bool has_pending_char_ = false;
    uint8_t pending_char_ = 0;
};
