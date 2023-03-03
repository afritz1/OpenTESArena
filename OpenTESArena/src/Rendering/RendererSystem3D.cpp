#include "RendererSystem3D.h"

RendererSystem3D::ProfilerData::ProfilerData(int width, int height, int threadCount, int drawCallCount,
	int potentiallyVisTriangleCount, int visTriangleCount, int visLightCount)
{
	this->width = width;
	this->height = height;
	this->threadCount = threadCount;
	this->drawCallCount = drawCallCount;
	this->potentiallyVisTriangleCount = potentiallyVisTriangleCount;
	this->visTriangleCount = visTriangleCount;
	this->visLightCount = visLightCount;
}

RendererSystem3D::~RendererSystem3D()
{
	// Do nothing.
}
