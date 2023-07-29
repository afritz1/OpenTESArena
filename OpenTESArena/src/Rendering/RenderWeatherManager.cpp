#include <algorithm>

#include "Renderer.h"
#include "RenderWeatherManager.h"
#include "../Weather/ArenaWeatherUtils.h"
#include "../Weather/WeatherInstance.h"

RenderWeatherManager::RenderWeatherManager()
{
	this->rainVertexBufferID = -1;
	this->rainNormalBufferID = -1;
	this->rainTexCoordBufferID = -1;
	this->rainIndexBufferID = -1;
	this->rainTextureID = -1;

	this->snowVertexBufferID = -1;
	this->snowNormalBufferID = -1;	
	this->snowTexCoordBufferID = -1;	
	for (IndexBufferID &indexBufferID : this->snowIndexBufferIDs)
	{
		indexBufferID = -1;
	}

	for (ObjectTextureID &textureID : this->snowTextureIDs)
	{
		textureID = -1;
	}

	this->fogVertexBufferID = -1;
	this->fogNormalBufferID = -1;
	this->fogTexCoordBufferID = -1;
	this->fogIndexBufferID = -1;
	this->fogTextureID = -1;
}

bool RenderWeatherManager::initMeshes(Renderer &renderer)
{
	DebugLogError("Not implemented: RenderWeatherManager::initMeshes()");
	return true;
}

bool RenderWeatherManager::initTextures(Renderer &renderer)
{
	// Init rain texture.
	constexpr int rainTextureWidth = ArenaRenderUtils::RAINDROP_TEXTURE_WIDTH;
	constexpr int rainTextureHeight = ArenaRenderUtils::RAINDROP_TEXTURE_HEIGHT;
	constexpr int rainTexelCount = rainTextureWidth * rainTextureHeight;
	constexpr int rainBytesPerTexel = 1;
	if (!renderer.tryCreateObjectTexture(rainTextureWidth, rainTextureHeight, rainBytesPerTexel, &this->rainTextureID))
	{
		DebugLogError("Couldn't create rain object texture.");
		return false;
	}

	LockedTexture lockedRainTexture = renderer.lockObjectTexture(this->rainTextureID);
	if (!lockedRainTexture.isValid())
	{
		DebugLogError("Couldn't lock rain object texture for writing.");
		this->freeParticleBuffers(renderer);
		return false;
	}

	const uint8_t *srcRainTexels = ArenaRenderUtils::RAINDROP_TEXELS;
	uint8_t *dstRainTexels = static_cast<uint8_t*>(lockedRainTexture.texels);
	std::copy(srcRainTexels, srcRainTexels + rainTexelCount, dstRainTexels);
	renderer.unlockObjectTexture(this->rainTextureID);

	// Init snow textures.
	for (size_t i = 0; i < std::size(this->snowTextureIDs); i++)
	{
		const int snowTextureWidth = ArenaRenderUtils::SNOWFLAKE_TEXTURE_WIDTHS[i];
		const int snowTextureHeight = ArenaRenderUtils::SNOWFLAKE_TEXTURE_HEIGHTS[i];
		const int snowTexelCount = snowTextureWidth * snowTextureHeight;
		constexpr int snowBytesPerTexel = rainBytesPerTexel;
		ObjectTextureID &snowTextureID = this->snowTextureIDs[i];
		if (!renderer.tryCreateObjectTexture(snowTextureWidth, snowTextureHeight, snowBytesPerTexel, &snowTextureID))
		{
			DebugLogError("Couldn't create snow object texture \"" + std::to_string(i) + "\".");
			this->freeParticleBuffers(renderer);
			return false;
		}

		LockedTexture lockedSnowTexture = renderer.lockObjectTexture(snowTextureID);
		if (!lockedSnowTexture.isValid())
		{
			DebugLogError("Couldn't lock snow object texture \"" + std::to_string(i) + "\" for writing.");
			this->freeParticleBuffers(renderer);
			return false;
		}

		const uint8_t *srcSnowTexels = ArenaRenderUtils::SNOWFLAKE_TEXELS_PTRS[i];
		uint8_t *dstSnowTexels = static_cast<uint8_t*>(lockedSnowTexture.texels);
		std::copy(srcSnowTexels, srcSnowTexels + snowTexelCount, dstSnowTexels);
		renderer.unlockObjectTexture(snowTextureID);
	}

	// Init fog texture (currently temp, not understood).
	constexpr int fogTextureWidth = ArenaRenderUtils::FOG_MATRIX_WIDTH;
	constexpr int fogTextureHeight = ArenaRenderUtils::FOG_MATRIX_HEIGHT;
	constexpr int fogTexelCount = fogTextureWidth * fogTextureHeight;
	constexpr int fogBytesPerTexel = 1;
	if (!renderer.tryCreateObjectTexture(fogTextureWidth, fogTextureHeight, fogBytesPerTexel, &this->fogTextureID))
	{
		DebugLogError("Couldn't create fog object texture.");
		return false;
	}

	LockedTexture lockedFogTexture = renderer.lockObjectTexture(this->fogTextureID);
	if (!lockedFogTexture.isValid())
	{
		DebugLogError("Couldn't lock fog object texture for writing.");
		return false;
	}

	const uint8_t tempFogTexelColors[] = { 1, 2, 3, 4 };
	const uint8_t *srcFogTexels = tempFogTexelColors;
	uint8_t *dstFogTexels = static_cast<uint8_t*>(lockedFogTexture.texels);
	std::copy(srcFogTexels, srcFogTexels + fogTexelCount, dstFogTexels);
	renderer.unlockObjectTexture(this->fogTextureID);
	return true;
}

