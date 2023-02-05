// Auto Generated File - DO NOT EDIT!
#pragma once
#include <cstdint>

inline constexpr uint16_t colorFromRGB(uint8_t R, uint8_t G, uint8_t B){
    return (int(R>>3) << 11) + (int(G>>2) << 5) + int(B>>3);
}

#include "general.hpp"
