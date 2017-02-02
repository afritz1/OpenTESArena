#ifndef MOVABLE_H
#define MOVABLE_H

#include "../Math/Vector3.h"

// To be inherited by entities that can be moved over time.

// Pronounced "move-able".

class Movable
{
private:
	Double3 velocity;
	double maxWalkSpeed, maxRunSpeed;
public:
	Movable(const Double3 &velocity, double maxWalkSpeed, double maxRunSpeed);
	~Movable();

	const Double3 &getVelocity() const;
	double getMaxWalkSpeed() const;
	double getMaxRunSpeed() const;

	// Changes the velocity (as a force) given a normalized direction, magnitude, 
	// and delta time, as well as whether the entity is running. The direction could 
	// have had its magnitude based on its length, but this way is more explicit.
	void accelerate(const Double3 &direction, double magnitude, 
		bool isRunning, double dt);

	// Sets the velocity directly.
	void setVelocity(const Double3 &velocity);

	void setMaxWalkSpeed(double maxWalkSpeed);
	void setMaxRunSpeed(double maxRunSpeed);
};

#endif
