#include <numeric>
#include <vector>

#include "ArenaRenderUtils.h"
#include "Renderer.h"
#include "RenderSkyManager.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ExeData.h"
#include "../Assets/TextureManager.h"
#include "../Math/Constants.h"
#include "../Math/Vector3.h"
#include "../Sky/SkyDefinition.h"
#include "../Sky/SkyInfoDefinition.h"
#include "../Sky/SkyInstance.h"
#include "../Weather/WeatherDefinition.h"
#include "../Weather/WeatherInstance.h"
#include "../World/MeshUtils.h"

void RenderSkyManager::LoadedGeneralSkyObjectTextureEntry::init(const TextureAsset &textureAsset,
	ScopedObjectTextureRef &&objectTextureRef)
{
	this->textureAsset = textureAsset;
	this->objectTextureRef = std::move(objectTextureRef);
}

void RenderSkyManager::LoadedSmallStarTextureEntry::init(uint8_t paletteIndex, ScopedObjectTextureRef &&objectTextureRef)
{
	this->paletteIndex = paletteIndex;
	this->objectTextureRef = std::move(objectTextureRef);
}

RenderSkyManager::RenderSkyManager()
{
	this->bgVertexBufferID = -1;
	this->bgNormalBufferID = -1;
	this->bgTexCoordBufferID = -1;
	this->bgIndexBufferID = -1;

	this->objectVertexBufferID = -1;
	this->objectNormalBufferID = -1;
	this->objectTexCoordBufferID = -1;
	this->objectIndexBufferID = -1;
}

