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
        L,
        HL
    };
    enum class Register16 : U8
    {
        AF,
        BC,
        DE,
        HL,
        SP,
        PC,
        HLi,
        HLd
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

    void Push(U8 value);
    void Push(U16 value);
    U8 Pop();
    U16 Pop16();

    bool Condition(U8 condition);
    
    void Step();
    void Step(U16 address);

#pragma region Instructions
    // 16-bit Load Instructions
    void LoadFromStackPointer();
    void LoadImm16ToR16(Register16 reg);
    void LoadAccumulatorToR16Address(Register16 reg);

    // 8-bit Arithmetic and Logical Instructions
    void DecimalAdjustAccumulator();
    void ComplementAccumulator();
    void SetCarryFlag();
    void ComplementCarryFlag();
    
    // Rotate, Shift and Bit Instructions
    void RotateLeftCarryAccumulator();
    void RotateRightCarryAccumulator();
    void RotateLeftAccumulator();
    void RotateRightAccumulator();
    
#pragma endregion
    
private:
    std::map<Register8, U8> m_Registers;
    std::weak_ptr<Bus> m_Bus;

    std::map<U8, Register8> m_R8;
    std::map<U8, Register16> m_R16;
    std::map<U8, Register16> m_R16stk;
    std::map<U8, Register16> m_R16mem;
    
    U16 m_SP;
    U16 m_PC;
};
