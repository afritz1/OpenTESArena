#ifndef CAMERA_2D_H
#define CAMERA_2D_H

#include "../Math/Vector2.h"

// Intended for non-player entities in Arena.

class Camera2D
{
private:
	// Helper method for rotating.
	void yaw(double radians);
public:
	// Position in the XZ plane.
	Double2 position;
	Double2 direction;

	Camera2D(const Double2 &position, const Double2 &direction);

	// A simple method for turning the camera around a "global up" vector.
	void rotate(double degrees);

	// Recalculates the camera so it faces the given point.
	void lookAt(const Double2 &point);
};

#endif
