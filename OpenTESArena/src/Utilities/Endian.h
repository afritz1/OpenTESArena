#ifndef ENDIAN_H
#define ENDIAN_H

#include "SDL_endian.h"

namespace Endian
{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	constexpr int RGBA_RedShift = 0;
	constexpr int RGBA_GreenShift = 8;
	constexpr int RGBA_BlueShift = 16;
	constexpr int RGBA_AlphaShift = 24;
#else
	constexpr int RGBA_RedShift = 24;
	constexpr int RGBA_GreenShift = 16;
	constexpr int RGBA_BlueShift = 8;
	constexpr int RGBA_AlphaShift = 0;
#endif
}

#endif