bool RenderWeatherManager::init(Renderer &renderer)
{
	if (!this->initMeshes(renderer))
	{
		DebugLogError("Couldn't init all weather meshes.");
		return false;
	}

	if (!this->initTextures(renderer))
	{
		DebugLogError("Couldn't init all weather textures.");
		return false;
	}

	return true;
}

void RenderWeatherManager::shutdown(Renderer &renderer)
{
	this->freeParticleBuffers(renderer);
	this->rainDrawCalls.clear();
	this->snowDrawCalls.clear();

	this->freeFogBuffers(renderer);
	this->fogDrawCall.clear();
}

BufferView<const RenderDrawCall> RenderWeatherManager::getRainDrawCalls() const
{
	return this->rainDrawCalls;
}

BufferView<const RenderDrawCall> RenderWeatherManager::getSnowDrawCalls() const
{
	return this->snowDrawCalls;
}

const RenderDrawCall &RenderWeatherManager::getFogDrawCall() const
{
	return this->fogDrawCall;
}

void RenderWeatherManager::freeParticleBuffers(Renderer &renderer)
{
	if (this->rainVertexBufferID >= 0)
	{
		renderer.freeVertexBuffer(this->rainVertexBufferID);
		this->rainVertexBufferID = -1;
	}

	if (this->rainNormalBufferID >= 0)
	{
		renderer.freeAttributeBuffer(this->rainNormalBufferID);
		this->rainNormalBufferID = -1;
	}

	if (this->rainTexCoordBufferID >= 0)
	{
		renderer.freeAttributeBuffer(this->rainTexCoordBufferID);
		this->rainTexCoordBufferID = -1;
	}

	if (this->rainIndexBufferID >= 0)
	{
		renderer.freeIndexBuffer(this->rainIndexBufferID);
		this->rainIndexBufferID = -1;
	}

	if (this->rainTextureID >= 0)
	{
		renderer.freeObjectTexture(this->rainTextureID);
		this->rainTextureID = -1;
	}

	if (this->snowVertexBufferID >= 0)
	{
		renderer.freeVertexBuffer(this->snowVertexBufferID);
		this->snowVertexBufferID = -1;
	}

	if (this->snowNormalBufferID >= 0)
	{
		renderer.freeAttributeBuffer(this->snowNormalBufferID);
		this->snowNormalBufferID = -1;
	}

	if (this->snowTexCoordBufferID >= 0)
	{
		renderer.freeAttributeBuffer(this->snowTexCoordBufferID);
		this->snowTexCoordBufferID = -1;
	}

	for (IndexBufferID &snowIndexBufferID : this->snowIndexBufferIDs)
	{
		if (snowIndexBufferID >= 0)
		{
			renderer.freeIndexBuffer(snowIndexBufferID);
			snowIndexBufferID = -1;
		}
	}

	for (ObjectTextureID &snowTextureID : this->snowTextureIDs)
	{
		if (snowTextureID >= 0)
		{
			renderer.freeObjectTexture(snowTextureID);
			snowTextureID = -1;
		}
	}
}

void RenderWeatherManager::freeFogBuffers(Renderer &renderer)
{
	if (this->fogVertexBufferID >= 0)
	{
		renderer.freeVertexBuffer(this->fogVertexBufferID);
		this->fogVertexBufferID = -1;
	}

	if (this->fogNormalBufferID >= 0)
	{
		renderer.freeAttributeBuffer(this->fogNormalBufferID);
		this->fogNormalBufferID = -1;
	}

	if (this->fogTexCoordBufferID >= 0)
	{
		renderer.freeAttributeBuffer(this->fogTexCoordBufferID);
		this->fogTexCoordBufferID = -1;
	}

	if (this->fogIndexBufferID >= 0)
	{
		renderer.freeIndexBuffer(this->fogIndexBufferID);
		this->fogIndexBufferID = -1;
	}

	if (this->fogTextureID >= 0)
	{
		renderer.freeObjectTexture(this->fogTextureID);
		this->fogTextureID = -1;
	}
}

void RenderWeatherManager::update(const WeatherInstance &weatherInst, const RenderCamera &camera)
{
	// @todo: might need camera for doing the projection coordinates right

	if (weatherInst.hasRain())
	{
		// @todo: make draw calls
	}

	if (weatherInst.hasSnow())
	{
		// @todo: make draw calls
	}

	if (weatherInst.hasFog())
	{
		// @todo: update fog state + texture
	}
}
