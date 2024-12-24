#include "Cartridge.hpp"

#include "Utility/Types.hpp"

#include <fstream>
#include <iostream>
#include <print>

Cartridge::Cartridge(const std::string& file)
{
    std::ifstream input(file, std::ios::binary);

    if (input.fail())
    {
        std::cerr << "Failed to open file: " << file << '\n';
        std::cin.get();
        exit(127);
    }

    int byteCounter = 0;
    while (!input.eof())
    {
        Byte byte;
        input.read(reinterpret_cast<char*>(&byte), sizeof(Byte));
        m_ROM.emplace_back(byte);

        byteCounter++;
    }

    input.close();

    LoadROM();
}

void Cartridge::LoadROM()
{
    VerifyNintendoLogo();

    for (var i = 0x134; i <= 0x142; i++)
    {
        m_Title += m_ROM[i];
    }

    m_CartridgeType = static_cast<CartridgeType>(m_ROM[0x147]);

    m_ROMSize = static_cast<ROMSize>(m_ROM[0x148]);

    if (m_CartridgeType == CartridgeType::MBC1_RAM || m_CartridgeType == CartridgeType::MBC1_RAM_BATTERY ||
        m_CartridgeType == CartridgeType::ROM_RAM || m_CartridgeType == CartridgeType::ROM_RAM_BATTERY ||
        m_CartridgeType == CartridgeType::MMM01_RAM || m_CartridgeType == CartridgeType::MMM01_RAM_BATTERY ||
        m_CartridgeType == CartridgeType::MBC3_TIMER_RAM_BATTERY || m_CartridgeType == CartridgeType::MBC3_RAM ||
        m_CartridgeType == CartridgeType::MBC3_RAM_BATTERY || m_CartridgeType == CartridgeType::HuC1_RAM_BATTERY ||
        m_CartridgeType == CartridgeType::MBC5_RAM_BATTERY || m_CartridgeType == CartridgeType::MBC5_RUMBLE_RAM ||
        m_CartridgeType == CartridgeType::MBC5_RUMBLE_RAM_BATTERY || m_CartridgeType == CartridgeType::MBC5_RAM ||
        m_CartridgeType == CartridgeType::MBC7_SENSOR_RUMBLE_RAM_BATTERY)
    {
        m_RAMSize = RAMSize::None;
    }
    else
    {
        m_RAMSize = static_cast<RAMSize>(m_ROM[0x149]);
    }

    m_DestinationCode = static_cast<DestinationCode>(m_ROM[0x14A]);

    m_OldLicenseeCode = m_ROM[0x14B];
    if (m_OldLicenseeCode == 0x33)
    {
        m_NewLicenseeCode = static_cast<Word>(m_ROM[0x144] << 8) | m_ROM[0x145];
    }

    m_MaskROMVersionNumber = m_ROM[0x14C];

    U8 calculatedChecksum = 0;
    for (U16 address = 0x0134; address <= 0x014C; address++)
    {
        calculatedChecksum = calculatedChecksum - m_ROM[address] - 1;
    }

    if (calculatedChecksum != m_ROM[0x14D])
    {
        std::cerr << "\n\nHeader Checksum is incorrect!";
        exit(1);
    }
}

void Cartridge::VerifyNintendoLogo()
{
    for (var i = 0x0104; i <= 0x0133; i++)
    {
        if (m_ROM[i] == k_NintendoLogo[i - 0x0104]) continue;
        std::cerr << std::format("Rom Nintendo Logo does not match!\n{:#06x}\t{:#06x} != {:#06x}\nBlocking Execution...\n",
            i, m_ROM[i], k_NintendoLogo[i - 0x0104]);
        exit(1);
    }
}

Byte Cartridge::ReadROM(Address address) const
{
    return m_ROM[address];
}

std::vector<Byte> Cartridge::ReadROM(Address start, Size length) const
{
    std::vector<Byte> data;
    for (Size i = start; i < start + length; i++)
    {
        data.push_back(m_ROM[i]);
    }

    return data;
}

