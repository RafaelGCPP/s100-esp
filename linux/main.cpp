#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "linux_console.hpp"
#include "machine.hpp"
#include "memory_map.hpp"
#include "uart_8250.hpp"

/**
 * Loads one binary image from disk.
 */
static bool loadBinaryImage(const std::string& path, std::vector<std::uint8_t>& out_data) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return false;
    }

    input.seekg(0, std::ios::end);
    const std::streamoff size = input.tellg();
    if (size <= 0) {
        return false;
    }
    input.seekg(0, std::ios::beg);

    out_data.resize(static_cast<std::size_t>(size));
    input.read(reinterpret_cast<char*>(out_data.data()), size);
    return static_cast<bool>(input);
}

/**
 * Boots the Phase 1 machine from assembled bootloader binary.
 */
int main() {
    MemoryMap memory;
    std::vector<std::uint8_t> bootloader;
    if (!loadBinaryImage("assembly/BOOTLOADER.BIN", bootloader) &&
        !loadBinaryImage("../assembly/BOOTLOADER.BIN", bootloader)) {
        std::cerr << "Unable to load BOOTLOADER.BIN from assembly/ or ../assembly/.\n";
        return 1;
    }
    memory.loadROM(bootloader.data(), 0x0000, bootloader.size());

    LinuxConsole console;
    UART8250 uart(console, 0x00);

    Machine machine(memory, uart);
    machine.reset();
    while (!machine.isHalted()) {
        machine.step();
    }

    std::cout << "\nPhase 1 machine run complete. PC=" << machine.getPC() << '\n';
    return 0;
}
