#include "CoordinateFrame.h"

#include "../Math/Float4.h"

CoordinateFrame::CoordinateFrame(const Float3d &forward, const Float3d &right,
	const Float3d &up)
{
	this->forward = forward;
	this->right = right;
	this->up = up;
}

CoordinateFrame::~CoordinateFrame()
{

}

const Float3d &CoordinateFrame::getForward() const
{
	return this->forward;
}

const Float3d &CoordinateFrame::getRight() const
{
	return this->right;
}

const Float3d &CoordinateFrame::getUp() const
{
	return this->up;
}

Matrix4d CoordinateFrame::toMatrix4(const Float3d &point) const
{
	// Verify that this code is correct. It probably is.

	auto forward = this->getForward().normalized();
	auto right = this->getRight().normalized();
	auto up = this->getUp().normalized();

	// Column vectors.
	auto rotationX = Float4d(right.getX(), up.getX(), -forward.getX(), 0.0);
	auto rotationY = Float4d(right.getY(), up.getY(), -forward.getY(), 0.0);
	auto rotationZ = Float4d(right.getZ(), up.getZ(), -forward.getZ(), 0.0);
	auto rotationW = Float4d(0.0, 0.0, 0.0, 1.0);
	
	auto rotation = Matrix4d(rotationX, rotationY, rotationZ, rotationW);

	// Column vectors.
	auto translationX = Float4d(1.0, 0.0, 0.0, 0.0);
	auto translationY = Float4d(0.0, 1.0, 0.0, 0.0);
	auto translationZ = Float4d(0.0, 0.0, 1.0, 0.0);
	auto translationW = Float4d(-point.getX(), -point.getY(), -point.getZ(), 1.0);

	auto translation = Matrix4d(translationX, translationY, translationZ, translationW);

	return rotation * translation;
}
