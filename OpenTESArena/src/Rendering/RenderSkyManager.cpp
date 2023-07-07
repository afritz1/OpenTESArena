#include <numeric>
#include <vector>

#include "ArenaRenderUtils.h"
#include "Renderer.h"
#include "RenderSkyManager.h"
#include "../Assets/TextureManager.h"
#include "../Math/Constants.h"
#include "../Math/Vector3.h"
#include "../Sky/SkyDefinition.h"
#include "../Sky/SkyInfoDefinition.h"
#include "../Sky/SkyInstance.h"
#include "../World/MeshUtils.h"

RenderSkyManager::LoadedSkyObjectTexture::LoadedSkyObjectTexture()
{
	this->type = static_cast<LoadedSkyObjectTextureType>(-1);
	this->paletteIndex = 0;
}

void RenderSkyManager::LoadedSkyObjectTexture::initPaletteIndex(uint8_t paletteIndex, ScopedObjectTextureRef &&objectTextureRef)
{
	this->type = LoadedSkyObjectTextureType::PaletteIndex;
	this->paletteIndex = paletteIndex;
	this->objectTextureRef = std::move(objectTextureRef);
}

void RenderSkyManager::LoadedSkyObjectTexture::initTextureAsset(const TextureAsset &textureAsset, ScopedObjectTextureRef &&objectTextureRef)
{
	this->type = LoadedSkyObjectTextureType::TextureAsset;
	this->textureAsset = textureAsset;
	this->objectTextureRef = std::move(objectTextureRef);
}

RenderSkyManager::RenderSkyManager()
{
	this->bgVertexBufferID = -1;
	this->bgNormalBufferID = -1;
	this->bgTexCoordBufferID = -1;
	this->bgIndexBufferID = -1;
	this->bgObjectTextureID = -1;

	this->objectVertexBufferID = -1;
	this->objectNormalBufferID = -1;
	this->objectTexCoordBufferID = -1;
	this->objectIndexBufferID = -1;
}

