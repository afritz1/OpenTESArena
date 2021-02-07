#ifndef CAMERA_3D_H
#define CAMERA_3D_H

#include "../Math/Matrix4.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../World/VoxelUtils.h"

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
	CoordDouble3 position;

	// Default constructor for the player's camera. The axes are generated based on 
	// the given normalized direction.
	Camera3D(const CoordDouble3 &position, const Double3 &direction);

	// Gets where the camera is looking towards.
	const Double3 &getDirection() const;

	// Gets the direction pointing right of the camera. Always parallel to the
	// ground (i.e., y == 0). Intended for strafing.
	const Double3 &getRight() const;

	// Pitches and yaws the camera relative to a fixed "global up" vector. "dx" affects 
	// left/right, "dy" affects up/down, and "pitchLimit" affects how high or low the camera 
	// can look in degrees.
	void rotate(double dx, double dy, double pitchLimit);

	// Recalculates the camera so it faces the given point. The "global up" vector
	// is used in the process of generating the new 3D frame, so do not give a point
	// directly above or below the camera.
	void lookAt(const CoordDouble3 &coord);
};

#endif
