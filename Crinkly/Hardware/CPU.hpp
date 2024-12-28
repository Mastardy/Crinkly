#pragma once
#include <format>
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
        HL,
        Imm8
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

    U8 ReadImm8();
    U16 ReadImm16();
    
    void Push(U8 value);
    void Push(U16 value);
    U8 Pop();
    U16 Pop16();

    bool Condition(U8 condition);
    
    void Step();
    void Step(U16 address);

    template<class... Types>
    static void PrintInstruction(const std::format_string<Types...>& text, Types&&... args);
    
#pragma region Instructions
    // 16-bit Load Instructions
    void LoadFromStackPointer();
    void LoadImm16ToR16(Register16 reg);
    void LoadAccumulatorToR16Address(Register16 reg);
    void LoadR16AddressToAccumulator(Register16 reg);

    // 8-bit Load Instructions
    void LoadImm8ToR8(Register8 reg);
    void LoadR8ToR8(Register8 left, Register8 right);
    
    // 8-bit Arithmetic and Logical Instructions
    void Add(Register8 reg);
    void Adc(Register8 reg);
    void Sub(Register8 reg);
    void Sbc(Register8 reg);
    void And(Register8 reg);
    void Xor(Register8 reg);
    void Or(Register8 reg);
    void Cp(Register8 reg);
    void Increment(Register8 reg);
    void Decrement(Register8 reg);
    void DecimalAdjustAccumulator();
    void ComplementAccumulator();
    void SetCarryFlag();
    void ComplementCarryFlag();
    
    // 16-bit Arithmetic
    void Add(Register16 reg);
    void Increment(Register16 reg);
    void Decrement(Register16 reg);
    
    // Rotate, Shift and Bit Instructions
    void RotateLeftCarryAccumulator();
    void RotateRightCarryAccumulator();
    void RotateLeftAccumulator();
    void RotateRightAccumulator();

    // Control Flow Instructions
    void JumpRelative();
    void JumpRelativeConditional(U8 cond);
    
    // Miscellaneous Instructions
    void Stop() const;
    
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
