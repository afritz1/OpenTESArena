#include "Quad.h"

Quad::Quad() { }

Quad::Quad(const Double3 &v0, const Double3 &v1, const Double3 &v2, const Double3 &v3)
	: v0(v0), v1(v1), v2(v2), v3(v3) { }

Quad::Quad(const Double3 &v0, const Double3 &v1, const Double3 &v2)
	: Quad(v0, v1, v2, v0 + (v2 - v1)) { }

void Quad::init(const Double3 &v0, const Double3 &v1, const Double3 &v2, const Double3 &v3)
{
	this->v0 = v0;
	this->v1 = v1;
	this->v2 = v2;
	this->v3 = v3;
}

void Quad::init(const Double3 &v0, const Double3 &v1, const Double3 &v2)
{
	const Double3 v3 = v0 + (v2 - v1);
	this->init(v0, v1, v2, v3);
}

const Double3 &Quad::getV0() const
{
	return this->v0;
}

const Double3 &Quad::getV1() const
{
	return this->v1;
}

const Double3 &Quad::getV2() const
{
	return this->v2;
}

const Double3 &Quad::getV3() const
{
	return this->v3;
}

Double3 Quad::getV0V1() const
{
	return this->getV1() - this->getV0();
}

Double3 Quad::getV1V2() const
{
	return this->getV2() - this->getV1();
}

Double3 Quad::getV2V3() const
{
	return this->getV3() - this->getV2();
}

Double3 Quad::getV3V0() const
{
	return this->getV0() - this->getV3();
}

Double3 Quad::getNormal() const
{
	const Double3 v0v1 = this->getV0V1();
	const Double3 v0v3 = this->getV3() - this->getV0();
	return v0v1.cross(v0v3).normalized();
}
