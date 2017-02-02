#ifndef CARDINAL_DIRECTION_H
#define CARDINAL_DIRECTION_H

#include <string>

#include "../Math/Vector2.h"

// North, northeast, southwest, etc..

enum class CardinalDirectionName;

class CardinalDirection
{
private:
	CardinalDirectionName directionName;
public:
	CardinalDirection(const Double2 &direction);
	~CardinalDirection();

	CardinalDirectionName getDirectionName() const;
	std::string toString() const;
};

#endif
