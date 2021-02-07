#include <cmath>

#include "Camera3D.h"
#include "../Math/Constants.h"
#include "../Math/Quaternion.h"

#include "components/debug/Debug.h"

Camera3D::Camera3D(const CoordDouble3 &position, const Double3 &direction)
	: forward(direction), right(forward.cross(Double3::UnitY).normalized()),
	up(right.cross(forward).normalized()), position(position) { }

const Double3 &Camera3D::getDirection() const
{
	return this->forward;
}

const Double3 &Camera3D::getRight() const
{
	return this->right;
}

void Camera3D::pitch(double radians)
{
	Quaternion q = Quaternion::fromAxisAngle(this->right, radians) *
		Quaternion(this->forward, 0.0);

	this->forward = Double3(q.x, q.y, q.z).normalized();
	this->right = this->forward.cross(Double3::UnitY).normalized();
	this->up = this->right.cross(this->forward).normalized();
}

void Camera3D::yaw(double radians)
{
	Quaternion q = Quaternion::fromAxisAngle(Double3::UnitY, radians) *
		Quaternion(this->forward, 0.0);

	this->forward = Double3(q.x, q.y, q.z).normalized();
	this->right = this->forward.cross(Double3::UnitY).normalized();
	this->up = this->right.cross(this->forward).normalized();
}

void Camera3D::rotate(double dx, double dy, double pitchLimit)
{
	DebugAssert(std::isfinite(this->forward.length()));
	DebugAssert(pitchLimit >= 0.0);
	DebugAssert(pitchLimit < 90.0);

	auto safeDegToRad = [](double degrees)
	{
		const double rads = degrees * Constants::DegToRad;
		return std::isfinite(rads) ? rads : 0.0;
	};

	const double lookRightRads = safeDegToRad(dx);
	double lookUpRads = safeDegToRad(dy);

	const double currentDec = std::acos(this->forward.normalized().y);
	const double requestedDec = currentDec - lookUpRads;

	// Clamp the range that the camera can tilt up or down to avoid breaking
	// the vector cross product at extreme angles.
	const double zenithMaxDec = (90.0 - pitchLimit) * Constants::DegToRad;
	const double zenithMinDec = (90.0 + pitchLimit) * Constants::DegToRad;

	lookUpRads = (requestedDec > zenithMinDec) ? (currentDec - zenithMinDec) :
		((requestedDec < zenithMaxDec) ? (currentDec - zenithMaxDec) : lookUpRads);

	// Only divide by zoom when sensitivity depends on field of view (which it doesn't here).
	//const double zoom = 1.0 / std::tan((fovY * 0.5) * DEG_TO_RAD);
	this->pitch(lookUpRads/* / zoom*/);
	this->yaw(-lookRightRads/* / zoom*/);
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
