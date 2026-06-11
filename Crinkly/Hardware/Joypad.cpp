#include "Joypad.hpp"

Joypad::Joypad()
{
    m_Select = 0x30;
    m_DPad = 0x0F;
    m_Buttons = 0x0F;
}

void Joypad::SetRequestJoypadInterruptCallback(std::function<void()> callback)
{
    m_RequestJoypadInterrupt = std::move(callback);
}

U8 Joypad::Read() const
{
    U8 lines = 0x0F;
    if (!(m_Select & 0x10)) lines &= m_DPad;
    if (!(m_Select & 0x20)) lines &= m_Buttons;
    return static_cast<U8>(0xC0 | m_Select | lines);
}

void Joypad::Write(U8 value)
{
    m_Select = value & 0x30;
}

void Joypad::SetDPad(Button button, bool pressed)
{
    SetLine(m_DPad, !(m_Select & 0x10), button, pressed);
}

void Joypad::SetButtons(Button button, bool pressed)
{
    SetLine(m_Buttons, !(m_Select & 0x20), button, pressed);
}

void Joypad::SetLine(U8& group, bool selected, Button button, bool pressed)
{
    auto mask = static_cast<U8>(1 << static_cast<U8>(button));
    bool wasReleased = group & mask;

    if (pressed) group &= static_cast<U8>(~mask);
    else group |= mask;

    if (pressed && wasReleased && selected && m_RequestJoypadInterrupt) m_RequestJoypadInterrupt();
}
