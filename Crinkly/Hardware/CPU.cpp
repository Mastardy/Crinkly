#include "CPU.hpp"

#include <iostream>

CPU::CPU()
{
    m_Registers['A'] = 0;
    m_Registers['B'] = 0;
    m_Registers['C'] = 0;
    m_Registers['D'] = 0;
    m_Registers['E'] = 0;
    m_Registers['F'] = 0;
    m_Registers['H'] = 0;
    m_Registers['L'] = 0;
    
    m_SP = 0x0000;
    m_PC = 0x0000;
}

void CPU::VerifyRegister(char reg)
{
    if (reg == 'A' || reg == 'B' || reg == 'C' || reg == 'D' || reg == 'E' || reg == 'F' || reg == 'H' || reg == 'L')
        return;
    std::cerr << "Invalid register: " << reg << '\n';
    std::exit(1);
}

void CPU::VerifyRegister(const std::string& reg)
{
    if (reg == "AF" || reg == "BC" || reg == "DE" || reg == "HL" || reg == "SP" || reg == "PC")
        return;
    std::cerr << "Invalid register: " << reg << '\n';
    std::exit(1);
}

U8 CPU::Register(char reg)
{
    VerifyRegister(reg);
    return m_Registers[reg];
}

U16 CPU::Register(const std::string& reg)
{
    if (reg.size() == 1) return Register(reg[0]);
    VerifyRegister(reg);
    if (reg == "SP") return m_SP;
    if (reg == "PC") return m_PC;
    return static_cast<U16>(m_Registers[reg[0]] << 8) | m_Registers[reg[1]];    
}

void CPU::Register(char reg, U8 value)
{
    VerifyRegister(reg);
    m_Registers[reg] = value;
}

void CPU::Register(const std::string& reg, U16 value)
{
    if (reg.size() == 1)
    {
        Register(reg[0], static_cast<U8>(value | 0xFF));
        return;
    }
    
    VerifyRegister(reg);
    
    if (reg == "SP") m_SP = value;
    else if (reg == "PC") m_PC = value;
    else
    {
        m_Registers[reg[0]] = value >> 8;
        m_Registers[reg[1]] = value & 0xFF;
    }
}