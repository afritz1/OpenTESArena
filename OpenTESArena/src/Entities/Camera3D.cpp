#include <cmath>

#include "Camera3D.h"
#include "../Math/Constants.h"
#include "../Math/Quaternion.h"

#include "components/debug/Debug.h"

namespace
{
	Radians SafeDegreesToRadians(Degrees degrees)
	{
		const Radians radians = degrees * Constants::DegToRad;
		if (!std::isfinite(radians))
		{
			return 0.0;
		}

		return radians;
	}
}

void Camera3D::init(const CoordDouble3 &position, const Double3 &direction)
{
	this->position = position;
	this->forward = direction;
	this->right = this->forward.cross(Double3::UnitY).normalized();
	this->up = this->right.cross(this->forward).normalized();
}

const Double3 &Camera3D::getDirection() const
{
	return this->forward;
}

const Double3 &Camera3D::getRight() const
{
	return this->right;
}

void Camera3D::pitch(Radians deltaAngle)
{
	const Quaternion q = Quaternion::fromAxisAngle(this->right, deltaAngle) * Quaternion(this->forward, 0.0);
	this->forward = Double3(q.x, q.y, q.z).normalized();
	this->right = this->forward.cross(Double3::UnitY).normalized();
	this->up = this->right.cross(this->forward).normalized();
}

void Camera3D::yaw(Radians deltaAngle)
{
	const Quaternion q = Quaternion::fromAxisAngle(Double3::UnitY, deltaAngle) * Quaternion(this->forward, 0.0);
	this->forward = Double3(q.x, q.y, q.z).normalized();
	this->right = this->forward.cross(Double3::UnitY).normalized();
	this->up = this->right.cross(this->forward).normalized();
}

void Camera3D::rotateX(Degrees dx)
{
	DebugAssert(std::isfinite(this->forward.length()));
	const Radians deltaAsRadians = SafeDegreesToRadians(dx);
	this->yaw(-deltaAsRadians);
}

void Camera3D::rotateY(Degrees dy, Degrees pitchLimit)
{
	DebugAssert(std::isfinite(this->forward.length()));
	DebugAssert(pitchLimit >= 0.0);
	DebugAssert(pitchLimit < 90.0);

	const Radians deltaAsRadians = SafeDegreesToRadians(dy);
	const Radians currentAngle = std::acos(this->forward.normalized().y);
	const Radians requestedAngle = currentAngle - deltaAsRadians;

	// Clamp to avoid breaking cross product.
	const Radians maxAngle = (90.0 - pitchLimit) * Constants::DegToRad;
	const Radians minAngle = (90.0 + pitchLimit) * Constants::DegToRad;
	const Radians actualDeltaAngle = (requestedAngle > minAngle) ? (currentAngle - minAngle) : ((requestedAngle < maxAngle) ? (currentAngle - maxAngle) : deltaAsRadians);

	this->pitch(actualDeltaAngle);
}

void Camera3D::lookAt(const CoordDouble3 &coord)
{
	const Double3 newForward = (coord - this->position).normalized();
	const Double3 newRight = newForward.cross(Double3::UnitY).normalized();
	const Double3 newUp = newRight.cross(newForward).normalized();

	// Only accept the change if it's valid.
	if (std::isfinite(newUp.length()))
	{
		this->forward = newForward;
		this->right = newRight;
		this->up = newUp;
	}
}
