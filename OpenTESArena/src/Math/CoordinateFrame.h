#ifndef COORDINATE_FRAME_H
#define COORDINATE_FRAME_H

#include "Float3.h"
#include "Matrix4.h"

// A coordinate frame is a 3D axis to help with orienting entities in the world.

class CoordinateFrame
{
private:
	Float3d forward, right, up;
public:
	CoordinateFrame(const Float3d &forward, const Float3d &right, const Float3d &up);
	~CoordinateFrame();

	const Float3d &getForward() const;
	const Float3d &getRight() const;
	const Float3d &getUp() const;

	// The point argument is to compensate for the coordinate frame not having one.
	Matrix4d toMatrix4(const Float3d &point) const;
};

#endif
