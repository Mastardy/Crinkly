#include "GameBoyConsole.hpp"

#include <iostream>

GameBoyConsole::GameBoyConsole()
{
    m_Bus = std::make_shared<Bus>();
    m_LCD = std::make_shared<LCD>(m_Bus);
    m_CPU = std::make_shared<CPU>(m_Bus, m_LCD);
}

GameBoyConsole::~GameBoyConsole()
{
    EjectCartridge();
}

void GameBoyConsole::InsertCartridge(const std::string& cartridge) const
{
    m_Bus->InsertCartridge(cartridge);
    m_CPU->Bootstrap();
    m_CPU->Step(m_Bus->m_Cartridge->m_ROM.size() < 0x014F ? 0 : 0x100);
}

void GameBoyConsole::EjectCartridge()
{
    m_Cartridge.reset();
}