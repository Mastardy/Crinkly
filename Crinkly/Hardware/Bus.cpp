#include "Bus.hpp"

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
