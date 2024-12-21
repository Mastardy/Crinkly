#include "GameBoyConsole.hpp"

#include <iostream>

GameBoyConsole::GameBoyConsole()
{
    m_Bus = std::make_shared<Bus>();
    m_CPU = std::make_shared<CPU>();

    m_CPU->Register('A', 0x01);
    m_CPU->Register('B', 0x00);
    m_CPU->Register('C', 0x13);

    std::cout << std::format("{:#04x}\n", m_CPU->Register('A'));
    std::cout << std::format("{:#04x}\n", m_CPU->Register('B'));
    std::cout << std::format("{:#04x}\n", m_CPU->Register('C'));

    m_CPU->Register("AF", 0x0F2A);
    m_CPU->Register("BC", 0xFF69);
    m_CPU->Register("SP", 0x1234);

    std::cout << std::format("{:#06x}\n", m_CPU->Register("AF"));
    std::cout << std::format("{:#04x}\n", m_CPU->Register('B'));
    std::cout << std::format("{:#04x}\n", m_CPU->Register("C"));
    std::cout << std::format("{:#06x}\n", m_CPU->Register("SP"));
}

GameBoyConsole::~GameBoyConsole()
{
    EjectCartridge();
}

void GameBoyConsole::InsertCartridge(const std::string& cartridge)
{
    // m_Cartridge = std::make_shared<Cartridge>(cartridge);
}

void GameBoyConsole::EjectCartridge()
{
    std::cout << "Ejecting cartridge.\n";
}