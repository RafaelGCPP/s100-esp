#include "linux_console.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

/**
 * Enters raw non-blocking terminal mode.
 */
LinuxConsole::LinuxConsole() {
    configureTerminal();
}

/**
 * Restores the original terminal mode.
 */
LinuxConsole::~LinuxConsole() {
    restoreTerminal();
}

/**
 * Sends one character to stdout.
 */
void LinuxConsole::putChar(uint8_t value) {
    std::fputc(static_cast<int>(value), stdout);
    std::fflush(stdout);
}

/**
 * Returns true if one character can be read from stdin.
 */
bool LinuxConsole::hasChar() {
    if (has_pending_char_) {
        return true;
    }

    unsigned char value = 0;
    const ssize_t result = ::read(STDIN_FILENO, &value, 1);
    if (result == 1) {
        pending_char_ = value;
        has_pending_char_ = true;
        return true;
    }
    return false;
}

/**
 * Receives one character from stdin.
 */
uint8_t LinuxConsole::getChar() {
    if (has_pending_char_) {
        has_pending_char_ = false;
        return pending_char_;
    }

    unsigned char value = 0;
    const ssize_t result = ::read(STDIN_FILENO, &value, 1);
    if (result == 1) {
        return value;
    }
    return 0;
}

/**
 * Saves and applies terminal settings.
 */
void LinuxConsole::configureTerminal() {
    auto* saved = static_cast<termios*>(std::malloc(sizeof(termios)));
    if (saved == nullptr) {
        return;
    }

    if (::tcgetattr(STDIN_FILENO, saved) != 0) {
        std::free(saved);
        return;
    }

    termios raw = *saved;
    raw.c_lflag = static_cast<tcflag_t>(raw.c_lflag & ~(ICANON | ECHO));
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (::tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
        std::free(saved);
        return;
    }

    original_flags_ = ::fcntl(STDIN_FILENO, F_GETFL, 0);
    if (original_flags_ >= 0) {
        (void)::fcntl(STDIN_FILENO, F_SETFL, original_flags_ | O_NONBLOCK);
    }

    original_termios_ = saved;
    terminal_configured_ = true;
}

/**
 * Restores terminal settings and file descriptor flags.
 */
void LinuxConsole::restoreTerminal() {
    if (!terminal_configured_ || (original_termios_ == nullptr)) {
        return;
    }

    (void)::tcsetattr(STDIN_FILENO, TCSANOW, original_termios_);
    if (original_flags_ >= 0) {
        (void)::fcntl(STDIN_FILENO, F_SETFL, original_flags_);
    }

    std::free(original_termios_);
    original_termios_ = nullptr;
    terminal_configured_ = false;
}
