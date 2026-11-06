#include "GameBoyConsole.hpp"

#include <chrono>
#include <thread>

GameBoyConsole::GameBoyConsole()
{
    m_Timer = std::make_shared<Timer>();
    m_Joypad = std::make_shared<Joypad>();
    m_Bus = std::make_shared<Bus>(*m_Timer, *m_Joypad);
    m_CPU = std::make_shared<CPU>(*m_Bus);
    m_APU = std::make_shared<APU>(*m_Bus);

    m_LCD = std::make_shared<LCD>(*m_Bus, *m_Joypad);
    m_PPU = std::make_shared<PPU>(*m_Bus, *m_LCD);

    m_CPU->SetTickCallback([this](const U8 tCycles) { Advance(tCycles); });
}

void GameBoyConsole::Advance(const U8 tCycles)
{
    constexpr auto CLOCK_HZ = 4194304.0;
    constexpr auto SYNC_CYCLES = 70224;

    m_Timer->Tick(tCycles);
    m_PPU->Step(tCycles);
    m_APU->Step(tCycles);

    m_TotalCycles += tCycles;
    m_SinceSync += tCycles;
    if (m_SinceSync < SYNC_CYCLES) return;

    m_SinceSync = 0;
    std::this_thread::sleep_until(m_Start + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                      std::chrono::duration<double>(m_TotalCycles / CLOCK_HZ)));
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
    m_Start = std::chrono::steady_clock::now();

    while (m_Running)
    {
        Advance(m_CPU->Step());
    }
}
