#include <cassert>
#include <cmath>

#include "Camera3D.h"

#include "../Math/Constants.h"
#include "../Math/Quaternion.h"

// The intention with using 45 degrees is to have the software renderer translate the
// projected Y coordinates by +/- one screen height at a vertical field of view of 90.0. 
// That's an arbitrary value I'm basing the Y-shearing on, just to have some kind of 
// "identity" for everything else to be relative to.
const double Camera3D::MIN_Y_LIMIT = (PI / 4.0) * RAD_TO_DEG;

Camera3D::Camera3D(const Double3 &position, const Double3 &direction)
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

Matrix4d Camera3D::getViewMatrix() const
{
	// Column vectors.
	const Double4 rotationX(this->right.x, this->up.x, -this->forward.x, 0.0);
	const Double4 rotationY(this->right.y, this->up.y, -this->forward.y, 0.0);
	const Double4 rotationZ(this->right.z, this->up.z, -this->forward.z, 0.0);

	const Matrix4d rotation(rotationX, rotationY, rotationZ, Double4::UnitW);

	// Column vector.
	const Double4 translationW(-this->position.x,
		-this->position.y, -this->position.z, 1.0);

	const Matrix4d translation(Double4::UnitX, Double4::UnitY, 
		Double4::UnitZ, translationW);

	return rotation * translation;
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

void Camera3D::rotate(double dx, double dy, double yLimit)
{
	assert(std::isfinite(this->forward.length()));
	assert(yLimit >= Camera3D::MIN_Y_LIMIT);

	double lookRightRads = dx * DEG_TO_RAD;
	double lookUpRads = dy * DEG_TO_RAD;

	if (!std::isfinite(lookRightRads))
	{
		lookRightRads = 0.0;
	}

	if (!std::isfinite(lookUpRads))
	{
		lookUpRads = 0.0;
	}

	const double currentDec = std::acos(this->forward.normalized().y);
	const double requestedDec = currentDec - lookUpRads;

	// Clamp the range that the camera can tilt up or down to avoid breaking
	// the vector cross product at extreme angles.
	const double zenithMaxDec = yLimit * DEG_TO_RAD;
	const double zenithMinDec = (180.0 - yLimit) * DEG_TO_RAD;

	lookUpRads = (requestedDec > zenithMinDec) ? (currentDec - zenithMinDec) :
		((requestedDec < zenithMaxDec) ? (currentDec - zenithMaxDec) : lookUpRads);

	// Only divide by zoom when sensitivity depends on field of view (which it doesn't here).
	//const double zoom = 1.0 / std::tan((fovY * 0.5) * DEG_TO_RAD);
	this->pitch(lookUpRads/* / zoom*/);
	this->yaw(-lookRightRads/* / zoom*/);
}

void Camera3D::rotate(double dx, double dy)
{
	this->rotate(dx, dy, Camera3D::MIN_Y_LIMIT);
}

void Camera3D::lookAt(const Double3 &point)
{
	const Double3 newForward = (point - this->position).normalized();
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
