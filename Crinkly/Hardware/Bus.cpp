#include "Bus.hpp"

#include <iostream>

Bus::Bus()
{
    m_CartridgeROM_Bank0.resize(CARTRIDGE_BANK_ROM_SIZE);
    m_CartridgeROM_Bank1.resize(CARTRIDGE_BANK_ROM_SIZE);
    m_VideoRAM.resize(VIDEO_RAM_SIZE);
    m_CartridgeRAM.resize(CARTRIDGE_RAM_SIZE);
    m_WorkRAM.resize(WORK_RAM_SIZE);
    m_OAM.resize(0xA0); // 160 bytes (4 bytes per object)
    m_IO_Registers.resize(0x80); // 128 bytes for I/O Registers
    m_HighRAM.resize(0x7F); // 127 bytes for High RAM
    m_InterruptEnable = 0x0; // Interrupts are disabled by default
}

void Bus::InsertCartridge(const std::string& cartridge)
{
    m_Cartridge = std::make_shared<Cartridge>(cartridge);

    m_CartridgeROM_Bank0 = m_Cartridge->ReadROM(0, CARTRIDGE_BANK_ROM_SIZE);
    m_CartridgeROM_Bank1 = m_Cartridge->ReadROM(CARTRIDGE_BANK_ROM_SIZE, CARTRIDGE_BANK_ROM_SIZE);
}

Byte Bus::Read(Address address) const
{
    if (address < 0x4000) return m_CartridgeROM_Bank0[address];
    else if (address < 0x8000) return m_CartridgeROM_Bank1[address - 0x4000];
    else if (address < 0xA000) return m_VideoRAM[address - 0x8000];
    else if (address < 0xC000) return m_CartridgeRAM[address - 0xA000];
    else if (address < 0xE000) return m_WorkRAM[address - 0xC000];
    else if (address >= 0xFE00 && address < 0xFEA0) return m_OAM[address - 0xFE00];
    else if (address < 0xFEFF)
    {
        std::cerr << "Attempted to read prohibited memory address: " << address << '\n';
    }
    else if (address < 0xFF7F) return m_IO_Registers[address - 0xFF00];
    else if (address < 0xFFFF) return m_HighRAM[address - 0xFF80];
    else return m_InterruptEnable;

    return 0x0;
}

std::vector<Byte> Bus::Read(Address start, Size length) const
{
    var data = std::vector<Byte>(length);

    for (Size i = start; i < start + length; i++)
    {
        data.emplace_back(Read(static_cast<Address>(i)));
    }
    
    return data;
}

void Bus::Write(Address address, Byte value)
{
    if (address < 0x8000)
    {
        std::cerr << "Attempted to write to ROM: " << address << '\n';
    }
    else if (address < 0xA000) m_VideoRAM[address - 0x8000] = value;
    else if (address < 0xC000) m_CartridgeRAM[address - 0xA000] = value;
    else if (address < 0xE000) m_WorkRAM[address - 0xC000] = value;
    else if (address >= 0xFE00 && address < 0xFEA0) m_OAM[address - 0xFE00] = value;
    else if (address < 0xFEFF)
    {
        std::cerr << "Attempted to write to prohibited memory address: " << address << '\n';
    }
    else if (address < 0xFF7F) m_IO_Registers[address - 0xFF00] = value;
    else if (address < 0xFFFF) m_HighRAM[address - 0xFF80] = value;
    else m_InterruptEnable = value;
}
