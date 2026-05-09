#include <iostream>

#include "linux_console.hpp"
#include "machine.hpp"
#include "memory_map.hpp"
#include "monitor_rom.hpp"
#include "uart_8250.hpp"

/**
 * Boots the Phase 1 machine and runs a short monitor session.
 */
int main() {
    MemoryMap memory;
    memory.loadROM(monitor_rom, 0x0000, monitor_rom_size);

    LinuxConsole console;
    UART8250 uart(console, 0x00);

    Machine machine(memory, uart);
    machine.reset();
    machine.run(50000);

    std::cout << "\nPhase 1 machine run complete. PC=" << machine.getPC() << '\n';
    return 0;
}