void RenderSkyManager::init(Renderer &renderer)
{
	std::vector<double> bgVertices;
	std::vector<double> bgNormals;
	std::vector<double> bgTexCoords;
	std::vector<int32_t> bgIndices;

	const double pointDistance = 1000.0; // @todo: this is a hack while the sky is using naive depth testing w/o any occlusion culling, etc.

	constexpr int zenithVertexIndex = 0;
	constexpr int nadirVertexIndex = 1;
	const Double3 zenithPoint(0.0, 1.0 * pointDistance, 0.0);
	const Double3 nadirPoint(0.0, -1.0 * pointDistance, 0.0);
	bgVertices.emplace_back(zenithPoint.x);
	bgVertices.emplace_back(zenithPoint.y);
	bgVertices.emplace_back(zenithPoint.z);
	bgVertices.emplace_back(nadirPoint.x);
	bgVertices.emplace_back(nadirPoint.y);
	bgVertices.emplace_back(nadirPoint.z);

	const Double3 zenithNormal = -zenithPoint.normalized();
	const Double3 nadirNormal = -nadirPoint.normalized();
	bgNormals.emplace_back(zenithNormal.x);
	bgNormals.emplace_back(zenithNormal.y);
	bgNormals.emplace_back(zenithNormal.z);
	bgNormals.emplace_back(nadirNormal.x);
	bgNormals.emplace_back(nadirNormal.y);
	bgNormals.emplace_back(nadirNormal.z);

	const Double2 zenithTexCoord(0.50, 0.0);
	const Double2 nadirTexCoord(0.50, 1.0);
	bgTexCoords.emplace_back(zenithTexCoord.x);
	bgTexCoords.emplace_back(zenithTexCoord.y);
	bgTexCoords.emplace_back(nadirTexCoord.x);
	bgTexCoords.emplace_back(nadirTexCoord.y);

	constexpr int bgAboveHorizonTriangleCount = 16; // Arbitrary number of triangles, increases smoothness of cone shape.
	for (int i = 0; i < bgAboveHorizonTriangleCount; i++)
	{
		// Generate two triangles: one above horizon, one below.
		const double percent = static_cast<double>(i) / static_cast<double>(bgAboveHorizonTriangleCount);
		const double nextPercent = static_cast<double>(i + 1) / static_cast<double>(bgAboveHorizonTriangleCount);
		const double period = percent * Constants::TwoPi;
		const double nextPeriod = nextPercent * Constants::TwoPi;

		const Double3 point(std::cos(period) * pointDistance, 0.0, std::sin(period) * pointDistance);
		const Double3 nextPoint(std::cos(nextPeriod) * pointDistance, 0.0, std::sin(nextPeriod) * pointDistance);

		bgVertices.emplace_back(point.x);
		bgVertices.emplace_back(point.y);
		bgVertices.emplace_back(point.z);
		bgVertices.emplace_back(nextPoint.x);
		bgVertices.emplace_back(nextPoint.y);
		bgVertices.emplace_back(nextPoint.z);

		// Normals point toward the player.
		const Double3 normal = -point.normalized();
		const Double3 nextNormal = -nextPoint.normalized();
		bgNormals.emplace_back(normal.x);
		bgNormals.emplace_back(normal.y);
		bgNormals.emplace_back(normal.z);
		bgNormals.emplace_back(nextNormal.x);
		bgNormals.emplace_back(nextNormal.y);
		bgNormals.emplace_back(nextNormal.z);

		const Double2 texCoord(1.0, 1.0);
		const Double2 nextTexCoord(0.0, 1.0);
		bgTexCoords.emplace_back(texCoord.x);
		bgTexCoords.emplace_back(texCoord.y);
		bgTexCoords.emplace_back(nextTexCoord.x);
		bgTexCoords.emplace_back(nextTexCoord.y);

		// Above-horizon winding: next -> cur -> zenith
		const int32_t vertexIndex = static_cast<int32_t>((bgVertices.size() / 3) - 2);
		const int32_t nextVertexIndex = static_cast<int32_t>((bgVertices.size() / 3) - 1);
		bgIndices.emplace_back(nextVertexIndex);
		bgIndices.emplace_back(vertexIndex);
		bgIndices.emplace_back(zenithVertexIndex);

		// Below-horizon winding: cur -> next -> nadir
		bgIndices.emplace_back(vertexIndex);
		bgIndices.emplace_back(nextVertexIndex);
		bgIndices.emplace_back(nadirVertexIndex);
	}

	constexpr int positionComponentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
	constexpr int normalComponentsPerVertex = MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
	constexpr int texCoordComponentsPerVertex = MeshUtils::TEX_COORDS_PER_VERTEX;

	const int bgVertexCount = static_cast<int>(bgVertices.size()) / 3;
	if (!renderer.tryCreateVertexBuffer(bgVertexCount, positionComponentsPerVertex, &this->bgVertexBufferID))
	{
		DebugLogError("Couldn't create vertex buffer for sky background mesh ID.");
		return;
	}

	if (!renderer.tryCreateAttributeBuffer(bgVertexCount, normalComponentsPerVertex, &this->bgNormalBufferID))
	{
		DebugLogError("Couldn't create normal attribute buffer for sky background mesh ID.");
		this->freeBgBuffers(renderer);
		return;
	}

	if (!renderer.tryCreateAttributeBuffer(bgVertexCount, texCoordComponentsPerVertex, &this->bgTexCoordBufferID))
	{
		DebugLogError("Couldn't create tex coord attribute buffer for sky background mesh ID.");
		this->freeBgBuffers(renderer);
		return;
	}

	if (!renderer.tryCreateIndexBuffer(static_cast<int>(bgIndices.size()), &this->bgIndexBufferID))
	{
		DebugLogError("Couldn't create index buffer for sky background mesh ID.");
		this->freeBgBuffers(renderer);
		return;
	}

	renderer.populateVertexBuffer(this->bgVertexBufferID, bgVertices);
	renderer.populateAttributeBuffer(this->bgNormalBufferID, bgNormals);
	renderer.populateAttributeBuffer(this->bgTexCoordBufferID, bgTexCoords);
	renderer.populateIndexBuffer(this->bgIndexBufferID, bgIndices);

	BufferView<const uint8_t> bgPaletteIndices = ArenaRenderUtils::PALETTE_INDICES_SKY_COLOR_MORNING;
	constexpr int bgTextureWidth = 1;
	const int bgTextureHeight = bgPaletteIndices.getCount(); // @todo: figure out sky background texture coloring; probably lock+update the main world palette in an update() with DAYTIME.COL indices as times goes on?
	constexpr int bgBytesPerTexel = 1;
	if (!renderer.tryCreateObjectTexture(bgTextureWidth, bgTextureHeight, bgBytesPerTexel, &this->bgObjectTextureID))
	{
		DebugLogError("Couldn't create object texture for sky background texture ID.");
		this->freeBgBuffers(renderer);
		return;
	}

	LockedTexture bgLockedTexture = renderer.lockObjectTexture(this->bgObjectTextureID);
	uint8_t *bgTexels = static_cast<uint8_t*>(bgLockedTexture.texels);
	std::copy(bgPaletteIndices.begin(), bgPaletteIndices.end(), bgTexels);
	renderer.unlockObjectTexture(this->bgObjectTextureID);

	this->bgDrawCall.position = Double3::Zero;
	this->bgDrawCall.preScaleTranslation = Double3::Zero;
	this->bgDrawCall.rotation = Matrix4d::identity();
	this->bgDrawCall.scale = Matrix4d::identity();
	this->bgDrawCall.vertexBufferID = this->bgVertexBufferID;
	this->bgDrawCall.normalBufferID = this->bgNormalBufferID;
	this->bgDrawCall.texCoordBufferID = this->bgTexCoordBufferID;
	this->bgDrawCall.indexBufferID = this->bgIndexBufferID;
	this->bgDrawCall.textureIDs[0] = this->bgObjectTextureID;
	this->bgDrawCall.textureIDs[1] = std::nullopt;
	this->bgDrawCall.textureSamplingType0 = TextureSamplingType::Default;
	this->bgDrawCall.textureSamplingType1 = TextureSamplingType::Default;
	this->bgDrawCall.vertexShaderType = VertexShaderType::Voxel; // @todo: SkyBackground?
	this->bgDrawCall.pixelShaderType = PixelShaderType::Opaque; // @todo?
	this->bgDrawCall.pixelShaderParam0 = 0.0;

	// Initialize sky object mesh buffers shared with all sky objects.
	// @todo: to be more accurate, land/air vertices could rest on the horizon, while star/planet/sun vertices would sit halfway under the horizon, etc., and these would be separate buffers for the draw calls to pick from.
	constexpr int objectMeshVertexCount = 4;
	constexpr int objectMeshIndexCount = 6;
	if (!renderer.tryCreateVertexBuffer(objectMeshVertexCount, positionComponentsPerVertex, &this->objectVertexBufferID))
	{
		DebugLogError("Couldn't create vertex buffer for sky object mesh ID.");
		return;
	}

	if (!renderer.tryCreateAttributeBuffer(objectMeshVertexCount, normalComponentsPerVertex, &this->objectNormalBufferID))
	{
		DebugLogError("Couldn't create normal attribute buffer for sky object mesh def.");
		this->freeObjectBuffers(renderer);
		return;
	}

	if (!renderer.tryCreateAttributeBuffer(objectMeshVertexCount, texCoordComponentsPerVertex, &this->objectTexCoordBufferID))
	{
		DebugLogError("Couldn't create tex coord attribute buffer for sky object mesh def.");
		this->freeObjectBuffers(renderer);
		return;
	}

	if (!renderer.tryCreateIndexBuffer(objectMeshIndexCount, &this->objectIndexBufferID))
	{
		DebugLogError("Couldn't create index buffer for sky object mesh def.");
		this->freeObjectBuffers(renderer);
		return;
	}

	constexpr std::array<double, objectMeshVertexCount * positionComponentsPerVertex> objectVertices =
	{
		0.0, 1.0, -0.50,
		0.0, 0.0, -0.50,
		0.0, 0.0, 0.50,
		0.0, 1.0, 0.50
	};

	constexpr std::array<double, objectMeshVertexCount * normalComponentsPerVertex> objectNormals =
	{
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0
	};

	constexpr std::array<double, objectMeshVertexCount * texCoordComponentsPerVertex> objectTexCoords =
	{
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	constexpr std::array<int32_t, objectMeshIndexCount> objectIndices =
	{
		0, 1, 2,
		2, 3, 0
	};

	renderer.populateVertexBuffer(this->objectVertexBufferID, objectVertices);
	renderer.populateAttributeBuffer(this->objectNormalBufferID, objectNormals);
	renderer.populateAttributeBuffer(this->objectTexCoordBufferID, objectTexCoords);
	renderer.populateIndexBuffer(this->objectIndexBufferID, objectIndices);
}

void RenderSkyManager::shutdown(Renderer &renderer)
{
	this->freeBgBuffers(renderer);
	this->bgDrawCall.clear();

	this->freeObjectBuffers(renderer);
	this->objectDrawCalls.clear();
}

ObjectTextureID RenderSkyManager::getSkyObjectTextureID(const TextureAsset &textureAsset) const
{
	const auto iter = std::find_if(this->objectTextures.begin(), this->objectTextures.end(),
		[&textureAsset](const LoadedSkyObjectTexture &loadedTexture)
	{
		return loadedTexture.textureAsset == textureAsset;
	});

	if (iter == this->objectTextures.end())
	{
		DebugLogError("Couldn't find loaded sky object texture for \"" + textureAsset.filename + "\".");
		return -1;
	}

	return iter->objectTextureRef.get();
}

void RenderSkyManager::freeBgBuffers(Renderer &renderer)
{
	if (this->bgVertexBufferID >= 0)
	{
		renderer.freeVertexBuffer(this->bgVertexBufferID);
		this->bgVertexBufferID = -1;
	}

	if (this->bgNormalBufferID >= 0)
	{
		renderer.freeAttributeBuffer(this->bgNormalBufferID);
		this->bgNormalBufferID = -1;
	}

	if (this->bgTexCoordBufferID >= 0)
	{
		renderer.freeAttributeBuffer(this->bgTexCoordBufferID);
		this->bgTexCoordBufferID = -1;
	}

	if (this->bgIndexBufferID >= 0)
	{
		renderer.freeIndexBuffer(this->bgIndexBufferID);
		this->bgIndexBufferID = -1;
	}

	if (this->bgObjectTextureID >= 0)
	{
		renderer.freeObjectTexture(this->bgObjectTextureID);
		this->bgObjectTextureID = -1;
	}
}

void RenderSkyManager::freeObjectBuffers(Renderer &renderer)
{
	if (this->objectVertexBufferID >= 0)
	{
		renderer.freeVertexBuffer(this->objectVertexBufferID);
		this->objectVertexBufferID = -1;
	}

	if (this->objectNormalBufferID >= 0)
	{
		renderer.freeAttributeBuffer(this->objectNormalBufferID);
		this->objectNormalBufferID = -1;
	}

	if (this->objectTexCoordBufferID >= 0)
	{
		renderer.freeAttributeBuffer(this->objectTexCoordBufferID);
		this->objectTexCoordBufferID = -1;
	}

	if (this->objectIndexBufferID >= 0)
	{
		renderer.freeIndexBuffer(this->objectIndexBufferID);
		this->objectIndexBufferID = -1;
	}

	this->objectTextures.clear();
}

RenderDrawCall RenderSkyManager::getBgDrawCall() const
{
	return this->bgDrawCall;
}

BufferView<const RenderDrawCall> RenderSkyManager::getObjectDrawCalls() const
{
	return this->objectDrawCalls;
}

void RenderSkyManager::loadScene(const SkyInstance &skyInst, const SkyInfoDefinition &skyInfoDef, 
	TextureManager &textureManager, Renderer &renderer)
{
	auto tryLoadTextureAsset = [this, &textureManager, &renderer](const TextureAsset &textureAsset)
	{
		const auto iter = std::find_if(this->objectTextures.begin(), this->objectTextures.end(),
			[&textureAsset](const LoadedSkyObjectTexture &loadedTexture)
		{
			return (loadedTexture.type == LoadedSkyObjectTextureType::TextureAsset) && (loadedTexture.textureAsset == textureAsset);
		});

		if (iter == this->objectTextures.end())
		{
			const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
			if (!textureBuilderID.has_value())
			{
				DebugLogError("Couldn't get texture builder ID for sky object texture \"" + textureAsset.filename + "\".");
				return;
			}

			const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
			ObjectTextureID textureID;
			if (!renderer.tryCreateObjectTexture(textureBuilder.getWidth(), textureBuilder.getHeight(), textureBuilder.getBytesPerTexel(), &textureID))
			{
				DebugLogError("Couldn't create object texture for sky object texture \"" + textureAsset.filename + "\".");
				return;
			}

			LoadedSkyObjectTexture loadedTexture;
			loadedTexture.initTextureAsset(textureAsset, ScopedObjectTextureRef(textureID, renderer));
			this->objectTextures.emplace_back(std::move(loadedTexture));
		}
	};

	auto tryLoadPaletteColor = [this, &renderer](uint8_t paletteIndex)
	{
		const auto iter = std::find_if(this->objectTextures.begin(), this->objectTextures.end(),
			[paletteIndex](const LoadedSkyObjectTexture &loadedTexture)
		{
			return (loadedTexture.type == LoadedSkyObjectTextureType::PaletteIndex) && (loadedTexture.paletteIndex == paletteIndex);
		});

		if (iter == this->objectTextures.end())
		{
			constexpr int textureWidth = 1;
			constexpr int textureHeight = textureWidth;
			constexpr int bytesPerTexel = 1;
			ObjectTextureID textureID;
			if (!renderer.tryCreateObjectTexture(textureWidth, textureHeight, bytesPerTexel, &textureID))
			{
				DebugLogError("Couldn't create object texture for sky object texture palette index \"" + std::to_string(paletteIndex) + "\".");
				return;
			}

			LockedTexture lockedTexture = renderer.lockObjectTexture(textureID);
			if (!lockedTexture.isValid())
			{
				DebugLogError("Couldn't lock sky object texture for writing palette index \"" + std::to_string(paletteIndex) + "\".");
				return;
			}

			DebugAssert(lockedTexture.bytesPerTexel == 1);
			uint8_t *dstTexels = static_cast<uint8_t*>(lockedTexture.texels);
			*dstTexels = paletteIndex;
			renderer.unlockObjectTexture(textureID);

			LoadedSkyObjectTexture loadedTexture;
			loadedTexture.initPaletteIndex(paletteIndex, ScopedObjectTextureRef(textureID, renderer));
			this->objectTextures.emplace_back(std::move(loadedTexture));
		}		
	};

	for (int i = 0; i < skyInfoDef.getLandCount(); i++)
	{
		const SkyLandDefinition &landDef = skyInfoDef.getLand(i);
		for (const TextureAsset &textureAsset : landDef.textureAssets)
		{
			tryLoadTextureAsset(textureAsset);
		}
	}

	for (int i = 0; i < skyInfoDef.getAirCount(); i++)
	{
		const SkyAirDefinition &airDef = skyInfoDef.getAir(i);
		tryLoadTextureAsset(airDef.textureAsset);
	}

	for (int i = 0; i < skyInfoDef.getStarCount(); i++)
	{
		const SkyStarDefinition &starDef = skyInfoDef.getStar(i);
		switch (starDef.type)
		{
		case SkyStarType::Small:
		{
			const SkySmallStarDefinition &smallStarDef = starDef.smallStar;
			tryLoadPaletteColor(smallStarDef.paletteIndex);
			break;
		}
		case SkyStarType::Large:
		{
			const SkyLargeStarDefinition &largeStarDef = starDef.largeStar;
			tryLoadTextureAsset(largeStarDef.textureAsset);
			break;
		}
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(starDef.type)));
			break;
		}
	}

	for (int i = 0; i < skyInfoDef.getSunCount(); i++)
	{
		const SkySunDefinition &sunDef = skyInfoDef.getSun(i);
		tryLoadTextureAsset(sunDef.textureAsset);
	}

	for (int i = 0; i < skyInfoDef.getMoonCount(); i++)
	{
		const SkyMoonDefinition &moonDef = skyInfoDef.getMoon(i);
		for (const TextureAsset &textureAsset : moonDef.textureAssets)
		{
			tryLoadTextureAsset(textureAsset);
		}
	}

	// @todo: load draw calls for all the sky objects (ideally here, but can be in update() for now if convenient)
}

void RenderSkyManager::update(const CoordDouble3 &cameraCoord)
{
	// Keep the sky centered on the player.
	this->bgDrawCall.position = VoxelUtils::coordToWorldPoint(cameraCoord);

	// @todo: create draw calls for sky objects. later this can be in loadScene() as an optimization
	// @todo: update sky object draw call transforms if they are affected by planet rotation
	DebugLogWarning("Not implemented: RenderSkyManager::update() sky object draw calls");
}

void RenderSkyManager::unloadScene(Renderer &renderer)
{
	this->objectTextures.clear();
	this->objectDrawCalls.clear();
}
