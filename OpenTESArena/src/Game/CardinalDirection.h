#ifndef CARDINAL_DIRECTION_H
#define CARDINAL_DIRECTION_H

#include <string>

#include "../Math/Vector2.h"

// North, northeast, southwest, etc..

enum class CardinalDirectionName;

class CardinalDirection
{
private:
	CardinalDirection() = delete;
	~CardinalDirection() = delete;
public:
	static CardinalDirectionName getDirectionName(const Double2 &direction);
	static const std::string &toString(CardinalDirectionName directionName);
};

#endif
