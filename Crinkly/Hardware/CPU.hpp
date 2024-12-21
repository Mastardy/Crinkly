#pragma once
#include <map>

#include "Utility/Types.hpp"

#include <string>

class CPU
{
public:
    CPU();

    static void VerifyRegister(char reg);
    static void VerifyRegister(const std::string& reg);
    
    U8 Register(char reg);
    U16 Register(const std::string& reg);
    
    void Register(char reg, U8 value);
    void Register(const std::string& reg, U16 value);
    
private:
    std::map<char, U8> m_Registers;
    
    U16 m_SP;
    U16 m_PC;

};
