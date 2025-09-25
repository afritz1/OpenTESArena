#include <algorithm>

#include "RenderCamera.h"
#include "RenderCommand.h"
#include "Renderer.h"
#include "RendererUtils.h"
#include "RenderWeatherManager.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Math/Constants.h"
#include "../Weather/ArenaWeatherUtils.h"
#include "../Weather/WeatherInstance.h"
#include "../World/MeshUtils.h"

namespace
{
	constexpr int RainTextureWidth = ArenaRenderUtils::RAINDROP_TEXTURE_WIDTH;
	constexpr int RainTextureHeight = ArenaRenderUtils::RAINDROP_TEXTURE_HEIGHT;
	constexpr int BytesPerTexel = 1;

	constexpr double ParticleZDistance = 1.0; // Arbitrary distance from camera, depth is ignored.
	static_assert(ParticleZDistance > RendererUtils::NEAR_PLANE);

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
		const Double3 basePosition = camera.floatingWorldPoint;
		const Double3 centerDir = camera.forwardScaled * ParticleZDistance;
		const Double3 rightDir = camera.rightScaled * ParticleZDistance;
		const Double3 upDir = camera.upScaled * ParticleZDistance;
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
		const double scaledWidth = baseWidth * ParticleZDistance;
		const double scaledHeight = baseHeight * ParticleZDistance;
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
	this->rainMaterialID = -1;

	this->snowTransformBufferID = -1;
	for (ObjectTextureID &textureID : this->snowTextureIDs)
	{
		textureID = -1;
	}

	for (RenderMaterialID &materialID : this->snowMaterialIDs)
	{
		materialID = -1;
	}

	this->fogPositionBufferID = -1;
	this->fogNormalBufferID = -1;
	this->fogTexCoordBufferID = -1;
	this->fogIndexBufferID = -1;
	this->fogTransformBufferID = -1;
	this->fogTextureID = -1;
	this->fogMaterialID = -1;

	this->materialInstID = -1;
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
		-0.5, 0.35, 0.5,
		-0.5, -0.35, 0.5,
		-0.5, -0.35, -0.5,
		-0.5, 0.35, -0.5,
		// X=1
		0.5, 0.35, -0.5,
		0.5, -0.35, -0.5,
		0.5, -0.35, 0.5,
		0.5, 0.35, 0.5,
		// Y=0
		-0.5, -0.35, 0.5,
		0.5, -0.35, 0.5,
		0.5, -0.35, -0.5,
		-0.5, -0.35, -0.5,
		// Y=1
		-0.5, 0.35, -0.5,
		0.5, 0.35, -0.5,
		0.5, 0.35, 0.5,
		-0.5, 0.35, 0.5,
		// Z=0
		-0.5, 0.35, -0.5,
		-0.5, -0.35, -0.5,
		0.5, -0.35, -0.5,
		0.5, 0.35, -0.5,
		// Z=1
		0.5, 0.35, 0.5,
		0.5, -0.35, 0.5,
		-0.5, -0.35, 0.5,
		-0.5, 0.35, 0.5
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
	this->rainTransformBufferID = renderer.createUniformBufferMatrix4s(ArenaWeatherUtils::RAINDROP_TOTAL_COUNT);
	if (this->rainTransformBufferID < 0)
	{
		DebugLogError("Couldn't create uniform buffer for raindrops.");
		return false;
	}

	this->snowTransformBufferID = renderer.createUniformBufferMatrix4s(ArenaWeatherUtils::SNOWFLAKE_TOTAL_COUNT);
	if (this->snowTransformBufferID < 0)
	{
		DebugLogError("Couldn't create uniform buffer for snowflakes.");
		return false;
	}

	// Fog is not updated every frame so it needs populating here.
	this->fogTransformBufferID = renderer.createUniformBufferMatrix4s(1);
	if (this->fogTransformBufferID < 0)
	{
		DebugLogError("Couldn't create uniform buffer for fog.");
		return false;
	}

	const Matrix4d fogModelMatrix = Matrix4d::identity();
	renderer.populateUniformBufferMatrix4s(this->fogTransformBufferID, Span<const Matrix4d>(&fogModelMatrix, 1));

	return true;
}

