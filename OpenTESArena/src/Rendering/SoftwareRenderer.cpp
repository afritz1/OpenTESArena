#include <algorithm>
#include <cmath>
#include <limits>

#include "ArenaRenderUtils.h"
#include "RendererUtils.h"
#include "RenderInitSettings.h"
#include "SoftwareRenderer.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Random.h"
#include "../Media/Color.h"
#include "../Media/Palette.h"
#include "../Media/TextureBuilder.h"

#include "components/debug/Debug.h"

SoftwareRenderer::SoftwareRenderer()
{
	
}

SoftwareRenderer::~SoftwareRenderer()
{
	
}

void SoftwareRenderer::init(const RenderInitSettings &settings)
{
	DebugNotImplemented();
}

void SoftwareRenderer::shutdown()
{
	DebugNotImplemented();
}

bool SoftwareRenderer::isInited() const
{
	return false;
}

void SoftwareRenderer::resize(int width, int height)
{
	DebugNotImplemented();
}

bool SoftwareRenderer::tryCreateObjectTexture(int width, int height, ObjectTextureID *outID)
{
	DebugNotImplemented();
	return false;
}

bool SoftwareRenderer::tryCreateObjectTexture(const TextureBuilder &textureBuilder, ObjectTextureID *outID)
{
	DebugNotImplemented();
	return false;
}

uint32_t *SoftwareRenderer::lockObjectTexture(ObjectTextureID id)
{
	DebugNotImplemented();
	return nullptr;
}

void SoftwareRenderer::unlockObjectTexture(ObjectTextureID id)
{
	DebugNotImplemented();
}

void SoftwareRenderer::freeObjectTexture(ObjectTextureID id)
{
	DebugNotImplemented();
}

bool SoftwareRenderer::tryGetEntitySelectionData(const Double2 &uv, ObjectTextureID textureID, bool pixelPerfect, bool *outIsSelected) const
{
	DebugNotImplemented(); // @todo: this is in LegacyRendererUtils
	return false;
}

Double3 SoftwareRenderer::screenPointToRay(double xPercent, double yPercent, const Double3 &cameraDirection, Degrees fovY, double aspect) const
{
	DebugNotImplemented(); // @todo: this is in LegacyRendererUtils
	return Double3();
}

RendererSystem3D::ProfilerData SoftwareRenderer::getProfilerData() const
{
	DebugNotImplemented();
	return ProfilerData(-1, -1, -1, -1, -1, -1);
}

void SoftwareRenderer::submitFrame(const RenderCamera &camera, const RenderFrameSettings &settings)
{
	DebugNotImplemented();
}

void SoftwareRenderer::present()
{
	DebugNotImplemented();
}
