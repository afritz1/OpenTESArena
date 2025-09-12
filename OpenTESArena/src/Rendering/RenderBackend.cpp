#include "RenderBackend.h"

RendererProfilerData2D::RendererProfilerData2D()
{
	this->drawCallCount = 0;
	this->uiTextureCount = 0;
	this->uiTextureByteCount = 0;
}

RendererProfilerData3D::RendererProfilerData3D()
{
	this->width = 0;
	this->height = 0;
	this->threadCount = 0;
	this->drawCallCount = 0;
	this->presentedTriangleCount = 0;
	this->objectTextureCount = 0;
	this->objectTextureByteCount = 0;
	this->materialCount = 0;
	this->totalLightCount = 0;
	this->totalCoverageTests = 0;
	this->totalDepthTests = 0;
	this->totalColorWrites = 0;
}
