#include <algorithm>

#include "RenderCamera.h"
#include "RenderCommandBuffer.h"
#include "RenderTransform.h"
#include "Renderer.h"
#include "RendererUtils.h"
#include "RenderWeatherManager.h"
#include "../Math/Constants.h"
#include "../Weather/ArenaWeatherUtils.h"
#include "../Weather/WeatherInstance.h"
#include "../World/MeshUtils.h"

namespace
{
	constexpr int RainTextureWidth = ArenaRenderUtils::RAINDROP_TEXTURE_WIDTH;
	constexpr int RainTextureHeight = ArenaRenderUtils::RAINDROP_TEXTURE_HEIGHT;
	constexpr int BytesPerTexel = 1;

	constexpr double ParticleArbitraryZ = RendererUtils::NEAR_PLANE + Constants::Epsilon; // Close to camera but in front of near plane. @todo: use shader w/ no depth test

	int GetSnowTextureWidth(int index)
	{
		return ArenaRenderUtils::SNOWFLAKE_TEXTURE_WIDTHS[index];
	}

	int GetSnowTextureHeight(int index)
	{
		return ArenaRenderUtils::SNOWFLAKE_TEXTURE_HEIGHTS[index];
	}

	Matrix4d MakeParticleTranslationMatrix(const RenderCamera &camera, double xPercent, double yPercent)
	{
		const Double3 basePosition = camera.worldPoint;
		const Double3 centerDir = camera.forwardScaled * ParticleArbitraryZ;
		const Double3 rightDir = camera.rightScaled * ParticleArbitraryZ;
		const Double3 upDir = camera.upScaled * ParticleArbitraryZ;
		const Double3 topLeftPoint = basePosition + centerDir - rightDir + upDir;
		const Double3 position = topLeftPoint + (rightDir * (2.0 * xPercent)) - (upDir * (2.0 * yPercent));
		return Matrix4d::translation(position.x, position.y, position.z);
	}

	Matrix4d MakeParticleRotationMatrix(Degrees yaw, Degrees pitch)
	{
		const Radians yawRadians = MathUtils::degToRad(90.0 - yaw);
		const Radians pitchRadians = MathUtils::degToRad(pitch);
		const Matrix4d yawRotation = Matrix4d::yRotation(yawRadians);
		const Matrix4d pitchRotation = Matrix4d::zRotation(pitchRadians);
		return yawRotation * pitchRotation;
	}

	Matrix4d MakeParticleScaleMatrix(int textureWidth, int textureHeight)
	{
		const double baseWidth = static_cast<double>(textureWidth) / 100.0;
		const double baseHeight = static_cast<double>(textureHeight) / 100.0;
		const double scaledWidth = baseWidth * ParticleArbitraryZ;
		const double scaledHeight = baseHeight * ParticleArbitraryZ;
		return Matrix4d::scale(1.0, scaledHeight, scaledWidth);
	}
}

RenderWeatherManager::RenderWeatherManager()
{
	this->particlePositionBufferID = -1;
	this->particleNormalBufferID = -1;
	this->particleTexCoordBufferID = -1;
	this->particleIndexBufferID = -1;

	this->rainTransformBufferID = -1;
	this->rainTextureID = -1;

	this->snowTransformBufferID = -1;
	for (ObjectTextureID &textureID : this->snowTextureIDs)
	{
		textureID = -1;
	}

	this->fogPositionBufferID = -1;
	this->fogNormalBufferID = -1;
	this->fogTexCoordBufferID = -1;
	this->fogIndexBufferID = -1;
	this->fogTransformBufferID = -1;
	this->fogTextureID = -1;
}

