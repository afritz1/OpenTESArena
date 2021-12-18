#include <algorithm>
#include <cmath>
#include <limits>

#include "ArenaRenderUtils.h"
#include "LegacyRendererUtils.h"
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
	this->depthBuffer.init(settings.width, settings.height);
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

LockedTexture SoftwareRenderer::lockObjectTexture(ObjectTextureID id)
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

std::optional<Int2> SoftwareRenderer::tryGetObjectTextureDims(ObjectTextureID id) const
{
	DebugAssertIndex(this->objectTextures, id);
	const ObjectTexture &texture = this->objectTextures[id];
	return Int2(texture.texels.getWidth(), texture.texels.getHeight());
}

bool SoftwareRenderer::tryGetEntitySelectionData(const Double2 &uv, ObjectTextureID textureID, bool pixelPerfect, bool *outIsSelected) const
{
	if (pixelPerfect)
	{
		// Get the texture list from the texture group at the given animation state and angle.
		DebugAssertIndex(this->objectTextures, textureID);
		const ObjectTexture &texture = this->objectTextures[textureID];
		const int textureWidth = texture.texels.getWidth();
		const int textureHeight = texture.texels.getHeight();

		const int textureX = static_cast<int>(uv.x * static_cast<double>(textureWidth));
		const int textureY = static_cast<int>(uv.y * static_cast<double>(textureHeight));

		if ((textureX < 0) || (textureX >= textureWidth) ||
			(textureY < 0) || (textureY >= textureHeight))
		{
			// Outside the texture; out of bounds.
			return false;
		}

		// Check if the texel is non-transparent.
		const uint8_t texel = texture.texels.get(textureX, textureY);
		*outIsSelected = texel != 0;
		return true;
	}
	else
	{
		// The entity's projected rectangle is hit if the texture coordinates are valid.
		const bool withinEntity = (uv.x >= 0.0) && (uv.x <= 1.0) && (uv.y >= 0.0) && (uv.y <= 1.0);
		*outIsSelected = withinEntity;
		return true;
	}
}

Double3 SoftwareRenderer::screenPointToRay(double xPercent, double yPercent, const Double3 &cameraDirection,
	Degrees fovY, double aspect) const
{
	return LegacyRendererUtils::screenPointToRay(xPercent, yPercent, cameraDirection, fovY, aspect);
}

RendererSystem3D::ProfilerData SoftwareRenderer::getProfilerData() const
{
	const int renderWidth = this->depthBuffer.getWidth();
	const int renderHeight = this->depthBuffer.getHeight();

	// @todo
	const int threadCount = 1;
	const int potentiallyVisFlatCount = 0;
	const int visFlatCount = 0;
	const int visLightCount = 0;

	return ProfilerData(renderWidth, renderHeight, threadCount, potentiallyVisFlatCount, visFlatCount, visLightCount);
}

void SoftwareRenderer::submitFrame(const RenderCamera &camera, const RenderFrameSettings &settings, uint32_t *outputBuffer)
{
	// @todo: shade RGB directions into frame buffer
}

void SoftwareRenderer::present()
{
	// Do nothing for now, might change later.
}
