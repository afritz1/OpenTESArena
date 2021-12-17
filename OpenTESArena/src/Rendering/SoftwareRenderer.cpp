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

void SoftwareRenderer::ObjectTexture::init(int width, int height)
{
	this->texels.init(width, height);
}

SoftwareRenderer::SoftwareRenderer()
{
	this->nextObjectTextureID = -1;
}

SoftwareRenderer::~SoftwareRenderer()
{
	
}

void SoftwareRenderer::init(const RenderInitSettings &settings)
{
	this->depthBuffer.init(settings.getWidth(), settings.getHeight());
	this->nextObjectTextureID = 0;
}

void SoftwareRenderer::shutdown()
{
	this->depthBuffer.clear();
	this->objectTextures.clear();
	this->freedObjectTextureIDs.clear();
	this->nextObjectTextureID = -1;
}

bool SoftwareRenderer::isInited() const
{
	return this->depthBuffer.isValid();
}

void SoftwareRenderer::resize(int width, int height)
{
	this->depthBuffer.init(width, height);
	this->depthBuffer.fill(std::numeric_limits<double>::infinity());
}

ObjectTextureID SoftwareRenderer::getNextObjectTextureID()
{
	ObjectTextureID id;
	if (!this->freedObjectTextureIDs.empty())
	{
		id = this->freedObjectTextureIDs.back();
		this->freedObjectTextureIDs.pop_back();
	}
	else
	{
		id = this->nextObjectTextureID;
		this->nextObjectTextureID++;
	}

	return id;
}

bool SoftwareRenderer::tryCreateObjectTexture(int width, int height, ObjectTextureID *outID)
{
	ObjectTexture texture;
	texture.init(width, height);
	texture.texels.fill(0);

	this->objectTextures.emplace_back(std::move(texture));
	*outID = this->getNextObjectTextureID();
	return true;
}

bool SoftwareRenderer::tryCreateObjectTexture(const TextureBuilder &textureBuilder, ObjectTextureID *outID)
{
	const int width = textureBuilder.getWidth();
	const int height = textureBuilder.getHeight();

	ObjectTexture texture;
	texture.init(width, height);

	const TextureBuilder::Type textureBuilderType = textureBuilder.getType();
	if (textureBuilderType == TextureBuilder::Type::Paletted)
	{
		const TextureBuilder::PalettedTexture &palettedTexture = textureBuilder.getPaletted();
		std::copy(palettedTexture.texels.get(), palettedTexture.texels.end(), texture.texels.get());
	}
	else if (textureBuilderType == TextureBuilder::Type::TrueColor)
	{
		DebugLogWarning("True color texture (dimensions " + std::to_string(width) + "x" + std::to_string(height) + ") not supported.");
		texture.texels.fill(0);
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(textureBuilderType)));
	}

	this->objectTextures.emplace_back(std::move(texture));
	*outID = this->getNextObjectTextureID();
	return true;
}

RendererSystem3D::LockedTexture SoftwareRenderer::lockObjectTexture(ObjectTextureID id)
{
	DebugAssertIndex(this->objectTextures, id);
	ObjectTexture &texture = this->objectTextures[id];
	return LockedTexture(texture.texels.get(), false);
}

void SoftwareRenderer::unlockObjectTexture(ObjectTextureID id)
{
	// Do nothing; any writes are already in RAM.
	static_cast<void>(id);
}

void SoftwareRenderer::freeObjectTexture(ObjectTextureID id)
{
	DebugAssertIndex(this->objectTextures, id);
	ObjectTexture &texture = this->objectTextures[id];
	texture.texels.clear();
	this->freedObjectTextureIDs.emplace_back(id);
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
