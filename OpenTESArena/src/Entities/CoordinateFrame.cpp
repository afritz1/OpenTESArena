#include "CoordinateFrame.h"

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
