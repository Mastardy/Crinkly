#pragma once
#include <map>
#include <memory>

#include "Bus.hpp"
#include "Utility/Types.hpp"

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
    enum class Flags : U8
    {
        Z = 0x80,
        N = 0x40,
        H = 0x20,
        C = 0x10
    };
    
public:
    CPU(const std::shared_ptr<Bus>& bus);

    U8 Register(Register8 reg);
    U16 Register(Register16 reg);
    
    void Register(Register8 reg, U8 value);
    void Register(Register16 reg, const U16& value);

    void Flag(Flags flag, bool set);
    bool Flag(Flags flag);

    void Step();
    
private:
    std::map<Register8, U8> m_Registers;
    std::weak_ptr<Bus> m_Bus;
    
    U16 m_SP;
    U16 m_PC;
};
