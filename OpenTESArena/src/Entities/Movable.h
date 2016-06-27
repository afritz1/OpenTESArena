#ifndef MOVABLE_H
#define MOVABLE_H

#include "../Math/Float3.h"

// To be inherited by entities that can be moved over time.

// Pronounced "move-able".

class Movable
{
private:
	Float3d velocity;
	double maxWalkSpeed, maxRunSpeed;
public:
	Movable(const Float3d &velocity, double maxWalkSpeed, double maxRunSpeed);
	~Movable();

	const Float3d &getVelocity() const;
	double getMaxWalkSpeed() const;
	double getMaxRunSpeed() const;

	// Changes the velocity (as a force) given a normalized direction, magnitude, 
	// and delta time, as well as whether the entity is running. The direction could 
	// have had its magnitude based on its length, but this way is more explicit.
	void accelerate(const Float3d &direction, double magnitude, 
		bool isRunning, double dt);

	// Sets the velocity directly.
	void setVelocity(const Float3d &velocity);

	void setMaxWalkSpeed(double maxWalkSpeed);
	void setMaxRunSpeed(double maxRunSpeed);
};

#endif
