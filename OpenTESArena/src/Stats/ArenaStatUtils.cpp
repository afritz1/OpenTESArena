#include <cmath>

#include "ArenaStatUtils.h"

int ArenaStatUtils::scale256To100(int value)
{
	const double scaledValue = (static_cast<double>(value) * 100.0) / 256.0;
	return static_cast<int>(std::round(scaledValue));
}

int ArenaStatUtils::scale100To256(int value)
{
	return (value * 256) / 100;
}
