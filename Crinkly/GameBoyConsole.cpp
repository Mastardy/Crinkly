#include "GameBoyConsole.hpp"

#include <iostream>

GameBoyConsole::GameBoyConsole()
{
    m_Timer = std::make_shared<Timer>();
    m_Joypad = std::make_shared<Joypad>();
    m_Bus = std::make_shared<Bus>(m_Timer, m_Joypad);
    m_CPU = std::make_shared<CPU>(m_Bus);

    m_LCD = std::make_shared<LCD>(m_Bus, m_Joypad);
    m_PPU = std::make_shared<PPU>(m_Bus, m_LCD);
    m_APU = std::make_shared<APU>();

    m_CPU->SetTickCallback([this](const U8 tCycles)
    {
        m_Timer->Tick(tCycles);
        m_PPU->Step(tCycles);
    });
}

GameBoyConsole::~GameBoyConsole()
{
    EjectCartridge();
}

void GameBoyConsole::InsertCartridge(const std::string& cartridge)
{
    m_Bus->InsertCartridge(cartridge);
    m_CPU->Bootstrap();
    m_CPU->SetPC(0x100);
    this->Run();
}

void GameBoyConsole::EjectCartridge()
{
    m_Cartridge.reset();
}

void GameBoyConsole::Run()
{
    while (m_Running)
    {
        auto tCycles = m_CPU->Step();
        m_Timer->Tick(tCycles);
        m_PPU->Step(tCycles);
        m_APU->Step();
    }
}
