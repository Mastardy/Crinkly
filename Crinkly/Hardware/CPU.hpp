#pragma once
#include <map>

#include "Utility/Types.hpp"

#include <string>

class CPU
{
public:
    enum class Register8 : U8
    {
        A,
        B,
        C,
        D,
        E,
        F,
        H,
        L
    };

    enum class Register16 : U8
    {
        AF,
        BC,
        DE,
        HL,
        SP,
        PC
    };
    
public:
    CPU();

    U8 Register(Register8 reg);
    U16 Register(Register16 reg);
    
    void Register(Register8 reg, U8 value);
    void Register(Register16 reg, const U16& value);
    
private:
    std::map<Register8, U8> m_Registers;
    
    U16 m_SP;
    U16 m_PC;

};
