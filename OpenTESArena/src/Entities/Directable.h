#ifndef DIRECTABLE_H
#define DIRECTABLE_H

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

	// Get the direction normal to the ground.
	static Float3d getGlobalUp();

	const Float3d &getDirection() const;
	CoordinateFrame getFrame() const;

	void setDirection(const Float3d &direction);
};

#endif
