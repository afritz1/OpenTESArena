#ifndef PALETTE_H
#define PALETTE_H

#include <array>

#include "Color.h"

using Palette = std::array<Color, 256>;
using PaletteIndices = std::array<uint8_t, 256>;

#endif
