#include "RenderTriangle.h"

RenderTriangle::RenderTriangle(const Double3 &v0, const Double3 &v1, const Double3 &v2)
{
	this->init(v0, v1, v2);
}

RenderTriangle::RenderTriangle() { }

void RenderTriangle::init(const Double3 &v0, const Double3 &v1, const Double3 &v2)
{
	this->v0 = v0;
	this->v1 = v1;
	this->v2 = v2;
	this->v0v1 = v1 - v0;
	this->v1v2 = v2 - v1;
	this->v2v0 = v0 - v2;
	this->normal = this->v2v0.cross(this->v0v1).normalized();
}
