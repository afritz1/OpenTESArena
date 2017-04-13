#include <cmath>

#include "Camera2D.h"

#include "../Math/Constants.h"
#include "../Math/Quaternion.h"

Camera2D::Camera2D(const Double3 &position, const Double2 &direction)
	: position(position), direction(direction) { }

void Camera2D::yaw(double radians)
{
	// Convert direction to 3D.
	const Double3 forward = Double3(this->direction.x, 0.0, 
		this->direction.y).normalized();

	// Rotate around "global up".
	Quaternion q = Quaternion::fromAxisAngle(Double3::UnitY, radians) *
		Quaternion(forward, 0.0);

	// Convert back to 2D.
	this->direction = Double2(q.x, q.z).normalized();
}

void Camera2D::rotate(double degrees)
{
	double lookRightRads = degrees * DEG_TO_RAD;

	if (!std::isfinite(lookRightRads))
	{
		lookRightRads = 0.0;
	}

	this->yaw(-lookRightRads);
}

void Camera2D::lookAt(const Double2 &point)
{
	const Double2 position2D(this->position.x, this->position.z);
	const Double2 newDirection = (point - position2D).normalized();

	// Only accept the change if it's valid.
	if (std::isfinite(newDirection.length()))
	{
		this->direction = newDirection;
	}
}
