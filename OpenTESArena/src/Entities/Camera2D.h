#ifndef CAMERA_2D_H
#define CAMERA_2D_H

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"

// Intended for non-player entities in Arena.

class Camera2D
{
private:
	// Helper method for rotating.
	void yaw(double radians);
public:
	// Position with 2D direction in the XZ plane.
	Double3 position;
	Double2 direction;

	Camera2D(const Double3 &position, const Double2 &direction);

	// A simple method for turning the camera around a "global up" vector.
	void rotate(double degrees);
};

#endif