bool RenderWeatherManager::initMeshes(Renderer &renderer)
{
	constexpr int positionComponentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
	constexpr int normalComponentsPerVertex = MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
	constexpr int texCoordComponentsPerVertex = MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX;

	constexpr int particleMeshVertexCount = 4;
	constexpr int particleMeshIndexCount = 6;

	constexpr double particlePositions[particleMeshVertexCount * positionComponentsPerVertex] =
	{
		0.0, 0.0, 0.0, // Let the top left be the origin so each particle is positioned like a cursor icon
		0.0, -1.0, 0.0,
		0.0, -1.0, 1.0,
		0.0, 0.0, 1.0
	};

	constexpr double particleNormals[particleMeshVertexCount * normalComponentsPerVertex] =
	{
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0
	};

	constexpr double particleTexCoords[particleMeshVertexCount * texCoordComponentsPerVertex] =
	{
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	constexpr int32_t particleIndices[particleMeshIndexCount] =
	{
		0, 1, 2,
		2, 3, 0
	};

	this->particlePositionBufferID = renderer.createVertexPositionBuffer(particleMeshVertexCount, positionComponentsPerVertex);
	if (this->particlePositionBufferID < 0)
	{
		DebugLogError("Couldn't create vertex position buffer for rain mesh ID.");
		this->freeParticleBuffers(renderer);
		return false;
	}

	this->particleNormalBufferID = renderer.createVertexAttributeBuffer(particleMeshVertexCount, normalComponentsPerVertex);
	if (this->particleNormalBufferID < 0)
	{
		DebugLogError("Couldn't create vertex normal attribute buffer for rain mesh def.");
		this->freeParticleBuffers(renderer);
		return false;
	}

	this->particleTexCoordBufferID = renderer.createVertexAttributeBuffer(particleMeshVertexCount, texCoordComponentsPerVertex);
	if (this->particleTexCoordBufferID < 0)
	{
		DebugLogError("Couldn't create vertex tex coord attribute buffer for rain mesh def.");
		this->freeParticleBuffers(renderer);
		return false;
	}

	this->particleIndexBufferID = renderer.createIndexBuffer(particleMeshIndexCount);
	if (this->particleIndexBufferID < 0)
	{
		DebugLogError("Couldn't create index buffer for rain mesh def.");
		this->freeParticleBuffers(renderer);
		return false;
	}

	renderer.populateVertexPositionBuffer(this->particlePositionBufferID, particlePositions);
	renderer.populateVertexAttributeBuffer(this->particleNormalBufferID, particleNormals);
	renderer.populateVertexAttributeBuffer(this->particleTexCoordBufferID, particleTexCoords);
	renderer.populateIndexBuffer(this->particleIndexBufferID, particleIndices);

	constexpr int fogMeshVertexCount = 24; // 4 vertices per cube face
	constexpr int fogMeshIndexCount = 36;

	// Turned inward to face the camera.
	constexpr double fogPositions[fogMeshVertexCount * positionComponentsPerVertex] =
	{
		// X=0
		-0.5, 0.5, 0.5,
		-0.5, -0.5, 0.5,
		-0.5, -0.5, -0.5,
		-0.5, 0.5, -0.5,
		// X=1
		0.5, 0.5, -0.5,
		0.5, -0.5, -0.5,
		0.5, -0.5, 0.5,
		0.5, 0.5, 0.5,
		// Y=0
		-0.5, -0.5, 0.5,
		0.5, -0.5, 0.5,
		0.5, -0.5, -0.5,
		-0.5, -0.5, -0.5,
		// Y=1
		-0.5, 0.5, -0.5,
		0.5, 0.5, -0.5,
		0.5, 0.5, 0.5,
		-0.5, 0.5, 0.5,
		// Z=0
		-0.5, 0.5, -0.5,
		-0.5, -0.5, -0.5,
		0.5, -0.5, -0.5,
		0.5, 0.5, -0.5,
		// Z=1
		0.5, 0.5, 0.5,
		0.5, -0.5, 0.5,
		-0.5, -0.5, 0.5,
		-0.5, 0.5, 0.5
	};

	constexpr double fogNormals[fogMeshVertexCount * normalComponentsPerVertex] =
	{
		// X=0
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		// X=1
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		// Y=0
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		// Y=1
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		0.0, -1.0, 0.0,
		// Z=0
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		// Z=1
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0,
		0.0, 0.0, -1.0
	};

	constexpr double fogTexCoords[fogMeshVertexCount * texCoordComponentsPerVertex] =
	{
		// X=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// X=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Y=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Y=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Z=0
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		// Z=1
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	constexpr int32_t fogIndices[fogMeshIndexCount] =
	{
		// X=0
		0, 1, 2,
		2, 3, 0,
		// X=1
		4, 5, 6,
		6, 7, 4,
		// Y=0
		8, 9, 10,
		10, 11, 8,
		// Y=1
		12, 13, 14,
		14, 15, 12,
		// Z=0
		16, 17, 18,
		18, 19, 16,
		// Z=1
		20, 21, 22,
		22, 23, 20
	};

	this->fogPositionBufferID = renderer.createVertexPositionBuffer(fogMeshVertexCount, positionComponentsPerVertex);
	if (this->fogPositionBufferID < 0)
	{
		DebugLogError("Couldn't create position buffer for fog mesh ID.");
		this->freeParticleBuffers(renderer);
		return false;
	}

	this->fogNormalBufferID = renderer.createVertexAttributeBuffer(fogMeshVertexCount, normalComponentsPerVertex);
	if (this->fogNormalBufferID < 0)
	{
		DebugLogError("Couldn't create normal attribute buffer for fog mesh def.");
		this->freeParticleBuffers(renderer);
		this->freeFogBuffers(renderer);
		return false;
	}

	this->fogTexCoordBufferID = renderer.createVertexAttributeBuffer(fogMeshVertexCount, texCoordComponentsPerVertex);
	if (this->fogTexCoordBufferID < 0)
	{
		DebugLogError("Couldn't create tex coord attribute buffer for fog mesh def.");
		this->freeParticleBuffers(renderer);
		this->freeFogBuffers(renderer);
		return false;
	}

	this->fogIndexBufferID = renderer.createIndexBuffer(fogMeshIndexCount);
	if (this->fogIndexBufferID < 0)
	{
		DebugLogError("Couldn't create index buffer for fog mesh def.");
		this->freeParticleBuffers(renderer);
		this->freeFogBuffers(renderer);
		return false;
	}

	renderer.populateVertexPositionBuffer(this->fogPositionBufferID, fogPositions);
	renderer.populateVertexAttributeBuffer(this->fogNormalBufferID, fogNormals);
	renderer.populateVertexAttributeBuffer(this->fogTexCoordBufferID, fogTexCoords);
	renderer.populateIndexBuffer(this->fogIndexBufferID, fogIndices);

	return true;
}

bool RenderWeatherManager::initUniforms(Renderer &renderer)
{
	// Initialize rain and snow buffers but don't populate because they are updated every frame.
	this->rainTransformBufferID = renderer.createUniformBuffer(ArenaWeatherUtils::RAINDROP_TOTAL_COUNT, sizeof(RenderTransform), alignof(RenderTransform));
	if (this->rainTransformBufferID < 0)
	{
		DebugLogError("Couldn't create uniform buffer for raindrops.");
		return false;
	}

	this->snowTransformBufferID = renderer.createUniformBuffer(ArenaWeatherUtils::SNOWFLAKE_TOTAL_COUNT, sizeof(RenderTransform), alignof(RenderTransform));
	if (this->snowTransformBufferID < 0)
	{
		DebugLogError("Couldn't create uniform buffer for snowflakes.");
		return false;
	}

	// Fog is not updated every frame so it needs populating here.
	this->fogTransformBufferID = renderer.createUniformBuffer(1, sizeof(RenderTransform), alignof(RenderTransform));
	if (this->fogTransformBufferID < 0)
	{
		DebugLogError("Couldn't create uniform buffer for fog.");
		return false;
	}

	RenderTransform fogRenderTransform;
	fogRenderTransform.translation = Matrix4d::identity();
	fogRenderTransform.rotation = Matrix4d::identity();
	fogRenderTransform.scale = Matrix4d::identity();
	renderer.populateUniformBuffer(this->fogTransformBufferID, fogRenderTransform);

	return true;
}

bool RenderWeatherManager::initTextures(Renderer &renderer)
{
	// Init rain texture.
	constexpr int rainTexelCount = RainTextureWidth * RainTextureHeight;
	this->rainTextureID = renderer.createObjectTexture(RainTextureWidth, RainTextureHeight, BytesPerTexel);
	if (this->rainTextureID < 0)
	{
		DebugLogError("Couldn't create rain object texture.");
		this->freeParticleBuffers(renderer);
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
	for (int i = 0; i < static_cast<int>(std::size(this->snowTextureIDs)); i++)
	{
		const int snowTextureWidth = GetSnowTextureWidth(i);
		const int snowTextureHeight = GetSnowTextureHeight(i);
		const int snowTexelCount = snowTextureWidth * snowTextureHeight;
		ObjectTextureID &snowTextureID = this->snowTextureIDs[i];
		snowTextureID = renderer.createObjectTexture(snowTextureWidth, snowTextureHeight, BytesPerTexel);
		if (snowTextureID < 0)
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
	constexpr int fogTextureWidth = 2;// ArenaRenderUtils::FOG_MATRIX_WIDTH;
	constexpr int fogTextureHeight = 2;// ArenaRenderUtils::FOG_MATRIX_HEIGHT;
	constexpr int fogTexelCount = fogTextureWidth * fogTextureHeight;
	this->fogTextureID = renderer.createObjectTexture(fogTextureWidth, fogTextureHeight, BytesPerTexel);
	if (this->fogTextureID < 0)
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

	const uint8_t tempFogTexelColors[] = { 5, 6, 7, 8 };
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

	if (!this->initUniforms(renderer))
	{
		DebugLogError("Couldn't init all weather uniform buffers.");
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

void RenderWeatherManager::populateCommandBuffer(RenderCommandBuffer &commandBuffer, const WeatherInstance &weatherInst, bool isFoggy) const
{
	if (weatherInst.hasFog() && isFoggy)
	{
		commandBuffer.addDrawCalls(Span<const RenderDrawCall>(&this->fogDrawCall, 1));
	}

	if (weatherInst.hasRain())
	{
		commandBuffer.addDrawCalls(this->rainDrawCalls);
	}

	if (weatherInst.hasSnow())
	{
		commandBuffer.addDrawCalls(this->snowDrawCalls);
	}
}

void RenderWeatherManager::freeParticleBuffers(Renderer &renderer)
{
	if (this->particlePositionBufferID >= 0)
	{
		renderer.freeVertexPositionBuffer(this->particlePositionBufferID);
		this->particlePositionBufferID = -1;
	}

	if (this->particleNormalBufferID >= 0)
	{
		renderer.freeVertexAttributeBuffer(this->particleNormalBufferID);
		this->particleNormalBufferID = -1;
	}

	if (this->particleTexCoordBufferID >= 0)
	{
		renderer.freeVertexAttributeBuffer(this->particleTexCoordBufferID);
		this->particleTexCoordBufferID = -1;
	}

	if (this->particleIndexBufferID >= 0)
	{
		renderer.freeIndexBuffer(this->particleIndexBufferID);
		this->particleIndexBufferID = -1;
	}

	if (this->rainTransformBufferID >= 0)
	{
		renderer.freeUniformBuffer(this->rainTransformBufferID);
		this->rainTransformBufferID = -1;
	}

	if (this->rainTextureID >= 0)
	{
		renderer.freeObjectTexture(this->rainTextureID);
		this->rainTextureID = -1;
	}

	if (this->snowTransformBufferID >= 0)
	{
		renderer.freeUniformBuffer(this->snowTransformBufferID);
		this->snowTransformBufferID = -1;
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
	if (this->fogPositionBufferID >= 0)
	{
		renderer.freeVertexPositionBuffer(this->fogPositionBufferID);
		this->fogPositionBufferID = -1;
	}

	if (this->fogNormalBufferID >= 0)
	{
		renderer.freeVertexAttributeBuffer(this->fogNormalBufferID);
		this->fogNormalBufferID = -1;
	}

	if (this->fogTexCoordBufferID >= 0)
	{
		renderer.freeVertexAttributeBuffer(this->fogTexCoordBufferID);
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

void RenderWeatherManager::loadScene()
{
	// @todo: load draw calls here instead of update() for optimization. take weatherdef/weatherinst parameter so we know what to enable
}

void RenderWeatherManager::update(const WeatherInstance &weatherInst, const RenderCamera &camera, Renderer &renderer)
{
	this->rainDrawCalls.clear();
	this->snowDrawCalls.clear();
	this->fogDrawCall.clear();

	// @todo: this isn't doing anything that changes per-frame now, move it to loadScene()
	auto populateParticleDrawCall = [this](RenderDrawCall &drawCall, UniformBufferID transformBufferID, int transformIndex, ObjectTextureID textureID)
	{
		drawCall.transformBufferID = transformBufferID;
		drawCall.transformIndex = transformIndex;
		drawCall.preScaleTranslationBufferID = -1;
		drawCall.positionBufferID = this->particlePositionBufferID;
		drawCall.normalBufferID = this->particleNormalBufferID;
		drawCall.texCoordBufferID = this->particleTexCoordBufferID;
		drawCall.indexBufferID = this->particleIndexBufferID;
		drawCall.textureIDs[0] = textureID;
		drawCall.textureIDs[1] = -1;
		drawCall.lightingType = RenderLightingType::PerMesh;
		drawCall.lightPercent = 1.0;
		drawCall.lightIdCount = 0;
		drawCall.vertexShaderType = VertexShaderType::Basic; // @todo: actually have a dedicated vertex shader?
		drawCall.pixelShaderType = PixelShaderType::AlphaTested;
		drawCall.pixelShaderParam0 = 0.0;
		drawCall.enableBackFaceCulling = false;
		drawCall.enableDepthRead = false;
		drawCall.enableDepthWrite = false;
	};

	auto populateRainDrawCall = [this, &populateParticleDrawCall](RenderDrawCall &drawCall, int transformIndex)
	{
		populateParticleDrawCall(drawCall, this->rainTransformBufferID, transformIndex, this->rainTextureID);
	};

	auto populateSnowDrawCall = [this, &populateParticleDrawCall](RenderDrawCall &drawCall, int transformIndex, ObjectTextureID textureID)
	{
		populateParticleDrawCall(drawCall, this->snowTransformBufferID, transformIndex, textureID);
	};

	const Matrix4d particleRotationMatrix = MakeParticleRotationMatrix(camera.yaw, camera.pitch);

	if (weatherInst.hasRain())
	{
		const WeatherRainInstance &rainInst = weatherInst.getRain();
		const Span<const WeatherParticle> rainParticles = rainInst.particles;
		const int rainParticleCount = rainParticles.getCount();
		DebugAssert(rainParticleCount == ArenaWeatherUtils::RAINDROP_TOTAL_COUNT);

		if (this->rainDrawCalls.getCount() != rainParticleCount)
		{
			this->rainDrawCalls.init(rainParticleCount);
		}

		const Matrix4d raindropScaleMatrix = MakeParticleScaleMatrix(RainTextureWidth, RainTextureHeight);
		for (int i = 0; i < rainParticleCount; i++)
		{
			const WeatherParticle &rainParticle = rainInst.particles[i];
			const int raindropTransformIndex = i;

			RenderTransform raindropRenderTransform;
			raindropRenderTransform.translation = MakeParticleTranslationMatrix(camera, rainParticle.xPercent, rainParticle.yPercent);
			raindropRenderTransform.rotation = particleRotationMatrix;
			raindropRenderTransform.scale = raindropScaleMatrix;
			renderer.populateUniformAtIndex(this->rainTransformBufferID, raindropTransformIndex, raindropRenderTransform);

			populateRainDrawCall(this->rainDrawCalls[i], raindropTransformIndex);
		}
	}

	if (weatherInst.hasSnow())
	{
		const WeatherSnowInstance &snowInst = weatherInst.getSnow();
		const Span<const WeatherParticle> snowParticles = snowInst.particles;
		const int snowParticleCount = snowParticles.getCount();

		if (this->snowDrawCalls.getCount() != snowParticleCount)
		{
			this->snowDrawCalls.init(snowParticleCount);
		}

		constexpr int fastSnowParticleCount = ArenaWeatherUtils::SNOWFLAKE_FAST_COUNT;
		constexpr int mediumSnowParticleCount = ArenaWeatherUtils::SNOWFLAKE_MEDIUM_COUNT;
		constexpr int slowSnowParticleCount = ArenaWeatherUtils::SNOWFLAKE_SLOW_COUNT;
		DebugAssert((fastSnowParticleCount + mediumSnowParticleCount + slowSnowParticleCount) == snowParticleCount);

		constexpr int snowParticleTypeCount = ArenaWeatherUtils::SNOWFLAKE_TYPE_COUNT;
		constexpr int snowParticleStarts[snowParticleTypeCount] =
		{
			0,
			fastSnowParticleCount,
			fastSnowParticleCount + mediumSnowParticleCount
		};

		constexpr int snowParticleEnds[snowParticleTypeCount] =
		{
			fastSnowParticleCount,
			fastSnowParticleCount + mediumSnowParticleCount,
			fastSnowParticleCount + mediumSnowParticleCount + slowSnowParticleCount
		};

		for (int snowParticleSizeIndex = 0; snowParticleSizeIndex < snowParticleTypeCount; snowParticleSizeIndex++)
		{
			const int snowParticleStart = snowParticleStarts[snowParticleSizeIndex];
			const int snowParticleEnd = snowParticleEnds[snowParticleSizeIndex];
			const int snowParticleTextureWidth = GetSnowTextureWidth(snowParticleSizeIndex);
			const int snowParticleTextureHeight = GetSnowTextureHeight(snowParticleSizeIndex);
			const Matrix4d snowParticleScaleMatrix = MakeParticleScaleMatrix(snowParticleTextureWidth, snowParticleTextureHeight);

			for (int i = snowParticleStart; i < snowParticleEnd; i++)
			{
				const WeatherParticle &snowParticle = snowInst.particles[i];
				const int snowParticleTransformIndex = i;

				RenderTransform snowParticleRenderTransform;
				snowParticleRenderTransform.translation = MakeParticleTranslationMatrix(camera, snowParticle.xPercent, snowParticle.yPercent);
				snowParticleRenderTransform.rotation = particleRotationMatrix;
				snowParticleRenderTransform.scale = snowParticleScaleMatrix;
				renderer.populateUniformAtIndex(this->snowTransformBufferID, snowParticleTransformIndex, snowParticleRenderTransform);

				const ObjectTextureID snowParticleTextureID = this->snowTextureIDs[snowParticleSizeIndex];
				populateSnowDrawCall(this->snowDrawCalls[i], snowParticleTransformIndex, snowParticleTextureID);
			}
		}
	}

	if (weatherInst.hasFog())
	{
		RenderTransform fogRenderTransform;
		fogRenderTransform.translation = Matrix4d::translation(camera.worldPoint.x, camera.worldPoint.y, camera.worldPoint.z);
		fogRenderTransform.rotation = Matrix4d::identity();
		fogRenderTransform.scale = Matrix4d::identity();
		renderer.populateUniformBuffer(this->fogTransformBufferID, fogRenderTransform);

		this->fogDrawCall.transformBufferID = this->fogTransformBufferID;
		this->fogDrawCall.transformIndex = 0;
		this->fogDrawCall.preScaleTranslationBufferID = -1;
		this->fogDrawCall.positionBufferID = this->fogPositionBufferID;
		this->fogDrawCall.normalBufferID = this->fogNormalBufferID;
		this->fogDrawCall.texCoordBufferID = this->fogTexCoordBufferID;
		this->fogDrawCall.indexBufferID = this->fogIndexBufferID;
		this->fogDrawCall.textureIDs[0] = this->fogTextureID;
		this->fogDrawCall.textureIDs[1] = -1;
		this->fogDrawCall.lightingType = RenderLightingType::PerMesh;
		this->fogDrawCall.lightPercent = 1.0;
		this->fogDrawCall.lightIdCount = 0;
		this->fogDrawCall.vertexShaderType = VertexShaderType::Basic;
		this->fogDrawCall.pixelShaderType = PixelShaderType::AlphaTestedWithLightLevelOpacity; // @todo: don't depth test
		this->fogDrawCall.pixelShaderParam0 = 0.0;
		this->fogDrawCall.enableBackFaceCulling = false;
		this->fogDrawCall.enableDepthRead = false;
		this->fogDrawCall.enableDepthWrite = false;
	}
}

void RenderWeatherManager::unloadScene()
{
	this->rainDrawCalls.clear();
	this->snowDrawCalls.clear();
	this->fogDrawCall.clear();
}
