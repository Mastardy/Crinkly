#include "GameBoyConsole.hpp"

#include <iostream>

GameBoyConsole::GameBoyConsole()
{
    m_Bus = std::make_shared<Bus>();
    m_CPU = std::make_shared<CPU>(m_Bus);
}

GameBoyConsole::~GameBoyConsole()
{
    EjectCartridge();
}

void GameBoyConsole::InsertCartridge(const std::string& cartridge) const
{
    m_Bus->InsertCartridge(cartridge);
    m_CPU->Step(0x100);
}

void GameBoyConsole::EjectCartridge()
{
    m_Cartridge.reset();
}