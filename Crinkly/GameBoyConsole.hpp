#pragma once
#include <memory>
#include <string>

#include "Hardware/APU.hpp"
#include "Hardware/Bus.hpp"
#include "Hardware/Cartridge.hpp"
#include "Hardware/CPU.hpp"
#include "Hardware/Joypad.hpp"
#include "Hardware/LCD.hpp"
#include "Hardware/PPU.hpp"
#include "Hardware/Timer.hpp"
#include "Utility/Types.hpp"

class GameBoyConsole
{
private: // Specifications
    constexpr static double CPU_FREQUENCY = 4'194'304.0;

    constexpr static Size RESOLUTION_X = 160;
    constexpr static Size RESOLUTION_Y = 144;

    constexpr static Size MAX_SPRITES = 40;
    constexpr static Size MAX_SPRITES_PER_LINE = 10;

public:
    GameBoyConsole();
    ~GameBoyConsole();

    GameBoyConsole(const GameBoyConsole&) = delete;
    GameBoyConsole& operator=(const GameBoyConsole&) = delete;
    GameBoyConsole(GameBoyConsole&&) = delete;
    GameBoyConsole& operator=(GameBoyConsole&&) = delete;

    void InsertCartridge(const std::string& cartridge);
    void EjectCartridge();

    void Run();

private:
    bool m_Running = true;

    std::shared_ptr<Timer> m_Timer;
    std::shared_ptr<Joypad> m_Joypad;
    std::shared_ptr<Bus> m_Bus;
    std::shared_ptr<CPU> m_CPU;
    std::shared_ptr<LCD> m_LCD;
    std::shared_ptr<PPU> m_PPU;
    std::shared_ptr<APU> m_APU;
    std::shared_ptr<Cartridge> m_Cartridge;
};
