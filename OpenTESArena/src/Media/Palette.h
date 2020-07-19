#ifndef PALETTE_H
#define PALETTE_H

#include <array>

#include "Color.h"

constexpr int PALETTE_SIZE = 256;
using Palette = std::array<Color, PALETTE_SIZE>;

#endif
