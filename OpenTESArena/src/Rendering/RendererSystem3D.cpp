#include "RendererSystem3D.h"

Renderer3DProfilerData::Renderer3DProfilerData(int width, int height, int threadCount, int drawCallCount, int presentedTriangleCount,
	int textureCount, int64_t textureByteCount, int totalLightCount, int64_t totalCoverageTests, int64_t totalDepthTests, int64_t totalColorWrites)
{
	this->width = width;
	this->height = height;
	this->threadCount = threadCount;
	this->drawCallCount = drawCallCount;
	this->presentedTriangleCount = presentedTriangleCount;
	this->textureCount = textureCount;
	this->textureByteCount = textureByteCount;
	this->totalLightCount = totalLightCount;
	this->totalCoverageTests = totalCoverageTests;
	this->totalDepthTests = totalDepthTests;
	this->totalColorWrites = totalColorWrites;
}

RendererSystem3D::~RendererSystem3D()
{
	// Do nothing.
}
