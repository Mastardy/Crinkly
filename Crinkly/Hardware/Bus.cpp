#include "Bus.hpp"

#include <print>

Bus::Bus(Timer& timer, Joypad& joypad) : m_Timer(timer), m_Joypad(joypad)
{
    m_VideoRAM.resize(VIDEO_RAM_SIZE);
    m_CartridgeRAM.resize(CARTRIDGE_RAM_SIZE);
    m_WorkRAM.resize(WORK_RAM_SIZE);
    m_OAM.resize(0xA0); // 160 bytes (4 bytes per object)
    m_IO_Registers.resize(0x80); // 128 bytes for I/O Registers
    m_HighRAM.resize(0x7F); // 127 bytes for High RAM
    m_InterruptEnable = 0x0; // Interrupts are disabled by default

    timer.SetRequestTimerInterruptCallback([this]()
    {
        this->Write(0xFF0F, this->Read(0xFF0F) | (1 << 2));
    });

    joypad.SetRequestJoypadInterruptCallback([this]()
    {
        this->Write(0xFF0F, this->Read(0xFF0F) | (1 << 4));
    });
}

void Bus::InsertCartridge(const std::string& cartridge)
{
    m_CartridgeName = SplitString(SplitString(cartridge, "\\").back(), ".")[0];

    m_Cartridge = std::make_shared<Cartridge>(cartridge);
}

static constexpr Byte APU_READ_MASKS[0x20] = {
    0x80, 0x3F, 0x00, 0xFF, 0xBF, // NR10-NR14
    0xFF, 0x3F, 0x00, 0xFF, 0xBF, // NR21-NR24
    0x7F, 0xFF, 0x9F, 0xFF, 0xBF, // NR30-NR34
    0xFF, 0xFF, 0x00, 0x00, 0xBF, // NR41-NR44
    0x00, 0x00, 0x70, // NR50-NR52
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // FF27-FF2B 
    0xFF, 0xFF, 0xFF, 0xFF // FF2C-FF2F
};

Byte Bus::Read(Address address)
{
    const U8 mode = m_IO_Registers[0x41] & 0x03;
    if (address >= 0x8000 && address < 0xA000 && mode == 3) return 0xFF;
    if (address >= 0xFE00 && address < 0xFEA0 && mode >= 2) return 0xFF;
    if (address >= 0xFF10 && address < 0xFF30) return ReadRaw(address) | APU_READ_MASKS[address - 0xFF10];
    if (address >= 0xFF30 && address < 0xFF40 && m_WaveRamAccess)
    {
        const int index = m_WaveRamAccess();
        if (index == WAVE_RAM_BLOCKED) return 0xFF;
        if (index >= 0) address = static_cast<Address>(0xFF30 + index);
    }
    return ReadRaw(address) | (address == 0xFF0F ? 0xE0 : 0);
}

void Bus::Write(Address address, Byte value)
{
    const U8 mode = m_IO_Registers[0x41] & 0x03;
    if (address == 0xFF44) return;
    if (address >= 0x8000 && address < 0xA000 && mode == 3) return;
    if (address >= 0xFE00 && address < 0xFEA0 && mode >= 2) return;
    if (address >= 0xFF10 && address <= 0xFF25 && !(m_IO_Registers[0x26] & 0x80))
    {
        const bool isLength = address == 0xFF11 || address == 0xFF16 || address == 0xFF1B || address == 0xFF20;
        if (isLength && m_OnWrite) m_OnWrite(address, value);
        return;
    }
    if (address >= 0xFF30 && address < 0xFF40 && m_WaveRamAccess)
    {
        const int index = m_WaveRamAccess();
        if (index == WAVE_RAM_BLOCKED) return;
        if (index >= 0) address = static_cast<Address>(0xFF30 + index);
    }
    if (address == 0xFF26) value = (m_IO_Registers[0x26] & 0x0F) | (value & 0x80);
    if (address == 0xFF41) value = (m_IO_Registers[0x41] & 0x07) | (value & 0x78);
    WriteRaw(address, value);
    if (m_OnWrite) m_OnWrite(address, value);
}

Byte Bus::ReadRaw(Address address)
{
    if (address < 0x8000) return m_Cartridge->Read(address);
    else if (address < 0xA000) return m_VideoRAM[address - 0x8000];
    else if (address < 0xC000) return m_Cartridge->Read(address);
    else if (address < 0xE000) return m_WorkRAM[address - 0xC000];
    else if (address >= 0xFE00 && address < 0xFEA0) return m_OAM[address - 0xFE00];
    else if (address < 0xFF00) return 0xFF;
    else if (address == 0xFF00) return m_Joypad.Read();
    else if (address >= 0xFF04 && address < 0xFF08) return m_Timer.Read(address);
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

void Bus::WriteRaw(Address address, Byte value)
{
    if (address == 0xFF41)
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
            m_OAM[i] = ReadRaw(static_cast<Address>(source + i));
        }

        return;
    }

    if (address < 0x8000)
    {
        m_Cartridge->Write(address, value);
    }
    else if (address < 0xA000) m_VideoRAM[address - 0x8000] = value;
    else if (address < 0xC000) m_Cartridge->Write(address, value);
    else if (address < 0xE000) m_WorkRAM[address - 0xC000] = value;
    else if (address >= 0xFE00 && address < 0xFEA0) m_OAM[address - 0xFE00] = value;
    else if (address < 0xFF00)
    {
    }
    else if (address == 0xFF00) m_Joypad.Write(value);
    else if (address >= 0xFF04 && address < 0xFF08) m_Timer.Write(address, value);
    else if (address < 0xFF80) m_IO_Registers[address - 0xFF00] = value;
    else if (address < 0xFFFF) m_HighRAM[address - 0xFF80] = value;
    else m_InterruptEnable = value;
}
