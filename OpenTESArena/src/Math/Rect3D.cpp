#include "Rect3D.h"

Rect3D::Rect3D(const Float3f &p1, const Float3f &p2, const Float3f &p3)
	: p1(p1), p2(p2), p3(p3), p4(p1 + (p2 - p1) + (p3 - p1)) { }

Rect3D::~Rect3D()
{

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

const Float3f &Rect3D::getP4() const
{
	return this->p4;
}

Float3f Rect3D::getNormal() const
{
	const Float3f p1p2 = this->p2 - this->p1;
	const Float3f p1p3 = this->p3 - this->p1;
	return p1p2.cross(p1p3).normalized();
}
