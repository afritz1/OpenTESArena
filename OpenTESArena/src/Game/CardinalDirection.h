#ifndef CARDINAL_DIRECTION_H
#define CARDINAL_DIRECTION_H

#include <string>

#include "../Math/Vector2.h"

// North, northeast, southwest, etc..

enum class CardinalDirectionName;

namespace CardinalDirection
{
	// Cardinal directions in the XZ plane (bird's eye view).
	const Double2 North(1.0, 0.0);
	const Double2 South(-1.0, 0.0);
	const Double2 East(0.0, 1.0);
	const Double2 West(0.0, -1.0);

	CardinalDirectionName getDirectionName(const Double2 &direction);
	const std::string &toString(CardinalDirectionName directionName);
}

#endif
