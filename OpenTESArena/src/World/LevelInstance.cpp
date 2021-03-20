#include <algorithm>

#include "LevelInstance.h"
#include "MapDefinition.h"
#include "MapType.h"
#include "WeatherUtils.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Entities/CitizenUtils.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/RendererUtils.h"

#include "components/debug/Debug.h"

LevelInstance::LevelInstance()
{
	this->ceilingScale = 0.0;
}

void LevelInstance::init(double ceilingScale)
{
	this->ceilingScale = ceilingScale;
}

ChunkManager &LevelInstance::getChunkManager()
{
	return this->chunkManager;
}

const ChunkManager &LevelInstance::getChunkManager() const
{
	return this->chunkManager;
}

EntityManager &LevelInstance::getEntityManager()
{
	return this->entityManager;
}

const EntityManager &LevelInstance::getEntityManager() const
{
	return this->entityManager;
}

double LevelInstance::getCeilingScale() const
{
	return this->ceilingScale;
}

bool LevelInstance::trySetActive(WeatherType weatherType, bool nightLightsAreActive,
	const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	TextureManager &textureManager, Renderer &renderer)
{
	renderer.clearTextures();
	renderer.clearSky(); // @todo: not sure this needs to stay. The sky could be treated the same way as visible entities (updated every frame).

	const std::string &paletteName = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette \"" + paletteName + "\".");
		return false;
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);

	// Set sky colors and objects.
	const MapType mapType = mapDefinition.getMapType();
	const SkyDefinition &skyDefinition = [&activeLevelIndex, &mapDefinition, mapType]() -> const SkyDefinition&
	{
		const int skyIndex = [&activeLevelIndex, &mapDefinition, mapType]()
		{
			if ((mapType == MapType::Interior) || (mapType == MapType::City))
			{
				DebugAssert(activeLevelIndex.has_value());
				return mapDefinition.getSkyIndexForLevel(*activeLevelIndex);
			}
			else if (mapType == MapType::Wilderness)
			{
				return mapDefinition.getSkyIndexForLevel(0);
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(mapType)));
			}
		}();
		
		return mapDefinition.getSky(skyIndex);
	}();

	Buffer<uint32_t> skyColors(skyDefinition.getSkyColorCount());
	for (int i = 0; i < skyColors.getCount(); i++)
	{
		const Color &color = skyDefinition.getSkyColor(i);
		skyColors.set(i, color.toARGB());
	}

	renderer.setSkyPalette(skyColors.get(), skyColors.getCount());
	// @todo: renderer.setSky();

	// Loaded texture caches for tracking which textures have already been loaded in the renderer.
	// @todo: eventually don't preload all textures and let the chunk manager load them for new chunks.
	RendererUtils::LoadedVoxelTextureCache loadedVoxelTextures;
	RendererUtils::LoadedEntityTextureCache loadedEntityTextures;

	// Load textures known at level load time.
	auto loadLevelDefTextures = [&mapDefinition, &textureManager, &renderer, &loadedVoxelTextures,
		&loadedEntityTextures](int levelIndex)
	{
		const LevelInfoDefinition &levelInfoDef = mapDefinition.getLevelInfoForLevel(levelIndex);

		for (int i = 0; i < levelInfoDef.getVoxelDefCount(); i++)
		{
			const VoxelDefinition &voxelDef = levelInfoDef.getVoxelDef(i);
			for (int j = 0; j < voxelDef.getTextureAssetReferenceCount(); j++)
			{
				const TextureAssetReference &textureAssetRef = voxelDef.getTextureAssetReference(j);
				const auto cacheIter = std::find(loadedVoxelTextures.begin(), loadedVoxelTextures.end(), textureAssetRef);
				if (cacheIter == loadedVoxelTextures.end())
				{
					loadedVoxelTextures.push_back(textureAssetRef);
					if (!renderer.tryCreateVoxelTexture(textureAssetRef, textureManager))
					{
						DebugLogError("Couldn't create renderer voxel texture for \"" + textureAssetRef.filename + "\".");
					}
				}
			}
		}

		for (int defIndex = 0; defIndex < levelInfoDef.getEntityDefCount(); defIndex++)
		{
			const EntityDefinition &entityDef = levelInfoDef.getEntityDef(defIndex);
			const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
			const bool reflective = (entityDef.getType() == EntityDefinition::Type::Doodad) &&
				entityDef.getDoodad().puddle;

			for (int i = 0; i < animDef.getStateCount(); i++)
			{
				const EntityAnimationDefinition::State &state = animDef.getState(i);
				for (int j = 0; j < state.getKeyframeListCount(); j++)
				{
					const EntityAnimationDefinition::KeyframeList &keyframeList = state.getKeyframeList(j);
					const bool flipped = keyframeList.isFlipped();
					for (int k = 0; k < keyframeList.getKeyframeCount(); k++)
					{
						const EntityAnimationDefinition::Keyframe &keyframe = keyframeList.getKeyframe(k);
						const TextureAssetReference &textureAssetRef = keyframe.getTextureAssetRef();
						const auto cacheIter = std::find_if(loadedEntityTextures.begin(), loadedEntityTextures.end(),
							[&textureAssetRef, flipped, reflective](const RendererUtils::LoadedEntityTextureEntry &cacheEntry)
						{
							return (cacheEntry.textureAssetRef == textureAssetRef) && (cacheEntry.flipped == flipped) &&
								(cacheEntry.reflective == reflective);
						});

						if (cacheIter == loadedEntityTextures.end())
						{
							RendererUtils::LoadedEntityTextureEntry loadedEntityTextureEntry;
							loadedEntityTextureEntry.init(TextureAssetReference(textureAssetRef), flipped, reflective);
							loadedEntityTextures.emplace_back(std::move(loadedEntityTextureEntry));
							
							if (!renderer.tryCreateEntityTexture(textureAssetRef, flipped, reflective, textureManager))
							{
								DebugLogError("Couldn't create renderer entity texture for \"" + textureAssetRef.filename + "\".");
							}
						}
					}
				}
			}
		}
	};

	if ((mapType == MapType::Interior) || (mapType == MapType::City))
	{
		// Load textures for the active level.
		DebugAssert(activeLevelIndex.has_value());
		loadLevelDefTextures(*activeLevelIndex);
	}
	else if (mapType == MapType::Wilderness)
	{
		// Load textures for all wilderness chunks.
		for (int i = 0; i < mapDefinition.getLevelCount(); i++)
		{
			loadLevelDefTextures(i);
		}
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(mapType)));
	}

	// Load citizen textures if citizens can exist in the level.
	if ((mapType == MapType::City) || (mapType == MapType::Wilderness))
	{
		DebugAssert(citizenGenInfo.has_value());
		CitizenUtils::writeCitizenTextures(*citizenGenInfo->maleEntityDef, *citizenGenInfo->femaleEntityDef,
			textureManager, renderer);
	}

	// Load chasm textures (dry chasms are just a single color).
	const uint8_t dryChasmColor = ArenaRenderUtils::PALETTE_INDEX_DRY_CHASM_COLOR;
	renderer.addChasmTexture(ArenaTypes::ChasmType::Dry, &dryChasmColor, 1, 1, palette);

	auto writeChasmTextures = [&textureManager, &renderer, &palette](ArenaTypes::ChasmType chasmType)
	{
		const std::string chasmFilename = [chasmType]()
		{
			if (chasmType == ArenaTypes::ChasmType::Wet)
			{
				return ArenaRenderUtils::CHASM_WATER_FILENAME;
			}
			else if (chasmType == ArenaTypes::ChasmType::Lava)
			{
				return ArenaRenderUtils::CHASM_LAVA_FILENAME;
			}
			else
			{
				DebugUnhandledReturnMsg(std::string, std::to_string(static_cast<int>(chasmType)));
			}
		}();

		const std::optional<TextureBuilderIdGroup> textureBuilderIDs =
			textureManager.tryGetTextureBuilderIDs(chasmFilename.c_str());
		if (!textureBuilderIDs.has_value())
		{
			DebugLogError("Couldn't get chasm texture builder IDs for \"" + chasmFilename + "\".");
			return;
		}

		for (int i = 0; i < textureBuilderIDs->getCount(); i++)
		{
			const TextureBuilderID textureBuilderID = textureBuilderIDs->getID(i);
			const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(textureBuilderID);

			DebugAssert(textureBuilder.getType() == TextureBuilder::Type::Paletted);
			const TextureBuilder::PalettedTexture &palettedTexture = textureBuilder.getPaletted();
			renderer.addChasmTexture(chasmType, palettedTexture.texels.get(),
				textureBuilder.getWidth(), textureBuilder.getHeight(), palette);
		}
	};

	writeChasmTextures(ArenaTypes::ChasmType::Wet);
	writeChasmTextures(ArenaTypes::ChasmType::Lava);

	// Set renderer fog distance and night lights.
	const double fogDistance = WeatherUtils::getFogDistanceFromWeather(weatherType);
	renderer.setFogDistance(fogDistance);
	renderer.setNightLightsActive(nightLightsAreActive, palette);

	return true;
}

void LevelInstance::update(double dt, Game &game, const CoordDouble3 &playerCoord,
	const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, int chunkDistance, 
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager)
{
	const ChunkInt2 &centerChunk = playerCoord.chunk;
	this->chunkManager.update(dt, centerChunk, playerCoord, activeLevelIndex, mapDefinition, citizenGenInfo,
		chunkDistance, entityDefLibrary, binaryAssetLibrary, textureManager, this->entityManager);

	this->entityManager.tick(game, dt);
}