void RenderSkyManager::init(const ExeData &exeData, TextureManager &textureManager, Renderer &renderer)
{
	std::vector<double> bgVertices;
	std::vector<double> bgNormals;
	std::vector<double> bgTexCoords;
	std::vector<int32_t> bgIndices;

	constexpr double pointDistance = 3000.0; // @todo: hack while the sky is using naive depth testing w/o any occlusion culling, etc.
	constexpr Radians angleAboveHorizon = 25.0 * Constants::DegToRad;
	const double aboveHorizonPointHeight = pointDistance * std::tan(angleAboveHorizon);

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
	const Double2 nadirTexCoord(0.50, 0.0);
	bgTexCoords.emplace_back(zenithTexCoord.x);
	bgTexCoords.emplace_back(zenithTexCoord.y);
	bgTexCoords.emplace_back(nadirTexCoord.x);
	bgTexCoords.emplace_back(nadirTexCoord.y);

	constexpr int textureTileCount = 150; // # of times the sky gradient texture tiles around the horizon.
	constexpr int bgHorizonEdgeCount = 150; // # of hemisphere edges on the horizon, determines # of triangles and smoothness of cone shape.
	for (int i = 0; i < bgHorizonEdgeCount; i++)
	{
		// Each horizon edge has a quad above it, and a triangle above that. Generate above and below horizon.
		const double percent = static_cast<double>(i) / static_cast<double>(bgHorizonEdgeCount);
		const double nextPercent = static_cast<double>(i + 1) / static_cast<double>(bgHorizonEdgeCount);
		const double period = percent * Constants::TwoPi;
		const double nextPeriod = nextPercent * Constants::TwoPi;

		const Double3 horizonPoint(std::cos(period) * pointDistance, 0.0, std::sin(period) * pointDistance);
		const Double3 nextHorizonPoint(std::cos(nextPeriod) * pointDistance, 0.0, std::sin(nextPeriod) * pointDistance);
		const Double3 aboveHorizonPoint(horizonPoint.x, aboveHorizonPointHeight, horizonPoint.z);
		const Double3 nextAboveHorizonPoint(nextHorizonPoint.x, aboveHorizonPointHeight, nextHorizonPoint.z);
		bgVertices.emplace_back(horizonPoint.x);
		bgVertices.emplace_back(horizonPoint.y);
		bgVertices.emplace_back(horizonPoint.z);
		bgVertices.emplace_back(nextHorizonPoint.x);
		bgVertices.emplace_back(nextHorizonPoint.y);
		bgVertices.emplace_back(nextHorizonPoint.z);
		bgVertices.emplace_back(aboveHorizonPoint.x);
		bgVertices.emplace_back(aboveHorizonPoint.y);
		bgVertices.emplace_back(aboveHorizonPoint.z);
		bgVertices.emplace_back(nextAboveHorizonPoint.x);
		bgVertices.emplace_back(nextAboveHorizonPoint.y);
		bgVertices.emplace_back(nextAboveHorizonPoint.z);

		// Normals point toward the player.
		const Double3 normal = -horizonPoint.normalized();
		const Double3 nextNormal = -nextHorizonPoint.normalized();
		const Double3 aboveNormal = -aboveHorizonPoint.normalized();
		const Double3 nextAboveNormal = -nextAboveHorizonPoint.normalized();
		bgNormals.emplace_back(normal.x);
		bgNormals.emplace_back(normal.y);
		bgNormals.emplace_back(normal.z);
		bgNormals.emplace_back(nextNormal.x);
		bgNormals.emplace_back(nextNormal.y);
		bgNormals.emplace_back(nextNormal.z);
		bgNormals.emplace_back(aboveNormal.x);
		bgNormals.emplace_back(aboveNormal.y);
		bgNormals.emplace_back(aboveNormal.z);
		bgNormals.emplace_back(nextAboveNormal.x);
		bgNormals.emplace_back(nextAboveNormal.y);
		bgNormals.emplace_back(nextAboveNormal.z);

		const double coordXStart = static_cast<double>(i % (bgHorizonEdgeCount / textureTileCount)) / static_cast<double>(bgHorizonEdgeCount / textureTileCount);
		const double coordXEnd = coordXStart + (1.0 / static_cast<double>(bgHorizonEdgeCount / textureTileCount));
		const double coordYStart = 0.0;
		const double coordYEnd = 1.0;
		const Double2 texCoord(coordXStart, coordYEnd);
		const Double2 nextTexCoord(coordXEnd, coordYEnd);
		const Double2 aboveTexCoord(coordXStart, coordYStart);
		const Double2 nextAboveTexCoord(coordXEnd, coordYStart);
		bgTexCoords.emplace_back(texCoord.x);
		bgTexCoords.emplace_back(texCoord.y);
		bgTexCoords.emplace_back(nextTexCoord.x);
		bgTexCoords.emplace_back(nextTexCoord.y);
		bgTexCoords.emplace_back(aboveTexCoord.x);
		bgTexCoords.emplace_back(aboveTexCoord.y);
		bgTexCoords.emplace_back(nextAboveTexCoord.x);
		bgTexCoords.emplace_back(nextAboveTexCoord.y);

		// Above-horizon
		const int32_t vertexIndex = static_cast<int32_t>((bgVertices.size() / 3) - 4);
		const int32_t nextVertexIndex = static_cast<int32_t>((bgVertices.size() / 3) - 3);
		const int32_t aboveVertexIndex = static_cast<int32_t>((bgVertices.size() / 3) - 2);
		const int32_t nextAboveVertexIndex = static_cast<int32_t>((bgVertices.size() / 3) - 1);
		bgIndices.emplace_back(aboveVertexIndex);
		bgIndices.emplace_back(vertexIndex);
		bgIndices.emplace_back(nextVertexIndex);
		
		bgIndices.emplace_back(nextVertexIndex);
		bgIndices.emplace_back(nextAboveVertexIndex);
		bgIndices.emplace_back(aboveVertexIndex);

		bgIndices.emplace_back(zenithVertexIndex);
		bgIndices.emplace_back(aboveVertexIndex);
		bgIndices.emplace_back(nextAboveVertexIndex);

		// Below-horizon
		bgIndices.emplace_back(nadirVertexIndex);
		bgIndices.emplace_back(nextVertexIndex);
		bgIndices.emplace_back(vertexIndex);
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

	auto allocBgTextureID = [this, &renderer](BufferView2D<const uint8_t> texels)
	{
		const int textureWidth = texels.getWidth();
		const int textureHeight = texels.getHeight();
		const int bytesPerTexel = 1;
		ObjectTextureID textureID;
		if (!renderer.tryCreateObjectTexture(textureWidth, textureHeight, bytesPerTexel, &textureID))
		{
			DebugLogError("Couldn't create object texture for sky background texture ID.");
			this->freeBgBuffers(renderer);
			return -1;
		}

		LockedTexture lockedTexture = renderer.lockObjectTexture(textureID);
		uint8_t *bgTexels = static_cast<uint8_t*>(lockedTexture.texels);
		std::copy(texels.begin(), texels.end(), bgTexels);
		renderer.unlockObjectTexture(textureID);

		return textureID;
	};

	auto allocBgTextureIdByFilename = [this, &textureManager, &allocBgTextureID](const std::string &filename)
	{
		const std::optional<TextureBuilderID> skyGradientTextureBuilderID = textureManager.tryGetTextureBuilderID(filename.c_str());
		if (!skyGradientTextureBuilderID.has_value())
		{
			DebugLogError("Couldn't get texture builder ID for background \"" + filename + "\".");
			return -1;
		}

		const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*skyGradientTextureBuilderID);
		DebugAssert(textureBuilder.getType() == TextureBuilder::Type::Paletted);
		const TextureBuilder::PalettedTexture &palettedTexture = textureBuilder.getPaletted();
		return allocBgTextureID(BufferView2D<const uint8_t>(palettedTexture.texels.begin(), textureBuilder.getWidth(), textureBuilder.getHeight()));
	};

	const ObjectTextureID skyGradientAMTextureID = allocBgTextureIdByFilename(ArenaTextureName::SkyDitherAM);
	const ObjectTextureID skyGradientPMTextureID = allocBgTextureIdByFilename(ArenaTextureName::SkyDitherPM);
	const ObjectTextureID skyFogTextureID = allocBgTextureID(BufferView2D<const uint8_t>(&ArenaRenderUtils::PALETTE_INDEX_SKY_COLOR_FOG, 1, 1));
	this->skyGradientAMTextureRef.init(skyGradientAMTextureID, renderer);
	this->skyGradientPMTextureRef.init(skyGradientPMTextureID, renderer);
	this->skyFogTextureRef.init(skyFogTextureID, renderer);

	const BufferView<const uint8_t> thunderstormColorsView(exeData.weather.thunderstormFlashColors);
	this->skyThunderstormTextureRefs.init(thunderstormColorsView.getCount());
	for (int i = 0; i < thunderstormColorsView.getCount(); i++)
	{
		const ObjectTextureID flashTextureID = allocBgTextureID(BufferView2D<const uint8_t>(&thunderstormColorsView[i], 1, 1));
		this->skyThunderstormTextureRefs.set(i, ScopedObjectTextureRef(flashTextureID, renderer));
	}

	const uint8_t skyInteriorColor = 0; // Black
	const ObjectTextureID skyInteriorTextureID = allocBgTextureID(BufferView2D<const uint8_t>(&skyInteriorColor, 1, 1));
	this->skyInteriorTextureRef.init(skyInteriorTextureID, renderer);

	this->bgDrawCall.position = Double3::Zero;
	this->bgDrawCall.preScaleTranslation = Double3::Zero;
	this->bgDrawCall.rotation = Matrix4d::identity();
	this->bgDrawCall.scale = Matrix4d::identity();
	this->bgDrawCall.vertexBufferID = this->bgVertexBufferID;
	this->bgDrawCall.normalBufferID = this->bgNormalBufferID;
	this->bgDrawCall.texCoordBufferID = this->bgTexCoordBufferID;
	this->bgDrawCall.indexBufferID = this->bgIndexBufferID;
	this->bgDrawCall.textureIDs[0] = this->skyGradientAMTextureRef.get(); // ID is updated depending on weather.
	this->bgDrawCall.textureIDs[1] = std::nullopt;
	this->bgDrawCall.textureSamplingType0 = TextureSamplingType::Default;
	this->bgDrawCall.textureSamplingType1 = TextureSamplingType::Default;
	this->bgDrawCall.lightingType = RenderLightingType::PerMesh;
	this->bgDrawCall.lightPercent = 1.0;
	this->bgDrawCall.lightIdCount = 0;
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

ObjectTextureID RenderSkyManager::getGeneralSkyObjectTextureID(const TextureAsset &textureAsset) const
{
	const auto iter = std::find_if(this->generalSkyObjectTextures.begin(), this->generalSkyObjectTextures.end(),
		[&textureAsset](const LoadedGeneralSkyObjectTextureEntry &loadedTexture)
	{
		return loadedTexture.textureAsset == textureAsset;
	});

	if (iter == this->generalSkyObjectTextures.end())
	{
		DebugLogError("Couldn't find loaded sky object texture for \"" + textureAsset.filename + "\".");
		return -1;
	}

	return iter->objectTextureRef.get();
}

ObjectTextureID RenderSkyManager::getSmallStarTextureID(uint8_t paletteIndex) const
{
	const auto iter = std::find_if(this->smallStarTextures.begin(), this->smallStarTextures.end(),
		[paletteIndex](const LoadedSmallStarTextureEntry &loadedTexture)
	{
		return loadedTexture.paletteIndex == paletteIndex;
	});

	if (iter == this->smallStarTextures.end())
	{
		DebugLogError("Couldn't find loaded small star texture with palette index \"" + std::to_string(paletteIndex) + "\".");
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

	this->generalSkyObjectTextures.clear();
	this->smallStarTextures.clear();
}

const RenderDrawCall &RenderSkyManager::getBgDrawCall() const
{
	return this->bgDrawCall;
}

BufferView<const RenderDrawCall> RenderSkyManager::getObjectDrawCalls() const
{
	return this->objectDrawCalls;
}

void RenderSkyManager::loadScene(const SkyInfoDefinition &skyInfoDef, TextureManager &textureManager, Renderer &renderer)
{
	auto tryLoadTextureAsset = [this, &textureManager, &renderer](const TextureAsset &textureAsset)
	{
		const auto iter = std::find_if(this->generalSkyObjectTextures.begin(), this->generalSkyObjectTextures.end(),
			[&textureAsset](const LoadedGeneralSkyObjectTextureEntry &loadedTexture)
		{
			return loadedTexture.textureAsset == textureAsset;
		});

		if (iter == this->generalSkyObjectTextures.end())
		{
			const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
			if (!textureBuilderID.has_value())
			{
				DebugLogError("Couldn't get texture builder ID for sky object texture \"" + textureAsset.filename + "\".");
				return;
			}

			const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
			ObjectTextureID textureID;
			if (!renderer.tryCreateObjectTexture(textureBuilder, &textureID))
			{
				DebugLogError("Couldn't create object texture for sky object texture \"" + textureAsset.filename + "\".");
				return;
			}

			LoadedGeneralSkyObjectTextureEntry loadedEntry;
			loadedEntry.init(textureAsset, ScopedObjectTextureRef(textureID, renderer));
			this->generalSkyObjectTextures.emplace_back(std::move(loadedEntry));
		}
	};

	auto tryLoadPaletteColor = [this, &renderer](uint8_t paletteIndex)
	{
		const auto iter = std::find_if(this->smallStarTextures.begin(), this->smallStarTextures.end(),
			[paletteIndex](const LoadedSmallStarTextureEntry &loadedTexture)
		{
			return loadedTexture.paletteIndex == paletteIndex;
		});

		if (iter == this->smallStarTextures.end())
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

			LoadedSmallStarTextureEntry loadedEntry;
			loadedEntry.init(paletteIndex, ScopedObjectTextureRef(textureID, renderer));
			this->smallStarTextures.emplace_back(std::move(loadedEntry));
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

	for (int i = 0; i < skyInfoDef.getLightningCount(); i++)
	{
		const SkyLightningDefinition &lightningDef = skyInfoDef.getLightning(i);
		for (const TextureAsset &textureAsset : lightningDef.textureAssets)
		{
			tryLoadTextureAsset(textureAsset);
		}
	}

	// @todo: load draw calls for all the sky objects (ideally here, but can be in update() for now if convenient)
}

void RenderSkyManager::update(const SkyInstance &skyInst, const WeatherInstance &weatherInst,
	const CoordDouble3 &cameraCoord, bool isInterior, double daytimePercent, bool isFoggy, double distantAmbientPercent,
	Renderer &renderer)
{
	const WorldDouble3 cameraPos = VoxelUtils::coordToWorldPoint(cameraCoord);

	// Keep background centered on the player.
	this->bgDrawCall.position = cameraPos;

	// Update background texture ID based on active weather.
	std::optional<double> thunderstormFlashPercent;
	if (weatherInst.hasRain())
	{
		const WeatherRainInstance &rainInst = weatherInst.getRain();
		if (rainInst.thunderstorm.has_value())
		{
			const WeatherRainInstance::Thunderstorm &thunderstormInst = *rainInst.thunderstorm;
			thunderstormFlashPercent = thunderstormInst.getFlashPercent();
		}
	}

	const bool isAM = daytimePercent < 0.50;
	if (thunderstormFlashPercent.has_value())
	{
		const int flashTextureCount = this->skyThunderstormTextureRefs.getCount();
		const int flashIndex = std::clamp(static_cast<int>(static_cast<double>(flashTextureCount) * (*thunderstormFlashPercent)), 0, flashTextureCount - 1);
		this->bgDrawCall.textureIDs[0] = this->skyThunderstormTextureRefs[flashIndex].get();
	}
	else if (isFoggy)
	{
		this->bgDrawCall.textureIDs[0] = this->skyFogTextureRef.get();
	}
	else if (isInterior)
	{
		this->bgDrawCall.textureIDs[0] = this->skyInteriorTextureRef.get();
	}
	else if (isAM)
	{
		this->bgDrawCall.textureIDs[0] = this->skyGradientAMTextureRef.get();
	}
	else
	{
		this->bgDrawCall.textureIDs[0] = this->skyGradientPMTextureRef.get();
	}

	// @temp fix for Z ordering. Later I think we should just not do depth testing in the sky?
	constexpr double landDistance = 500.0;
	constexpr double airDistance = landDistance + 400.0;
	constexpr double moonDistance = airDistance + 400.0;
	constexpr double sunDistance = moonDistance + 400.0;
	constexpr double starDistance = sunDistance + 400.0;

	constexpr double fullBrightLightPercent = 1.0;

	auto addDrawCall = [this, &renderer, &cameraPos](const Double3 &direction, double width, double height, ObjectTextureID textureID,
		double arbitraryDistance, double meshLightPercent, PixelShaderType pixelShaderType)
	{
		RenderDrawCall drawCall;
		drawCall.position = cameraPos + (direction * arbitraryDistance);
		drawCall.preScaleTranslation = Double3::Zero;
		
		const Radians pitchRadians = direction.getYAngleRadians();
		const Radians yawRadians = MathUtils::fullAtan2(Double2(direction.z, direction.x).normalized()) + Constants::Pi;
		const Matrix4d pitchRotation = Matrix4d::zRotation(pitchRadians);
		const Matrix4d yawRotation = Matrix4d::yRotation(yawRadians);
		drawCall.rotation = yawRotation * pitchRotation;

		const double scaledWidth = width * arbitraryDistance;
		const double scaledHeight = height * arbitraryDistance;
		drawCall.scale = Matrix4d::scale(1.0, scaledHeight, scaledWidth);

		drawCall.vertexBufferID = this->objectVertexBufferID;
		drawCall.normalBufferID = this->objectNormalBufferID;
		drawCall.texCoordBufferID = this->objectTexCoordBufferID;
		drawCall.indexBufferID = this->objectIndexBufferID;
		drawCall.textureIDs[0] = textureID;
		drawCall.textureIDs[1] = std::nullopt;
		drawCall.textureSamplingType0 = TextureSamplingType::Default;
		drawCall.textureSamplingType1 = TextureSamplingType::Default;
		drawCall.lightingType = RenderLightingType::PerMesh;
		drawCall.lightPercent = meshLightPercent;
		drawCall.lightIdCount = 0;
		drawCall.vertexShaderType = VertexShaderType::SlidingDoor; // @todo: make a sky object vertex shader
		drawCall.pixelShaderType = pixelShaderType;
		drawCall.pixelShaderParam0 = 0.0;
		this->objectDrawCalls.emplace_back(std::move(drawCall));
	};

	// @todo: create draw calls in loadScene() as an optimization
	// @todo: update sky object draw call transforms if they are affected by planet rotation

	this->objectDrawCalls.clear(); // @todo: don't clear every frame, just change their transforms/animation texture ID

	// No sky objects during fog.
	if (isFoggy)
	{
		return;
	}

	// Order draw calls back to front.
	for (int i = skyInst.starStart; i < skyInst.starEnd; i++)
	{
		const SkyObjectInstance &skyObjectInst = skyInst.getSkyObjectInst(i);
		const SkyObjectTextureType textureType = skyObjectInst.textureType;

		ObjectTextureID textureID = -1;
		if (textureType == SkyObjectTextureType::TextureAsset)
		{
			const SkyObjectTextureAssetEntry &textureAssetEntry = skyInst.getTextureAssetEntry(skyObjectInst.textureAssetEntryID);
			const TextureAsset &textureAsset = textureAssetEntry.textureAssets.get(0);
			textureID = this->getGeneralSkyObjectTextureID(textureAsset);
		}
		else if (textureType == SkyObjectTextureType::PaletteIndex)
		{
			const SkyObjectPaletteIndexEntry &paletteIndexEntry = skyInst.getPaletteIndexEntry(skyObjectInst.paletteIndexEntryID);
			const uint8_t paletteIndex = paletteIndexEntry.paletteIndex;
			textureID = this->getSmallStarTextureID(paletteIndex);
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(textureType)));
		}

		addDrawCall(skyObjectInst.transformedDirection, skyObjectInst.width, skyObjectInst.height, textureID, starDistance,
			fullBrightLightPercent, PixelShaderType::AlphaTestedWithPreviousBrightnessLimit);
	}

	for (int i = skyInst.sunStart; i < skyInst.sunEnd; i++)
	{
		const SkyObjectInstance &skyObjectInst = skyInst.getSkyObjectInst(i);
		const SkyObjectTextureType textureType = skyObjectInst.textureType;
		DebugAssertMsg(textureType == SkyObjectTextureType::TextureAsset, "Expected all sky sun objects to use TextureAsset texture type.");

		const SkyObjectTextureAssetEntry &textureAssetEntry = skyInst.getTextureAssetEntry(skyObjectInst.textureAssetEntryID);
		const TextureAsset &textureAsset = textureAssetEntry.textureAssets.get(0);
		const ObjectTextureID textureID = this->getGeneralSkyObjectTextureID(textureAsset);

		addDrawCall(skyObjectInst.transformedDirection, skyObjectInst.width, skyObjectInst.height, textureID, sunDistance,
			fullBrightLightPercent, PixelShaderType::AlphaTested);
	}

	for (int i = skyInst.moonStart; i < skyInst.moonEnd; i++)
	{
		const SkyObjectInstance &skyObjectInst = skyInst.getSkyObjectInst(i);
		const SkyObjectTextureType textureType = skyObjectInst.textureType;
		DebugAssertMsg(textureType == SkyObjectTextureType::TextureAsset, "Expected all sky moon objects to use TextureAsset texture type.");

		const SkyObjectTextureAssetEntry &textureAssetEntry = skyInst.getTextureAssetEntry(skyObjectInst.textureAssetEntryID);
		const TextureAsset &textureAsset = textureAssetEntry.textureAssets.get(0);
		const ObjectTextureID textureID = this->getGeneralSkyObjectTextureID(textureAsset);

		addDrawCall(skyObjectInst.transformedDirection, skyObjectInst.width, skyObjectInst.height, textureID, moonDistance,
			fullBrightLightPercent, PixelShaderType::AlphaTestedWithLightLevelColor);
	}

	for (int i = skyInst.airStart; i < skyInst.airEnd; i++)
	{
		const SkyObjectInstance &skyObjectInst = skyInst.getSkyObjectInst(i);
		const SkyObjectTextureType textureType = skyObjectInst.textureType;
		DebugAssertMsg(textureType == SkyObjectTextureType::TextureAsset, "Expected all sky air objects to use TextureAsset texture type.");

		const SkyObjectTextureAssetEntry &textureAssetEntry = skyInst.getTextureAssetEntry(skyObjectInst.textureAssetEntryID);
		const TextureAsset &textureAsset = textureAssetEntry.textureAssets.get(0);
		const ObjectTextureID textureID = this->getGeneralSkyObjectTextureID(textureAsset);

		addDrawCall(skyObjectInst.transformedDirection, skyObjectInst.width, skyObjectInst.height, textureID, airDistance,
			distantAmbientPercent, PixelShaderType::AlphaTestedWithLightLevelColor);
	}

	for (int i = skyInst.landStart; i < skyInst.landEnd; i++)
	{
		const SkyObjectInstance &skyObjectInst = skyInst.getSkyObjectInst(i);
		const SkyObjectTextureType textureType = skyObjectInst.textureType;
		DebugAssertMsg(textureType == SkyObjectTextureType::TextureAsset, "Expected all sky land objects to use TextureAsset texture type.");

		const SkyObjectTextureAssetEntry &textureAssetEntry = skyInst.getTextureAssetEntry(skyObjectInst.textureAssetEntryID);
		const BufferView<const TextureAsset> textureAssets = textureAssetEntry.textureAssets;
		const int textureCount = textureAssets.getCount();

		int textureAssetIndex = 0;
		const int animIndex = skyObjectInst.animIndex;
		const bool hasAnimation = animIndex >= 0;
		if (hasAnimation)
		{
			const SkyObjectAnimationInstance &animInst = skyInst.getAnimInst(animIndex);
			const double animPercent = animInst.percentDone;
			textureAssetIndex = std::clamp(static_cast<int>(static_cast<double>(textureCount) * animPercent), 0, textureCount - 1);
		}

		const TextureAsset &textureAsset = textureAssets.get(textureAssetIndex);
		const ObjectTextureID textureID = this->getGeneralSkyObjectTextureID(textureAsset);
		const double meshLightPercent = skyObjectInst.emissive ? fullBrightLightPercent : distantAmbientPercent;
		addDrawCall(skyObjectInst.transformedDirection, skyObjectInst.width, skyObjectInst.height, textureID, landDistance,
			meshLightPercent, PixelShaderType::AlphaTested);
	}

	for (int i = skyInst.lightningStart; i < skyInst.lightningEnd; i++)
	{
		if (!skyInst.isLightningVisible(i))
		{
			continue;
		}

		const SkyObjectInstance &skyObjectInst = skyInst.getSkyObjectInst(i);
		const SkyObjectTextureType textureType = skyObjectInst.textureType;
		DebugAssertMsg(textureType == SkyObjectTextureType::TextureAsset, "Expected all sky lightning objects to use TextureAsset texture type.");

		const SkyObjectTextureAssetEntry &textureAssetEntry = skyInst.getTextureAssetEntry(skyObjectInst.textureAssetEntryID);
		const BufferView<const TextureAsset> textureAssets = textureAssetEntry.textureAssets;
		const int textureCount = textureAssets.getCount();

		int textureAssetIndex = 0;
		const int animIndex = skyObjectInst.animIndex;
		double meshLightPercent = fullBrightLightPercent;
		const bool hasAnimation = animIndex >= 0;
		DebugAssert(hasAnimation);

		const SkyObjectAnimationInstance &animInst = skyInst.getAnimInst(animIndex);
		const double animPercent = animInst.percentDone;
		textureAssetIndex = std::clamp(static_cast<int>(static_cast<double>(textureCount) * animPercent), 0, textureCount - 1);

		const TextureAsset &textureAsset = textureAssets.get(textureAssetIndex);
		const ObjectTextureID textureID = this->getGeneralSkyObjectTextureID(textureAsset);
		addDrawCall(skyObjectInst.transformedDirection, skyObjectInst.width, skyObjectInst.height, textureID, landDistance,
			meshLightPercent, PixelShaderType::AlphaTested);
	}
}

void RenderSkyManager::unloadScene(Renderer &renderer)
{
	this->generalSkyObjectTextures.clear();
	this->smallStarTextures.clear();
	this->objectDrawCalls.clear();
}
