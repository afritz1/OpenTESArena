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

class Camera3D
{
private:
	// @todo: polar coordinates (XYZ angles)
	Double3 forward, right, up;

	void pitch(Radians deltaAngle);
	void yaw(Radians deltaAngle);
public:
	CoordDouble3 position;

	void init(const CoordDouble3 &position, const Double3 &direction);

	const Double3 &getDirection() const;

	// Always parallel to the ground (i.e., y == 0). Intended for strafing.
	const Double3 &getRight() const;

	// Pitches and yaws the camera relative to a fixed global up vector. "pitchLimit" affects how high or low the camera 
	// can look in degrees.
	void rotateX(Degrees dx);
	void rotateY(Degrees dy, Degrees pitchLimit);

	// Recalculates the camera so it faces the given point. The global up vector is used when generating the new 3D frame,
	// so don't give a point directly above or below the camera.
	void lookAt(const CoordDouble3 &coord);
};

#endif