void Cartridge::PrintHeader() const
{
    std::print("Title:\n {}", m_Title);
    std::print("\n\nCartridge Type:\n {}", GetCartridgeTypeLiteral());
    std::print("\n\nROM Size:\n {}", GetROMSizeLiteral());
    std::print("\n\nRAM Size:\n {}", GetRAMSizeLiteral());
    std::print("\n\nDestination Code:\n {}", GetDestinationCodeLiteral());
    std::print("\n\nLicensee Code:\n {}", GetLicenseeCodeLiteral());
    std::print("\n\nMask ROM Version Number:\n {}", m_MaskROMVersionNumber);
}

std::string Cartridge::GetLicenseeCodeLiteral() const
{
    if (m_OldLicenseeCode != 0x33)
    {
        switch (m_OldLicenseeCode)
        {
        case 0x00:
            return "None";
        case 0x01:
            return "Nintendo";
        case 0x08:
            return "Capcom";
        case 0x09:
            return "Hot-B";
        case 0x0A:
            return "Jaleco";
        case 0x0B:
            return "Coconuts Japan";
        case 0x0C:
            return "Elite Systems";
        case 0x13:
            return "Electronic Arts";
        case 0x18:
            return "Hudson Soft";
        case 0x19:
            return "ITC Entertainment";
        case 0x1A:
            return "Yanoman";
        case 0x1D:
            return "Japan Clary";
        case 0x1F:
            return "Virgin Games Ltd.";
        case 0x24:
            return "PCM Complete";
        case 0x25:
            return "San-X";
        case 0x28:
            return "Kemco";
        case 0x29:
            return "SETA Corporation";
        case 0x30:
            return "Infogrames";
        case 0x31:
            return "Nintendo";
        case 0x32:
            return "Bandai";
        case 0x34:
            return "Konami";
        case 0x35:
            return "HectorSoft";
        case 0x38:
            return "Capcom";
        case 0x39:
            return "Banpresto";
        case 0x3C:
            return "Entertainment Interactive";
        case 0x3E:
            return "Gremlin";
        case 0x41:
            return "Ubisoft";
        case 0x42:
            return "Atlus";
        case 0x44:
            return "Malibu Interactive";
        case 0x46:
            return "Angel";
        case 0x47:
            return "Spectrum HoloByte";
        case 0x49:
            return "Irem";
        case 0x4A:
            return "Virgin Games Ltd.";
        case 0x4D:
            return "Malibu Interactive";
        case 0x4F:
            return "U.S. Gold";
        case 0x50:
            return "Absolute";
        case 0x51:
            return "Acclaim Entertainment";
        case 0x52:
            return "Activision";
        case 0x53:
            return "Sammy USA Corporation";
        case 0x54:
            return "GameTek";
        case 0x55:
            return "Park Place";
        case 0x56:
            return "LJN";
        case 0x57:
            return "Mtchbox";
        case 0x59:
            return "Milton Bradley Company";
        case 0x5A:
            return "Mindscape";
        case 0x5B:
            return "Romstar";
        case 0x5C:
            return "Naxat Soft";
        case 0x5D:
            return "Tradewest";
        case 0x60:
            return "Titus Interactive";
        case 0x61:
            return "Virgin Games Ltd.";
        case 0x67:
            return "Ocean Software";
        case 0x69:
            return "Electronic Arts";
        case 0x6E:
            return "Elite Systems";
        case 0x6F:
            return "Electro Brain";
        case 0x70:
            return "Infogrames";
        case 0x71:
            return "Interplay Entertainment";
        case 0x72:
            return "Broderbund";
        case 0x73:
            return "Sculptured Software";
        case 0x75:
            return "The Sales Curve Ltd.";
        case 0x78:
            return "THQ";
        case 0x79:
            return "Accolade";
        case 0x7A:
            return "Triffix Entertainment";
        case 0x7C:
            return "MicroProse";
        case 0x7F:
            return "Kemco";
        case 0x80:
            return "Misawa Entertainment";
        case 0x83:
            return "LOZC G.";
        case 0x86:
            return "Tokuma Shoten";
        case 0x8B:
            return "Bullet-Proof Software";
        case 0x8C:
            return "Vic Tokai Corp.";
        case 0x8E:
            return "Ape Inc.";
        case 0x8F:
            return "I'Max";
        case 0x91:
            return "Chunsoft Co.";
        case 0x92:
            return "Video System";
        case 0x93:
            return "Tsubaraya Productions";
        case 0x95:
            return "Varie";
        case 0x96:
            return "Yonezawa/S'pal";
        case 0x97:
            return "Kemco";
        case 0x99:
            return "Arc";
        case 0x9A:
            return "Nihon Bussan";
        case 0x9B:
            return "Tecmo";
        case 0x9C:
            return "Imagineer";
        case 0x9D:
            return "Banpresto";
        case 0x9F:
            return "Nova";
        case 0xA1:
            return "Hori Electric";
        case 0xA2:
            return "Bandai";
        case 0xA4:
            return "Konami";
        case 0xA6:
            return "Kawada";
        case 0xA7:
            return "Takara";
        case 0xA9:
            return "Technos Japan";
        case 0xAA:
            return "Broderbund";
        case 0xAC:
            return "Toei Animation";
        case 0xAD:
            return "Toho";
        case 0xAF:
            return "Namco";
        case 0xB0:
            return "Acclaim Entertainment";
        case 0xB1:
            return "ASCII Corp./Nexsoft";
        case 0xB2:
            return "Bandai";
        case 0xB4:
            return "Square Enix";
        case 0xB6:
            return "HAL Laboratory";
        case 0xB7:
            return "SNK";
        case 0xB9:
            return "Pony Canyon";
        case 0xBA:
            return "Culture Brain";
        case 0xBB:
            return "Sunsoft";
        case 0xBD:
            return "Sony Imagesoft";
        case 0xBF:
            return "Sammy Corp.";
        case 0xC0:
            return "Taito";
        case 0xC2:
            return "Kemco";
        case 0xC3:
            return "Square";
        case 0xC4:
            return "Tokuma Shoten";
        case 0xC5:
            return "Data East";
        case 0xC6:
            return "Tonkin House";
        case 0xC8:
            return "Koei";
        case 0xC9:
            return "UFL";
        case 0xCA:
            return "Ultra Games";
        case 0xCB:
            return "VAP, Inc.";
        case 0xCC:
            return "Use Corp.";
        case 0xCD:
            return "Meldac";
        case 0xCE:
            return "Pony Canyon";
        case 0xCF:
            return "Angel";
        case 0xD0:
            return "Taito";
        case 0xD1:
            return "SOFEL";
        case 0xD2:
            return "Quest";
        case 0xD3:
            return "Sigma Enterprises";
        case 0xD4:
            return "Ask Kodansha Co.";
        case 0xD6:
            return "Naxat Soft";
        case 0xD7:
            return "Copya Systems";
        case 0xD9:
            return "Banpresto";
        case 0xDA:
            return "Tomy";
        case 0xDB:
            return "LJN";
        case 0xDD:
            return "Nippon Computer Systems";
        case 0xDE:
            return "Human Entertainment";
        case 0xDF:
            return "Altron";
        case 0xE0:
            return "Jaleco";
        case 0xE1:
            return "Towa Chiki";
        case 0xE2:
            return "Yutaka";
        case 0xE3:
            return "Varie";
        case 0xE5:
            return "Epoch";
        case 0xE7:
            return "Athena";
        case 0xE8:
            return "Asmik Ace Entertainment";
        case 0xE9:
            return "Natsume";
        case 0xEA:
            return "King Records";
        case 0xEB:
            return "Atlus";
        case 0xEC:
            return "Epic/Sony Records";
        case 0xEE:
            return "IGS";
        case 0xF0:
            return "A Wave";
        case 0xF3:
            return "Extreme Entertainment";
        case 0xFF:
            return "LJN";
        default:
            return "";
        }
    }
    else
    {
        std::string newLicenseeCode;
        newLicenseeCode += static_cast<char>(m_NewLicenseeCode >> 8);
        newLicenseeCode += static_cast<char>(m_NewLicenseeCode & 0xFF);

        if (newLicenseeCode == "00") return "None";
        if (newLicenseeCode == "01" || newLicenseeCode == "31") return "Nintendo";
        if (newLicenseeCode == "08") return "Capcom";
        if (newLicenseeCode == "13" || newLicenseeCode == "69") return "Electronic Arts";
        if (newLicenseeCode == "18" || newLicenseeCode == "38") return "Hudson Soft";
        if (newLicenseeCode == "19") return "B-AI";
        if (newLicenseeCode == "20") return "KSS";
        if (newLicenseeCode == "22") return "Planning Office WADA";
        if (newLicenseeCode == "24") return "PCM Complete";
        if (newLicenseeCode == "25") return "San-X";
        if (newLicenseeCode == "28") return "Kemco";
        if (newLicenseeCode == "29") return "SETA Corporation";
        if (newLicenseeCode == "30") return "Viacom";
        if (newLicenseeCode == "32") return "Bandai";
        if (newLicenseeCode == "33" || newLicenseeCode == "93") return "Ocean Software/Acclaim Entertainment";
        if (newLicenseeCode == "34" || newLicenseeCode == "54") return "Konami";
        if (newLicenseeCode == "35") return "HectorSoft";
        if (newLicenseeCode == "37") return "Taito";
        if (newLicenseeCode == "39") return "Banpresto";
        if (newLicenseeCode == "41") return "Ubi Soft";
        if (newLicenseeCode == "42") return "Atlus";
        if (newLicenseeCode == "44") return "Malibu Interactive";
        if (newLicenseeCode == "46") return "Angel";
        if (newLicenseeCode == "47") return "Bullet-Proof Software";
        if (newLicenseeCode == "49") return "IREM";
        if (newLicenseeCode == "50") return "Absolute";
        if (newLicenseeCode == "51") return "Acclaim Entertainment";
        if (newLicenseeCode == "52") return "Activision";
        if (newLicenseeCode == "53") return "Sammy USA Corporation";
        if (newLicenseeCode == "55") return "Hi Tech Expressions";
        if (newLicenseeCode == "56") return "LJN";
        if (newLicenseeCode == "57") return "Matchbox";
        if (newLicenseeCode == "58") return "Mattel";
        if (newLicenseeCode == "59") return "Milton Bradley Company";
        if (newLicenseeCode == "60") return "Titus Interactive";
        if (newLicenseeCode == "61") return "Virgin Games Ltd.";
        if (newLicenseeCode == "64") return "Lucasfilm Games";
        if (newLicenseeCode == "67") return "Ocean Software";
        if (newLicenseeCode == "70") return "Infogrames";
        if (newLicenseeCode == "71") return "Interplay Entertainment";
        if (newLicenseeCode == "72") return "Broderbund";
        if (newLicenseeCode == "73") return "Sculptured Software";
        if (newLicenseeCode == "75") return "The Sales Curve Ltd.";
        if (newLicenseeCode == "78") return "THQ";
        if (newLicenseeCode == "79") return "Accolade";
        if (newLicenseeCode == "80") return "Misawa Entertainment";
        if (newLicenseeCode == "83") return "LOZC";
        if (newLicenseeCode == "86") return "Tokuma Shoten";
        if (newLicenseeCode == "87") return "Tsukuda Original";
        if (newLicenseeCode == "91") return "Chunsoft Co.";
        if (newLicenseeCode == "92") return "Video System";
        if (newLicenseeCode == "95") return "Varie";
        if (newLicenseeCode == "96") return "Yonezawa/S'pal";
        if (newLicenseeCode == "97") return "Kaneko";
        if (newLicenseeCode == "99") return "Pack-In-Video";
        if (newLicenseeCode == "9H") return "Bottom Up";
        if (newLicenseeCode == "A4") return "Konami (Yu-Gi-Oh!)";
        if (newLicenseeCode == "BL") return "MTO";
        if (newLicenseeCode == "DK") return "Kodansha";
    }

    return "Unknown";
}

