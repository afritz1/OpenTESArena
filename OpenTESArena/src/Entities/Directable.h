#ifndef DIRECTABLE_H
#define DIRECTABLE_H

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"

// To be inherited by classes that have a 3D direction that they face.

// Generating a 3D frame using this direction only breaks if it is very close to
// the global up direction. To avoid that, just limit the maximum look up and down
// like usual.

class CoordinateFrame;

class Directable
{
private:
	Double3 direction;
public:
	Directable(const Double3 &direction);
	~Directable();

	// Gets the direction normal to the ground. Maybe this method should be in a
	// different class, like World or something.
	static Double3 getGlobalUp();

	const Double3 &getDirection() const;

	// Gets the top-down 2D direction.
	Double2 getGroundDirection() const;

	// Generates a 3D frame from the direction and global up. This must be paired
	// with a point in order to be relative to another coordinate system.
	CoordinateFrame getFrame() const;

	// Sets the normalized direction if it is not too close to the global up axis.
	void setDirection(const Double3 &direction);
};

#endif
