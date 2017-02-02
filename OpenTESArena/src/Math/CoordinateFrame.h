#ifndef COORDINATE_FRAME_H
#define COORDINATE_FRAME_H

#include "Vector3.h"
#include "Matrix4.h"

// A coordinate frame is a 3D axis to help with orienting entities in the world.

class CoordinateFrame
{
private:
	Double3 forward, right, up;
public:
	CoordinateFrame(const Double3 &forward, const Double3 &right, const Double3 &up);
	~CoordinateFrame();

	const Double3 &getForward() const;
	const Double3 &getRight() const;
	const Double3 &getUp() const;

	// The point argument is to compensate for the coordinate frame not having one.
	Matrix4d toMatrix4(const Double3 &point) const;
};

#endif