std::string Cartridge::GetCartridgeTypeLiteral() const
{
    switch (m_CartridgeType)
    {
    case CartridgeType::ROM_ONLY:
        return "ROM ONLY";
    case CartridgeType::MBC1:
        return "MBC1";
    case CartridgeType::MBC1_RAM:
        return "MBC1+RAM";
    case CartridgeType::MBC1_RAM_BATTERY:
        return "MBC1+RAM+BATTERY";
    case CartridgeType::MBC2:
        return "MBC2";
    case CartridgeType::MBC2_BATTERY:
        return "MBC2+BATTERY";
    case CartridgeType::ROM_RAM:
        return "ROM+RAM";
    case CartridgeType::ROM_RAM_BATTERY:
        return "ROM+RAM+BATTERY";
    case CartridgeType::MMM01:
        return "MMM01";
    case CartridgeType::MMM01_RAM:
        return "MMM01+RAM";
    case CartridgeType::MMM01_RAM_BATTERY:
        return "MMM01+RAM+BATTERY";
    case CartridgeType::MBC3_TIMER_BATTERY:
        return "MBC3+TIMER+BATTERY";
    case CartridgeType::MBC3_TIMER_RAM_BATTERY:
        return "MBC3+TIMER+RAM+BATTERY";
    case CartridgeType::MBC3:
        return "MBC3";
    case CartridgeType::MBC3_RAM:
        return "MBC3+RAM";
    case CartridgeType::MBC3_RAM_BATTERY:
        return "MBC3+RAM+BATTERY";
    case CartridgeType::MBC5:
        return "MBC5";
    case CartridgeType::MBC5_RAM:
        return "MBC5+RAM";
    case CartridgeType::MBC5_RAM_BATTERY:
        return "MBC5+RAM+BATTERY";
    case CartridgeType::MBC5_RUMBLE:
        return "MBC5+RUMBLE";
    case CartridgeType::MBC5_RUMBLE_RAM:
        return "MBC5+RUMBLE+RAM";
    case CartridgeType::MBC5_RUMBLE_RAM_BATTERY:
        return "MBC5+RUMBLE+RAM+BATTERY";
    case CartridgeType::MBC6:
        return "MBC6";
    case CartridgeType::MBC7_SENSOR_RUMBLE_RAM_BATTERY:
        return "MBC7+SENSOR+RUMBLE+RAM+BATTERY";
    case CartridgeType::POCKET_CAMERA:
        return "POCKET CAMERA";
    case CartridgeType::BANDAI_TAMA5:
        return "BANDAI TAMA5";
    case CartridgeType::HuC3:
        return "HuC3";
    case CartridgeType::HuC1_RAM_BATTERY:
        return "HuC1+RAM+BATTERY";
    }

    return "Unknown";
}

