#include "RendererSystem3D.h"

RendererSystem3D::ProfilerData::ProfilerData(int width, int height, int potentiallyVisFlatCount,
	int visFlatCount, int visLightCount)
{
	this->width = width;
	this->height = height;
	this->potentiallyVisFlatCount = potentiallyVisFlatCount;
	this->visFlatCount = visFlatCount;
	this->visLightCount = visLightCount;
}
