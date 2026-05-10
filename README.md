# Crinkly

A Nintendo Game Boy (DMG) emulator written in modern C++, rendered with OpenGL.

Crinkly aims to be a clean, readable reference implementation of the original 1989 Game Boy hardware — the Sharp LR35902 CPU, memory bus, timers, and PPU — wrapped in a small, dependency-light desktop frontend.

## Features

- Sharp LR35902 (modified Z80) CPU core
- Memory bus and cartridge loading
- Hardware timers and interrupts
- PPU with background, window, and OAM (sprite) rendering
- Tile map and tile data viewers
- OpenGL 3.3 rendering via GLAD + GLFW
- Native resolution: 160 × 144 @ 4.194304 MHz

## Project Structure

```
Crinkly/
├── Crinkly.cpp              # Entry point
├── GameBoyConsole.{hpp,cpp} # Top-level system
├── Hardware/                # CPU, Bus, Cartridge, LCD/PPU
└── Utility/                 # Shared types and helpers
ThirdParty/
├── GLAD/                    # OpenGL loader
└── GLFW/                    # Windowing & input
Roms/                        # Test ROMs
```

## Building

Crinkly is developed on Windows with Visual Studio 2026.

1. Clone the repository.
2. Open `Crinkly.sln` in Visual Studio 2026. 
    1. If you want to use Visual Studio 2022, just change the PlatformToolset from v145 to v143.
3. Select the `x64` configuration.
4. Build and run.

GLFW and GLAD are vendored under `ThirdParty/`, so no external package manager setup is required.

## Usage

```
Crinkly.exe <path-to-rom.gb>
```

Example:

```
Crinkly.exe Roms/cpu_instrs.gb
```

## Status

Crinkly is a work in progress. The CPU passes Blargg's `cpu_instrs` test ROM and the PPU renders background, window, and basic OAM. Audio (APU), MBC mappers beyond the basics, and serial link are not yet implemented.

## License

Released under the [MIT License](LICENSE.md).