bool RenderWeatherManager::initTextures(TextureManager &textureManager, Renderer &renderer)
{
	// Init rain texture.
	this->rainTextureID = renderer.createObjectTexture(RainTextureWidth, RainTextureHeight, BytesPerTexel);
	if (this->rainTextureID < 0)
	{
		DebugLogError("Couldn't create rain object texture.");
		this->freeParticleBuffers(renderer);
		return false;
	}

	if (!renderer.populateObjectTexture8Bit(this->rainTextureID, ArenaRenderUtils::RAINDROP_TEXELS))
	{
		DebugLogError("Couldn't populate rain object texture.");
	}

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
			DebugLogErrorFormat("Couldn't create snow object texture %d.", i);
			this->freeParticleBuffers(renderer);
			return false;
		}

		Span<const uint8_t> srcSnowTexels(ArenaRenderUtils::SNOWFLAKE_TEXELS_PTRS[i], snowTexelCount);
		if (!renderer.populateObjectTexture8Bit(snowTextureID, srcSnowTexels))
		{
			DebugLogError("Couldn't populate snow object texture.");
		}
	}

	this->fogState.init(textureManager);

	// Init fog texture.
	constexpr int fogTextureWidth = ArenaRenderUtils::SCREEN_WIDTH;
	constexpr int fogTextureHeight = ArenaRenderUtils::SCREEN_HEIGHT;
	this->fogTextureID = renderer.createObjectTexture(fogTextureWidth, fogTextureHeight, BytesPerTexel);
	if (this->fogTextureID < 0)
	{
		DebugLogError("Couldn't create fog object texture.");
		return false;
	}

	return true;
}

bool RenderWeatherManager::initMaterials(Renderer &renderer)
{
	constexpr VertexShaderType vertexShaderType = VertexShaderType::Basic;
	constexpr RenderLightingType lightingType = RenderLightingType::PerMesh;

	RenderMaterialKey rainMaterialKey;
	rainMaterialKey.init(vertexShaderType, FragmentShaderType::AlphaTested, Span<const ObjectTextureID>(&this->rainTextureID, 1), lightingType, false, false, false);
	this->rainMaterialID = renderer.createMaterial(rainMaterialKey);

	for (int i = 0; i < static_cast<int>(std::size(this->snowMaterialIDs)); i++)
	{
		RenderMaterialKey snowMaterialKey;
		snowMaterialKey.init(vertexShaderType, FragmentShaderType::AlphaTested, Span<const ObjectTextureID>(&this->snowTextureIDs[i], 1), lightingType, false, false, false);
		this->snowMaterialIDs[i] = renderer.createMaterial(snowMaterialKey);
	}

	RenderMaterialKey fogMaterialKey;
	fogMaterialKey.init(vertexShaderType, FragmentShaderType::AlphaTestedWithLightLevelOpacity, Span<const ObjectTextureID>(&this->fogTextureID, 1), lightingType, false, false, false);
	this->fogMaterialID = renderer.createMaterial(fogMaterialKey);

	this->materialInstID = renderer.createMaterialInstance();
	renderer.setMaterialInstanceMeshLightPercent(this->materialInstID, 1.0);

	return true;
}

bool RenderWeatherManager::init(TextureManager &textureManager, Renderer &renderer)
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

	if (!this->initTextures(textureManager, renderer))
	{
		DebugLogError("Couldn't init all weather textures.");
		return false;
	}

	if (!this->initMaterials(renderer))
	{
		DebugLogError("Couldn't init all weather materials.");
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

	if (this->materialInstID >= 0)
	{
		renderer.freeMaterialInstance(this->materialInstID);
		this->materialInstID = -1;
	}
}