std::string Cartridge::GetROMSizeLiteral() const
{
    switch (m_ROMSize)
    {
    case ROMSize::_32KB:
        return "32KB";
    case ROMSize::_64KB:
        return "64KB";
    case ROMSize::_128KB:
        return "128KB";
    case ROMSize::_256KB:
        return "256KB";
    case ROMSize::_512KB:
        return "512KB";
    case ROMSize::_1MB:
        return "1MB";
    case ROMSize::_2MB:
        return "2MB";
    case ROMSize::_4MB:
        return "4MB";
    case ROMSize::_8MB:
        return "8MB";
    case ROMSize::_1_1MB:
        return "1.1MB";
    case ROMSize::_1_2MB:
        return "1.2MB";
    case ROMSize::_1_5MB:
        return "1.5MB";
    }

    return "Unknown";
}

std::string Cartridge::GetRAMSizeLiteral() const
{
    switch (m_RAMSize)
    {
    case RAMSize::None:
        return "None";
    case RAMSize::Unused:
        return "Unused";
    case RAMSize::_8KB:
        return "8KB";
    case RAMSize::_32KB:
        return "32KB";
    case RAMSize::_128KB:
        return "128KB";
    case RAMSize::_64KB:
        return "64KB";
    }

    return "Unknown";
}

std::string Cartridge::GetDestinationCodeLiteral() const
{
    return m_DestinationCode == DestinationCode::Japan ? "Japan" : "Overseas";
}
