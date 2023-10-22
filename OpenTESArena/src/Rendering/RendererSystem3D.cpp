#include "RendererSystem3D.h"

RendererSystem3D::ProfilerData::ProfilerData(int width, int height, int threadCount, int drawCallCount, int sceneTriangleCount,
	int visTriangleCount, int textureCount, int64_t textureByteCount, int totalLightCount, int totalDepthTests, int totalColorWrites)
{
	this->width = width;
	this->height = height;
	this->threadCount = threadCount;
	this->drawCallCount = drawCallCount;
	this->sceneTriangleCount = sceneTriangleCount;
	this->visTriangleCount = visTriangleCount;
	this->textureCount = textureCount;
	this->textureByteCount = textureByteCount;
	this->totalLightCount = totalLightCount;
	this->totalDepthTests = totalDepthTests;
	this->totalColorWrites = totalColorWrites;
}

RendererSystem3D::~RendererSystem3D()
{
	// Do nothing.
}
