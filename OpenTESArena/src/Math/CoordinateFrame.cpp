#include "CoordinateFrame.h"

CoordinateFrame::CoordinateFrame(const Double3 &forward, const Double3 &right,
	const Double3 &up)
{
	this->forward = forward;
	this->right = right;
	this->up = up;
}

CoordinateFrame::~CoordinateFrame()
{

}

const Double3 &CoordinateFrame::getForward() const
{
	return this->forward;
}

const Double3 &CoordinateFrame::getRight() const
{
	return this->right;
}

const Double3 &CoordinateFrame::getUp() const
{
	return this->up;
}

Matrix4d CoordinateFrame::toMatrix4(const Double3 &point) const
{
	// Verify that this code is correct. It probably is.

	const Double3 forward = this->getForward().normalized();
	const Double3 right = this->getRight().normalized();
	const Double3 up = this->getUp().normalized();

	// Column vectors.
	const Double4 rotationX(right.x, up.x, -forward.x, 0.0);
	const Double4 rotationY(right.y, up.y, -forward.y, 0.0);
	const Double4 rotationZ(right.z, up.z, -forward.z, 0.0);
	const Double4 rotationW(0.0, 0.0, 0.0, 1.0);
	
	const Matrix4d rotation(rotationX, rotationY, rotationZ, rotationW);

	// Column vectors.
	const Double4 translationX(1.0, 0.0, 0.0, 0.0);
	const Double4 translationY(0.0, 1.0, 0.0, 0.0);
	const Double4 translationZ(0.0, 0.0, 1.0, 0.0);
	const Double4 translationW(-point.x, -point.y, -point.z, 1.0);

	const Matrix4d translation(translationX, translationY, translationZ, translationW);

	return rotation * translation;
}
