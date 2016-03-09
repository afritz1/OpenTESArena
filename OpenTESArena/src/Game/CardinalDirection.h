#ifndef CARDINAL_DIRECTION_H
#define CARDINAL_DIRECTION_H

#include <map>
#include <string>

#include "CardinalDirectionName.h"
#include "../Math/Vector2.h"

class CardinalDirection
{
private:
	CardinalDirectionName directionName;
public:
	CardinalDirection(const Vector2d &direction);
	~CardinalDirection();

	const CardinalDirectionName &getDirectionName() const;
	std::string toString() const;
};

#endif