void RenderWeatherManager::populateCommandList(RenderCommandList &commandList, const WeatherInstance &weatherInst, bool isFoggy) const
{
	if (weatherInst.hasFog() && isFoggy)
	{
		commandList.addDrawCalls(Span<const RenderDrawCall>(&this->fogDrawCall, 1));
	}

	if (weatherInst.hasRain())
	{
		commandList.addDrawCalls(this->rainDrawCalls);
	}

	if (weatherInst.hasSnow())
	{
		commandList.addDrawCalls(this->snowDrawCalls);
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

	if (this->rainMaterialID >= 0)
	{
		renderer.freeMaterial(this->rainMaterialID);
		this->rainMaterialID = -1;
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

	for (RenderMaterialID &snowMaterialID : this->snowMaterialIDs)
	{
		if (snowMaterialID >= 0)
		{
			renderer.freeMaterial(snowMaterialID);
			snowMaterialID = -1;
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

	if (this->fogMaterialID >= 0)
	{
		renderer.freeMaterial(this->fogMaterialID);
		this->fogMaterialID = -1;
	}
}

void RenderWeatherManager::loadScene()
{
	// @todo: load draw calls here instead of update() for optimization. take weatherdef/weatherinst parameter so we know what to enable
}

void RenderWeatherManager::update(double dt, const WeatherInstance &weatherInst, const RenderCamera &camera, const Double2 &playerDirXZ, MapType mapType, Renderer &renderer)
{
	this->rainDrawCalls.clear();
	this->snowDrawCalls.clear();
	this->fogDrawCall.clear();

	// @todo: this isn't doing anything that changes per-frame now, move it to loadScene()
	auto populateParticleDrawCall = [this, &renderer](RenderDrawCall &drawCall, UniformBufferID transformBufferID, int transformIndex, RenderMaterialID materialID)
	{
		drawCall.transformBufferID = transformBufferID;
		drawCall.transformIndex = transformIndex;
		drawCall.positionBufferID = this->particlePositionBufferID;
		drawCall.normalBufferID = this->particleNormalBufferID;
		drawCall.texCoordBufferID = this->particleTexCoordBufferID;
		drawCall.indexBufferID = this->particleIndexBufferID;
		drawCall.materialID = materialID;
		drawCall.materialInstID = this->materialInstID;
		drawCall.multipassType = RenderMultipassType::None;
	};

	auto populateRainDrawCall = [this, &populateParticleDrawCall](RenderDrawCall &drawCall, int transformIndex)
	{
		populateParticleDrawCall(drawCall, this->rainTransformBufferID, transformIndex, this->rainMaterialID);
	};

	auto populateSnowDrawCall = [this, &populateParticleDrawCall](RenderDrawCall &drawCall, int transformIndex, RenderMaterialID materialID)
	{
		populateParticleDrawCall(drawCall, this->snowTransformBufferID, transformIndex, materialID);
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

			const Matrix4d raindropTranslationMatrix = MakeParticleTranslationMatrix(camera, rainParticle.xPercent, rainParticle.yPercent);
			const Matrix4d raindropModelMatrix = raindropTranslationMatrix * (particleRotationMatrix * raindropScaleMatrix);
			renderer.populateUniformBufferIndexMatrix4(this->rainTransformBufferID, raindropTransformIndex, raindropModelMatrix);

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

				const Matrix4d snowParticleTranslationMatrix = MakeParticleTranslationMatrix(camera, snowParticle.xPercent, snowParticle.yPercent);
				const Matrix4d snowParticleModelMatrix = snowParticleTranslationMatrix * (particleRotationMatrix * snowParticleScaleMatrix);
				renderer.populateUniformBufferIndexMatrix4(this->snowTransformBufferID, snowParticleTransformIndex, snowParticleModelMatrix);

				const RenderMaterialID snowParticleMaterialID = this->snowMaterialIDs[snowParticleSizeIndex];
				populateSnowDrawCall(this->snowDrawCalls[i], snowParticleTransformIndex, snowParticleMaterialID);
			}
		}
	}

	if (weatherInst.hasFog())
	{
		this->fogState.update(dt, camera.worldPoint, playerDirXZ, mapType);

		LockedTexture lockedFogTexture = renderer.lockObjectTexture(this->fogTextureID);
		if (!lockedFogTexture.isValid())
		{
			DebugLogError("Couldn't lock fog texture for updating.");
		}

		Span2D<uint8_t> dstFogTexels = lockedFogTexture.getTexels8();
		ArenaRenderUtils::populateFogTexture(this->fogState, dstFogTexels);

		renderer.unlockObjectTexture(this->fogTextureID);

		const Matrix4d fogModelMatrix = Matrix4d::translation(camera.floatingWorldPoint.x, camera.floatingWorldPoint.y, camera.floatingWorldPoint.z);
		renderer.populateUniformBufferMatrix4s(this->fogTransformBufferID, Span<const Matrix4d>(&fogModelMatrix, 1));

		this->fogDrawCall.transformBufferID = this->fogTransformBufferID;
		this->fogDrawCall.transformIndex = 0;
		this->fogDrawCall.positionBufferID = this->fogPositionBufferID;
		this->fogDrawCall.normalBufferID = this->fogNormalBufferID;
		this->fogDrawCall.texCoordBufferID = this->fogTexCoordBufferID;
		this->fogDrawCall.indexBufferID = this->fogIndexBufferID;
		this->fogDrawCall.materialID = this->fogMaterialID;
		this->fogDrawCall.materialInstID = this->materialInstID;
		this->fogDrawCall.multipassType = RenderMultipassType::Ghosts;
	}
}

void RenderWeatherManager::unloadScene()
{
	this->rainDrawCalls.clear();
	this->snowDrawCalls.clear();
	this->fogDrawCall.clear();
}
