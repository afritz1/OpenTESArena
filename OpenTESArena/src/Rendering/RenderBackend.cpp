#include "RenderBackend.h"

RenderElement2D::RenderElement2D(UiTextureID id, double x, double y, double width, double height)
{
	this->id = id;
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
}

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
