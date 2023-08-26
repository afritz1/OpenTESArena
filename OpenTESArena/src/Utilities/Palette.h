#ifndef PALETTE_H
#define PALETTE_H

#include <array>

#include "Color.h"

static constexpr int PaletteLength = 256;
using Palette = std::array<Color, PaletteLength>;
using PaletteIndices = std::array<uint8_t, PaletteLength>;

#endif
