#include "RendererSystem3D.h"

RendererSystem3D::LockedTexture::LockedTexture(void *texels, bool isTrueColor)
{
	this->texels = texels;
	this->isTrueColor = isTrueColor;
}

bool RendererSystem3D::LockedTexture::isValid()
{
	return this->texels != nullptr;
}

RendererSystem3D::ProfilerData::ProfilerData(int width, int height, int threadCount, int potentiallyVisFlatCount,
	int visFlatCount, int visLightCount)
{
	this->width = width;
	this->height = height;
	this->threadCount = threadCount;
	this->potentiallyVisFlatCount = potentiallyVisFlatCount;
	this->visFlatCount = visFlatCount;
	this->visLightCount = visLightCount;
}

RendererSystem3D::~RendererSystem3D()
{
	// Do nothing.
}
