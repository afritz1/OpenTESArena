#ifndef CAMERA_3D_H
#define CAMERA_3D_H

#include "../Math/Matrix4.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"

// A camera for the player. Make sure not to look directly up or down, as
// that breaks the vector cross product used for determining the camera's axes.

// I purposefully decided not to have this class own the field of view or 
// the aspect ratio because 1) that's just more state to take care of, and
// 2) they're application-level variables that the user may change frequently,
// so they do just fine in an "Options" object instead.

class Camera3D
{
private:
	// A set of normalized axes acting as the camera's coordinate frame.
	Double3 forward, right, up;

	// Helper methods for rotating around "right" vector and "up" vector.
	void pitch(double radians);
	void yaw(double radians);
public:
	Double3 position;

	// Default constructor for the player's camera. The axes are generated based on 
	// the given normalized direction.
	Camera3D(const Double3 &position, const Double3 &direction);

	// Gets where the camera is looking towards.
	const Double3 &getDirection() const;

	// Gets the direction pointing right of the camera. Always parallel to the
	// ground (i.e., y == 0). Intended for strafing.
	const Double3 &getRight() const;

	// Generates a 4x4 view matrix for use with 3D transformations.
	Matrix4d getViewMatrix() const;

	// Pitches and yaws the camera relative to a fixed "global up" vector. "dx" affects 
	// left/right, "dy" affects up/down.
	void rotate(double dx, double dy);
};

#endif
