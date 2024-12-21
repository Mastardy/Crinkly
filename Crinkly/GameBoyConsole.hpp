#pragma once
#include <memory>
#include <string>

#include "Hardware/Bus.hpp"
#include "Hardware/Cartridge.hpp"
#include "Hardware/CPU.hpp"
#include "Utility/Utils.hpp"
#include "Utility/Types.hpp"

class GameBoyConsole
{
private: // Specifications
    inline constexpr static double CPU_FREQUENCY = 4'194'304.0;
    
    inline constexpr static Size RESOLUTION_X = 160;
    inline constexpr static Size RESOLUTION_Y = 144;

    inline constexpr static Size MAX_SPRITES = 40;
    inline constexpr static Size MAX_SPRITES_PER_LINE = 10;

public:
    GameBoyConsole();
    ~GameBoyConsole();

    GameBoyConsole(const GameBoyConsole&) = delete;
    GameBoyConsole& operator=(const GameBoyConsole&) = delete;
    GameBoyConsole(GameBoyConsole&&) = delete;
    GameBoyConsole& operator=(GameBoyConsole&&) = delete;    
    
    void InsertCartridge(const std::string& cartridge);
    void EjectCartridge();

private:
    std::shared_ptr<CPU> m_CPU;
    std::shared_ptr<Bus> m_Bus;
    std::shared_ptr<Cartridge> m_Cartridge;
};
