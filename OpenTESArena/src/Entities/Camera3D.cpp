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

void Camera3D::rotateX(Degrees deltaX)
{
	DebugAssert(std::isfinite(this->forward.length()));
	const Radians deltaAsRadians = SafeDegreesToRadians(deltaX);
	const Quaternion quat = Quaternion::fromAxisAngle(Double3::UnitY, -deltaAsRadians) * Quaternion(this->forward, 0.0);
	this->forward = Double3(quat.x, quat.y, quat.z).normalized();
	this->right = this->forward.cross(Double3::UnitY).normalized();
	this->up = this->right.cross(this->forward).normalized();
}

void Camera3D::rotateY(Degrees deltaY, Degrees pitchLimit)
{
	DebugAssert(std::isfinite(this->forward.length()));
	DebugAssert(pitchLimit >= 0.0);
	DebugAssert(pitchLimit < 90.0);

	const Radians deltaAsRadians = SafeDegreesToRadians(deltaY);
	const Radians currentAngle = std::acos(this->forward.normalized().y);
	const Radians requestedAngle = currentAngle - deltaAsRadians;

	// Clamp to avoid breaking cross product.
	const Radians maxAngle = (90.0 - pitchLimit) * Constants::DegToRad;
	const Radians minAngle = (90.0 + pitchLimit) * Constants::DegToRad;
	const Radians actualDeltaAngle = (requestedAngle > minAngle) ? (currentAngle - minAngle) : ((requestedAngle < maxAngle) ? (currentAngle - maxAngle) : deltaAsRadians);

	const Quaternion quat = Quaternion::fromAxisAngle(this->right, actualDeltaAngle) * Quaternion(this->forward, 0.0);
	this->forward = Double3(quat.x, quat.y, quat.z).normalized();
	this->right = this->forward.cross(Double3::UnitY).normalized();
	this->up = this->right.cross(this->forward).normalized();
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
