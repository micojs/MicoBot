// Auto Generated File - DO NOT EDIT!
#pragma once
#include <cstdint>

inline constexpr uint16_t colorFromRGB(uint8_t R, uint8_t G, uint8_t B){
    return (int(R>>3) << 11) + (int(G>>2) << 5) + int(B>>3);
}

#include "general.hpp"

namespace resource {
    inline auto& fontMini = ::fontMini;
    inline auto& fontTIC806x6 = ::fontTIC806x6;
    inline auto& fontZXSpec = ::fontZXSpec;
    inline auto& fontAdventurer = ::fontAdventurer;
    inline auto& fontDonut = ::fontDonut;
    inline auto& fontDragon = ::fontDragon;
    inline auto& fontC64 = ::fontC64;
    inline auto& fntC64UIGfx = ::fntC64UIGfx;
    inline auto& fontMonkey = ::fontMonkey;
    inline auto& fontKarateka = ::fontKarateka;
    inline auto& fontKoubit = ::fontKoubit;
    inline auto& fontRunes = ::fontRunes;
    inline auto& fontTight = ::fontTight;
    inline auto& fontTiny = ::fontTiny;
}
