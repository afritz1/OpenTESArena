#include <cassert>

#include "Rect3D.h"

Rect3D::Rect3D(const Float3f &p1, const Float3f &p2, const Float3f &p3)
	: p1(p1), p2(p2), p3(p3) { }

Rect3D::~Rect3D()
{

}

Rect3D Rect3D::fromFrame(const Float3f &point, const Float3f &right, 
	const Float3f &up, float width, float height)
{
	assert(right.isNormalized());
	assert(up.isNormalized());

	// Right and up diff vectors that determine how big the rectangle is.
	const Float3f dR = right * (width * 0.5f);
	const Float3f dU = up * height;

	const Float3f p1 = point + dR + dU;
	const Float3f p2 = point + dR;
	const Float3f p3 = point - dR;

	return Rect3D(p1, p2, p3);
}

const Float3f &Rect3D::getP1() const
{
	return this->p1;
}

const Float3f &Rect3D::getP2() const
{
	return this->p2;
}

const Float3f &Rect3D::getP3() const
{
	return this->p3;
}

Float3f Rect3D::getNormal() const
{
	const Float3f p1p2 = this->p2 - this->p1;
	const Float3f p1p3 = this->p3 - this->p1;

	// To do: Reverse the order if the normal is flipped.
	return p1p2.cross(p1p3).normalized();
}
