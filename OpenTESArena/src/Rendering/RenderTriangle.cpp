#include "RenderTriangle.h"

RenderTriangle::RenderTriangle(const Double3 &v0, const Double3 &v1, const Double3 &v2, const Double2 &uv0,
	const Double2 &uv1, const Double2 &uv2, ObjectTextureID textureID)
{
	this->init(v0, v1, v2, uv0, uv1, uv2, textureID);
}

RenderTriangle::RenderTriangle()
{
	this->textureID = -1;
}

void RenderTriangle::init(const Double3 &v0, const Double3 &v1, const Double3 &v2, const Double2 &uv0,
	const Double2 &uv1, const Double2 &uv2, ObjectTextureID textureID)
{
	this->v0 = v0;
	this->v1 = v1;
	this->v2 = v2;
	this->v0v1 = v1 - v0;
	this->v1v2 = v2 - v1;
	this->v2v0 = v0 - v2;
	this->normal = this->v2v0.cross(this->v0v1).normalized();
	this->uv0 = uv0;
	this->uv1 = uv1;
	this->uv2 = uv2;
	this->textureID = textureID;
}
