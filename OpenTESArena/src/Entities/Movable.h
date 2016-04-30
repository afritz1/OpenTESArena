#ifndef MOVABLE_H
#define MOVABLE_H

#include "../Math/Float3.h"

// To be inherited by entities that can be moved over time.

// Pronounced "move-able".

class Movable
{
private:
	Float3d velocity;
public:
	Movable(const Float3d &velocity);
	~Movable();

	const Float3d &getVelocity() const;

	void setVelocity(const Float3d &velocity);
};

#endif
