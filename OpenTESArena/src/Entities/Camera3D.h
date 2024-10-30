#ifndef CAMERA_3D_H
#define CAMERA_3D_H

#include "../Math/MathUtils.h"
#include "../Math/Matrix4.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Voxels/VoxelUtils.h"

// A camera for the player. Make sure not to look directly up or down, as
// that breaks the vector cross product used for determining the camera's axes.

// I purposefully decided not to have this class own the field of view or 
// the aspect ratio because 1) that's just more state to take care of, and
// 2) they're application-level variables that the user may change frequently,
// so they do just fine in an "Options" object instead.

struct Camera3D
{
	CoordDouble3 position;
	Double3 forward;
	Double3 right;
	Double3 up;
	// @todo: polar coordinates (XYZ angles)

	void init(const CoordDouble3 &position, const Double3 &direction);

	// Pitches and yaws relative to global up vector.
	void rotateX(Degrees deltaX);
	void rotateY(Degrees deltaY, Degrees pitchLimit);

	// Recalculates the camera so it faces the given point. The global up vector is used when generating the new 3D frame,
	// so don't give a point directly above or below the camera.
	void lookAt(const CoordDouble3 &coord);
};

#endif
