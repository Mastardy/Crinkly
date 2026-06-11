#include "Bus.hpp"

#include <iostream>
#include <print>

Bus::Bus(const std::shared_ptr<Timer>& timer, const std::shared_ptr<Joypad>& joypad)
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

    m_Timer = timer;
    timer->SetRequestTimerInterruptCallback([this]()
    {
        this->Write(0xFF0F, this->Read(0xFF0F) | (1 << 2));
    });

    m_Joypad = joypad;
    joypad->SetRequestJoypadInterruptCallback([this]()
    {
        this->Write(0xFF0F, this->Read(0xFF0F) | (1 << 4));
    });
}

void Bus::InsertCartridge(const std::string& cartridge)
{
    m_CartridgeName = SplitString(SplitString(cartridge, "\\").back(), ".")[0];

    m_Cartridge = std::make_shared<Cartridge>(cartridge);

    m_CartridgeROM_Bank0 = m_Cartridge->ReadROM(0, CARTRIDGE_BANK_ROM_SIZE);
    m_CartridgeROM_Bank1 = m_Cartridge->ReadROM(CARTRIDGE_BANK_ROM_SIZE, CARTRIDGE_BANK_ROM_SIZE);
}

Byte Bus::Read(Address address)
{
    if (address < 0x4000) return m_CartridgeROM_Bank0[address];
    else if (address < 0x8000) return m_CartridgeROM_Bank1[address - 0x4000];
    else if (address < 0xA000) return m_VideoRAM[address - 0x8000];
    else if (address < 0xC000) return m_CartridgeRAM[address - 0xA000];
    else if (address < 0xE000) return m_WorkRAM[address - 0xC000];
    else if (address >= 0xFE00 && address < 0xFEA0) return m_OAM[address - 0xFE00];
    else if (address < 0xFF00)
    {
        // 0xFEA0–0xFEFF: unusable region. Reads return 0xFF on DMG hardware.
        return 0xFF;
    }
    else if (address == 0xFF00)
    {
        if (const auto joypad = m_Joypad.lock()) return joypad->Read();
        return 0xFF;
    }
    else if (address >= 0xFF04 && address < 0xFF08)
    {
        if (const auto timer = m_Timer.lock()) return timer->Read(address);
        return 0xFF;
    }
    else if (address < 0xFF80) return m_IO_Registers[address - 0xFF00];
    else if (address < 0xFFFF) return m_HighRAM[address - 0xFF80];
    else return m_InterruptEnable;
}

std::vector<Byte> Bus::Read(Address start, Size length)
{
    auto data = std::vector<Byte>(length);

    for (Size i = 0; i < length; i++)
    {
        data[i] = Read(static_cast<Address>(start + i));
    }

    return data;
}

void Bus::Write(Address address, Byte value)
{
    if (address == 0xFF41) // STAT Register
    {
        m_IO_Registers[0x41] = value | 0x80;
        return;
    }

    if (address == 0xFF46) // DMA Transfer
    {
        m_IO_Registers[0x46] = value;

        auto source = static_cast<Address>(value << 8);
        for (U16 i = 0; i < 0xA0; i++)
        {
            m_OAM[i] = Read(static_cast<Address>(source + i));
        }

        return;
    }

    if (address < 0x8000)
    {
        // MBC register writes
    }
    else if (address < 0xA000)
    {
        U8 flag = m_IO_Registers[0x41] & 0x03;
        if (flag != 0x03) m_VideoRAM[address - 0x8000] = value;
    }
    else if (address < 0xC000) m_CartridgeRAM[address - 0xA000] = value;
    else if (address < 0xE000) m_WorkRAM[address - 0xC000] = value;
    else if (address >= 0xFE00 && address < 0xFEA0)
    {
        U8 flag = m_IO_Registers[0x41] & 0x03;
        if (flag == 0x00 || flag == 0x01) m_OAM[address - 0xFE00] = value;
    }
    else if (address < 0xFF00)
    {
        // unusable region, therefore ignored
    }
    else if (address == 0xFF00)
    {
        if (const auto joypad = m_Joypad.lock()) joypad->Write(value);  
    }
    else if (address >= 0xFF04 && address < 0xFF08)
    {
        if (const auto timer = m_Timer.lock()) timer->Write(address, value);
    }
    else if (address < 0xFF80) m_IO_Registers[address - 0xFF00] = value;
    else if (address < 0xFFFF) m_HighRAM[address - 0xFF80] = value;
    else m_InterruptEnable = value;
}
