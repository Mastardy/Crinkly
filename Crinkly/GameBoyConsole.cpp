#include "GameBoyConsole.hpp"

#include <iostream>

GameBoyConsole::GameBoyConsole()
{
    m_Bus = std::make_shared<Bus>();
    m_CPU = std::make_shared<CPU>();
}

GameBoyConsole::~GameBoyConsole()
{
    EjectCartridge();
}

void GameBoyConsole::InsertCartridge(const std::string& cartridge)
{
    m_Cartridge = std::make_shared<Cartridge>(cartridge);
}

void GameBoyConsole::EjectCartridge()
{
    std::cout << "Ejecting cartridge.\n";
}