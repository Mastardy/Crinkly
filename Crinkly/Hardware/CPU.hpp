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
        HLd,
        Imm16
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
    void Push(Register16 reg);
    U8 Pop();
    U16 Pop16();
    void Pop(Register16 reg);

    bool Condition(U8 condition);

    void Step();
    void Step(U16 address);

    template <class... Types>
    static void PrintInstruction(const std::format_string<Types...>& text, Types&&... args);

    inline static std::string ConditionLiteral(U8 cond)
    {
        if (cond == 0x0) return "NZ";
        else if (cond == 0x1) return "Z";
        else if (cond == 0x2) return "NC";
        else if (cond == 0x3) return "C";
        else return "???";
    }

    inline static std::string FlagLiteral(Flags flag)
    {
        return flag == Flags::C ? "C" : flag == Flags::H ? "H" : flag == Flags::N ? "N" : "Z";
    }

    inline static std::string RegisterLiteral(Register8 reg)
    {
        switch (reg)
        {
        case Register8::A: return "A";
        case Register8::B: return "B";
        case Register8::C: return "C";
        case Register8::D: return "D";
        case Register8::E: return "E";
        case Register8::F: return "F";
        case Register8::H: return "H";
        case Register8::L: return "L";
        case Register8::HL: return "HL";
        case Register8::Imm8: return "imm8";
        }
    }

    inline static std::string RegisterLiteral(Register16 reg)
    {
        switch (reg)
        {
        case Register16::AF: return "AF";
        case Register16::BC: return "BC";
        case Register16::DE: return "DE";
        case Register16::HL: return "HL";
        case Register16::SP: return "SP";
        case Register16::PC: return "PC";
        case Register16::HLi: return "[HL+]";
        case Register16::HLd: return "[HL-]";
        case Register16::Imm16: return "imm16";
        }
    }

#pragma region Instructions
    // 16-bit Load Instructions
    void LoadFromStackPointer();
    void LoadImm16ToR16(Register16 reg);
    void LoadAccumulatorToR16Address(Register16 reg);
    void LoadR16AddressToAccumulator(Register16 reg);
    void LoadSPOffsetToHL();
    void LoadHLToSP();

    // 8-bit Load Instructions
    void LoadImm8ToR8(Register8 reg);
    void LoadR8ToR8(Register8 left, Register8 right);
    void LoadHighToAccumulator(Register8 reg);
    void LoadHighFromAccumulator(Register8 reg);

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
    void AddSP();
    void Increment(Register16 reg);
    void Decrement(Register16 reg);

    // Rotate, Shift and Bit Instructions
    void RotateLeftCarryAccumulator();
    void RotateRightCarryAccumulator();
    void RotateLeftAccumulator();
    void RotateRightAccumulator();

    void RotateLeftCarry(Register8 reg);
    void RotateRightCarry(Register8 reg);
    void RotateLeft(Register8 reg);
    void RotateRight(Register8 reg);

    void ShiftLeft(Register8 reg);
    void ShiftRight(Register8 reg);
    void ShiftRightLogically(Register8 reg);

    void Swap(Register8);

    void Bit(U8 bitIndex, Register8 reg);
    void Reset(U8 bitIndex, Register8 reg);
    void Set(U8 bitIndex, Register8 reg);

    // Control Flow Instructions
    void Call();
    void CallConditional(U8 cond);
    void Jump(Register16 reg);
    void JumpConditional(U8 cond);
    void JumpRelative();
    void JumpRelativeConditional(U8 cond);
    void Return();
    void ReturnI();
    void ReturnConditional(U8 cond);
    void Restart(U8 vec);

    // Miscellaneous Instructions
    void Stop() const;
    void DisableInterrupts();
    void EnableInterrupts();

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
    bool m_IME;
};
