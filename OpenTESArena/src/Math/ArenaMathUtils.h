#ifndef ARENA_MATH_UTILS_H
#define ARENA_MATH_UTILS_H

#include <cstdint>

#include "components/utilities/Span.h"

namespace ArenaMathUtils
{
	void rotatePoint(int32_t angle, int16_t &x, int16_t &y, Span<const int16_t> cosineTable);
}

#endif
