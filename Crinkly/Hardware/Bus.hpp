#pragma once

#include "Utility/Types.hpp"
#include "Utility/Utils.hpp"

class Bus
{
private: // Specifications
    static constexpr Size CARTRIDGE_BANK_ROM_SIZE = 16_Kb;
    static constexpr Size VIDEO_RAM_SIZE = 8_Kb;
    static constexpr Size CARTRIDGE_RAM_SIZE = 8_Kb;
    static constexpr Size WORK_RAM_SIZE = 8_Kb;

private: // Ranges
    
    
public:
    Bus();
    
private:
    std::vector<Byte> m_CartridgeROM_Bank0;
    std::vector<Byte> m_CartridgeROM_Bank1;
    std::vector<Byte> m_VideoRAM;
    std::vector<Byte> m_CartridgeRAM;
    std::vector<Byte> m_WorkRAM;
    std::vector<Byte> m_OAM;
    std::vector<Byte> m_IO_Registers;
    std::vector<Byte> m_HighRAM;
    Byte m_InterruptEnable;
};
