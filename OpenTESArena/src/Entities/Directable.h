#ifndef DIRECTABLE_H
#define DIRECTABLE_H

#include "../Math/Float2.h"
#include "../Math/Float3.h"

// To be inherited by classes that have a 3D direction that they face.

// Generating a 3D frame using this direction only breaks if it is very close to
// the global up direction. To avoid that, just limit the maximum look up and down
// like usual.

class CoordinateFrame;

class Directable
{
private:
	Float3d direction;
public:
	Directable(const Float3d &direction);
	~Directable();

	// Gets the direction normal to the ground. Maybe this method should be in a
	// different class, like World or something.
	static Float3d getGlobalUp();

	const Float3d &getDirection() const;

	// Gets the top-down 2D direction.
	Float2d getGroundDirection() const;

	// Generates a 3D frame from the direction and global up. This must be paired
	// with a point in order to be relative to another coordinate system.
	CoordinateFrame getFrame() const;

	// Sets the normalized direction if it is not too close to the global up axis.
	void setDirection(const Float3d &direction);
};

#endif
