#include "Cartridge.hpp"

#include "Utility/Types.hpp"

#include <fstream>
#include <iostream>

Cartridge::Cartridge(const std::string& file)
{
    std::ifstream input(file, std::ios::binary);

    if (input.fail())
    {
        std::cerr << "Failed to open file: " << file << '\n';
        std::cin.get();
        exit(127);
    }

    int byteCounter = 0;
    while (!input.eof())
    {
        Byte byte;
        input.read(reinterpret_cast<char*>(&byte), sizeof(Byte));
        m_ROM.emplace_back(byte);

        byteCounter++;
    }

    std::cout << "Entry Point:\n";
    for (var i = 0x100; i <= 0x103; i++)
    {
        std::cout << std::format(" {:#04x}", static_cast<int>(m_ROM[i]));
    }
    std::cout << "\n\nTitle:\n";
    for (var i = 0x134; i <= 0x142; i++)
    {
        std::cout << std::format(" {:#04x}", static_cast<int>(m_ROM[i]));
    }
    std::cout << '\n';
    for (var i = 0x134; i <= 0x142; i++)
    {
        std::cout << "    " << m_ROM[i];
    }
    std::cout << "\n\nManufacturer Code:\n";
    for (var i = 0x13F; i <= 0x142; i++)
    {
        std::cout << std::format(" {:#04x}", static_cast<int>(m_ROM[i]));
    }
    std::cout << "\n\nCartridge Type:\n";
    std::cout << std::format(" {:#04x}", static_cast<int>(m_ROM[0x147]));
    std::cout << "\n\nROM Size:\n";
    std::cout << std::format(" {:#04x}", static_cast<int>(m_ROM[0x148]));
    std::cout << "\n\nRAM Size:\n";
    std::cout << std::format(" {:#04x}", static_cast<int>(m_ROM[0x149]));
    std::cout << "\n\nDestination Code:\n";
    std::cout << std::format(" {:#04x}", static_cast<int>(m_ROM[0x14a]));
    std::cout << "\n\nOld Licensee Code:\n";
    std::cout << std::format(" {:#04x}", static_cast<int>(m_ROM[0x14b]));
    std::cout << "\n\nMask ROM Version Number:\n";
    std::cout << std::format(" {:#04x}", static_cast<int>(m_ROM[0x14c]));
    std::cout << "\n\nHeader Checksum:\n";
    std::cout << std::format(" {:#04x}", static_cast<int>(m_ROM[0x14d]));
    U8 calculatedChecksum = 0;
    for(U16 address = 0x0134; address <= 0x014C; address++)
    {
        calculatedChecksum = calculatedChecksum - m_ROM[address] - 1;
    }
    std::cout << std::format(" {:#04x}", calculatedChecksum);
    std::cout << "\n\nGlobal Checksum:\n";
    for (var i = 0x14E; i <= 0x14F; i++)
    {
        std::cout << std::format(" {:#04x}", static_cast<int>(m_ROM[i]));
    }
    std::cout << '\n';

    input.close();
}
