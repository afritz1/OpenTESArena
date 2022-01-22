#include <algorithm>

#include "ArenaWeatherUtils.h"
#include "LevelInstance.h"
#include "MapDefinition.h"
#include "MapType.h"
#include "WeatherDefinition.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Entities/CitizenUtils.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/RendererUtils.h"

#include "components/debug/Debug.h"

void LevelInstance::LoadedVoxelMaterial::init(const TextureAssetReference &textureAssetRef,
	ScopedObjectTextureRef &&objectTextureRef, ScopedObjectMaterialRef &&objectMaterialRef)
{
	this->textureAssetRef = textureAssetRef;
	this->objectTextureRef = std::move(objectTextureRef);
	this->objectMaterialRef = std::move(objectMaterialRef);
}

void LevelInstance::LoadedEntityMaterial::init(const TextureAssetReference &textureAssetRef, bool flipped, bool reflective,
	ScopedObjectTextureRef &&objectTextureRef, ScopedObjectMaterialRef &&objectMaterialRef)
{
	this->textureAssetRef = textureAssetRef;
	this->flipped = flipped;
	this->reflective = reflective;
	this->objectTextureRef = std::move(objectTextureRef);
	this->objectMaterialRef = std::move(objectMaterialRef);
}

void LevelInstance::LoadedChasmMaterialList::Entry::init(ScopedObjectTextureRef &&objectTextureRef0,
	ScopedObjectTextureRef &&objectTextureRef1, ScopedObjectMaterialRef &&objectMaterialRef)
{
	this->objectTextureRef0 = std::move(objectTextureRef0);
	this->objectTextureRef1 = std::move(objectTextureRef1);
	this->objectMaterialRef = std::move(objectMaterialRef);
}

void LevelInstance::LoadedChasmMaterialList::init(ArenaTypes::ChasmType chasmType)
{
	this->chasmType = chasmType;
}

void LevelInstance::LoadedChasmMaterialList::add(Entry &&entry)
{
	this->entries.emplace_back(std::move(entry));
}

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

ObjectTextureID LevelInstance::getPaletteTextureID() const
{
	return this->paletteTextureRef.get();
}

ObjectTextureID LevelInstance::getLightTableTextureID() const
{
	return this->lightTableTextureRef.get();
}

double LevelInstance::getCeilingScale() const
{
	return this->ceilingScale;
}

ObjectMaterialID LevelInstance::getVoxelMaterialID(const TextureAssetReference &textureAssetRef) const
{
	const auto iter = std::find_if(this->voxelMaterials.begin(), this->voxelMaterials.end(),
		[&textureAssetRef](const LoadedVoxelMaterial &loadedMaterial)
	{
		return loadedMaterial.textureAssetRef == textureAssetRef;
	});

	DebugAssertMsg(iter != this->voxelMaterials.end(), "No loaded voxel material for \"" + textureAssetRef.filename + "\".");
	const ScopedObjectMaterialRef &objectMaterialRef = iter->objectMaterialRef;
	return objectMaterialRef.get();
}

ObjectMaterialID LevelInstance::getEntityMaterialID(const TextureAssetReference &textureAssetRef, bool flipped, bool reflective) const
{
	const auto iter = std::find_if(this->entityMaterials.begin(), this->entityMaterials.end(),
		[&textureAssetRef, flipped, reflective](const LoadedEntityMaterial &loadedMaterial)
	{
		return (loadedMaterial.textureAssetRef == textureAssetRef) && (loadedMaterial.flipped == flipped) &&
			(loadedMaterial.reflective == reflective);
	});

	DebugAssertMsg(iter != this->entityMaterials.end(), "No loaded entity material for \"" + textureAssetRef.filename + "\".");
	const ScopedObjectMaterialRef &objectMaterialRef = iter->objectMaterialRef;
	return objectMaterialRef.get();
}

ObjectMaterialID LevelInstance::getChasmMaterialID(ArenaTypes::ChasmType chasmType, double chasmAnimPercent) const
{
	const auto iter = std::find_if(this->chasmMaterialLists.begin(), this->chasmMaterialLists.end(),
		[chasmType](const LoadedChasmMaterialList &loadedMaterialList)
	{
		return loadedMaterialList.chasmType == chasmType;
	});

	DebugAssertMsg(iter != this->chasmMaterialLists.end(), "No loaded chasm material list for type \"" + std::to_string(static_cast<int>(chasmType)) + "\".");
	const std::vector<LoadedChasmMaterialList::Entry> &entries = iter->entries;
	const int textureCount = static_cast<int>(entries.size());
	const int index = std::clamp(static_cast<int>(static_cast<double>(textureCount) * chasmAnimPercent), 0, textureCount - 1);
	DebugAssertIndex(entries, index);
	const LoadedChasmMaterialList::Entry &entry = entries[index];
	return entry.objectMaterialRef.get();
}

bool LevelInstance::trySetActive(const WeatherDefinition &weatherDef, bool nightLightsAreActive,
	const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	TextureManager &textureManager, Renderer &renderer)
{
	// Clear stored object texture refs, freeing them from the renderer.
	this->voxelMaterials.clear();
	this->entityMaterials.clear();
	this->chasmMaterialLists.clear();
	this->paletteTextureRef.destroy();
	this->lightTableTextureRef.destroy();

	auto loadVoxelDefTextures = [this, &textureManager, &renderer](const VoxelDefinition &voxelDef)
	{
		for (int i = 0; i < voxelDef.getTextureAssetReferenceCount(); i++)
		{
			const TextureAssetReference &textureAssetRef = voxelDef.getTextureAssetReference(i);
			const auto cacheIter = std::find_if(this->voxelMaterials.begin(), this->voxelMaterials.end(),
				[&textureAssetRef](const LoadedVoxelMaterial &loadedMaterial)
			{
				return loadedMaterial.textureAssetRef == textureAssetRef;
			});

			if (cacheIter == this->voxelMaterials.end())
			{
				const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
				if (!textureBuilderID.has_value())
				{
					DebugLogWarning("Couldn't load voxel texture \"" + textureAssetRef.filename + "\".");
					continue;
				}

				const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
				ObjectTextureID voxelTextureID;
				if (!renderer.tryCreateObjectTexture(textureBuilder, &voxelTextureID))
				{
					DebugLogWarning("Couldn't create voxel texture \"" + textureAssetRef.filename + "\".");
					continue;
				}

				ScopedObjectTextureRef voxelTextureRef(voxelTextureID, renderer);
				ObjectMaterialID voxelMaterialID;
				if (!renderer.tryCreateObjectMaterial(voxelTextureID, &voxelMaterialID))
				{
					DebugLogWarning("Couldn't create voxel material \"" + textureAssetRef.filename + "\".");
					continue;
				}

				ScopedObjectMaterialRef voxelMaterialRef(voxelMaterialID, renderer);
				LoadedVoxelMaterial newMaterial;
				newMaterial.init(textureAssetRef, std::move(voxelTextureRef), std::move(voxelMaterialRef));
				this->voxelMaterials.emplace_back(std::move(newMaterial));
			}
		}
	};

	auto loadEntityDefTextures = [this, &textureManager, &renderer](const EntityDefinition &entityDef)
	{
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
		const bool reflective = (entityDef.getType() == EntityDefinition::Type::Doodad) && entityDef.getDoodad().puddle;

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
					const auto cacheIter = std::find_if(this->entityMaterials.begin(), this->entityMaterials.end(),
						[&textureAssetRef, flipped, reflective](const LoadedEntityMaterial &loadedMaterial)
					{
						return (loadedMaterial.textureAssetRef == textureAssetRef) && (loadedMaterial.flipped == flipped) &&
							(loadedMaterial.reflective == reflective);
					});

					if (cacheIter == entityMaterials.end())
					{
						const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
						if (!textureBuilderID.has_value())
						{
							DebugLogWarning("Couldn't load entity texture \"" + textureAssetRef.filename + "\".");
							continue;
						}

						const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
						const int textureWidth = textureBuilder.getWidth();
						const int textureHeight = textureBuilder.getHeight();

						ObjectTextureID entityTextureID;
						if (!renderer.tryCreateObjectTexture(textureWidth, textureHeight, false, &entityTextureID))
						{
							DebugLogWarning("Couldn't create entity texture \"" + textureAssetRef.filename + "\".");
							continue;
						}

						ScopedObjectTextureRef entityTextureRef(entityTextureID, renderer);
						DebugAssert(textureBuilder.getType() == TextureBuilder::Type::Paletted);
						const TextureBuilder::PalettedTexture &srcTexture = textureBuilder.getPaletted();
						const uint8_t *srcTexels = srcTexture.texels.get();

						LockedTexture lockedEntityTexture = renderer.lockObjectTexture(entityTextureID);
						if (!lockedEntityTexture.isValid())
						{
							DebugLogWarning("Couldn't lock entity texture \"" + textureAssetRef.filename + "\".");
							continue;
						}

						DebugAssert(!lockedEntityTexture.isTrueColor);
						uint8_t *dstTexels = static_cast<uint8_t*>(lockedEntityTexture.texels);

						for (int y = 0; y < textureHeight; y++)
						{
							for (int x = 0; x < textureWidth; x++)
							{
								// Mirror texture if this texture is for an angle that gets mirrored.
								const int srcIndex = x + (y * textureWidth);
								const int dstIndex = (!flipped ? x : (textureWidth - 1 - x)) + (y * textureWidth);
								dstTexels[dstIndex] = srcTexels[srcIndex];
							}
						}

						renderer.unlockObjectTexture(entityTextureID);

						ObjectMaterialID entityMaterialID;
						if (!renderer.tryCreateObjectMaterial(entityTextureID, &entityMaterialID))
						{
							DebugLogWarning("Couldn't create entity material \"" + textureAssetRef.filename + "\".");
							continue;
						}

						ScopedObjectMaterialRef entityMaterialRef(entityMaterialID, renderer);
						LoadedEntityMaterial newMaterial;
						newMaterial.init(textureAssetRef, flipped, reflective, std::move(entityTextureRef), std::move(entityMaterialRef));
						this->entityMaterials.emplace_back(std::move(newMaterial));
					}
				}
			}
		}
	};

	// Load textures known at level load time. Note that none of the object texture IDs allocated here are
	// matched with voxel/entity instances until the chunks containing them are created.
	auto loadLevelDefTextures = [this, &mapDefinition, &textureManager, &renderer, &loadVoxelDefTextures,
		&loadEntityDefTextures](int levelIndex)
	{
		const LevelInfoDefinition &levelInfoDef = mapDefinition.getLevelInfoForLevel(levelIndex);

		for (int i = 0; i < levelInfoDef.getVoxelDefCount(); i++)
		{
			const VoxelDefinition &voxelDef = levelInfoDef.getVoxelDef(i);
			loadVoxelDefTextures(voxelDef);
		}

		for (int i = 0; i < levelInfoDef.getEntityDefCount(); i++)
		{
			const EntityDefinition &entityDef = levelInfoDef.getEntityDef(i);
			loadEntityDefTextures(entityDef);
		}
	};

	const MapType mapType = mapDefinition.getMapType();
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
		const EntityDefinition &maleEntityDef = *citizenGenInfo->maleEntityDef;
		const EntityDefinition &femaleEntityDef = *citizenGenInfo->femaleEntityDef;
		loadEntityDefTextures(maleEntityDef);
		loadEntityDefTextures(femaleEntityDef);
	}

	auto loadChasmTextures = [this, &textureManager, &renderer](ArenaTypes::ChasmType chasmType)
	{
		const auto cacheIter = std::find_if(this->chasmMaterialLists.begin(), this->chasmMaterialLists.end(),
			[chasmType](const LoadedChasmMaterialList &loadedMaterialList)
		{
			return loadedMaterialList.chasmType == chasmType;
		});

		DebugAssertMsg(cacheIter == this->chasmMaterialLists.end(), "Already loaded chasm material for type \"" +
			std::to_string(static_cast<int>(chasmType)) + "\".");

		const bool hasTexturedAnim = chasmType != ArenaTypes::ChasmType::Dry;
		if (hasTexturedAnim)
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

			LoadedChasmMaterialList newMaterialList;
			newMaterialList.init(chasmType);

			const Buffer<TextureAssetReference> textureAssetRefs = TextureUtils::makeTextureAssetRefs(chasmFilename, textureManager);
			for (int i = 0; i < textureAssetRefs.getCount(); i++)
			{
				const TextureAssetReference &textureAssetRef = textureAssetRefs.get(i);
				const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
				if (!textureBuilderID.has_value())
				{
					DebugLogWarning("Couldn't load chasm texture \"" + textureAssetRef.filename + "\".");
					continue;
				}

				const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
				ObjectTextureID chasmTextureID;
				if (!renderer.tryCreateObjectTexture(textureBuilder, &chasmTextureID))
				{
					DebugLogWarning("Couldn't create chasm texture \"" + textureAssetRef.filename + "\".");
					continue;
				}

				ScopedObjectTextureRef chasmTextureRef(chasmTextureID, renderer);				
				ObjectMaterialID chasmMaterialID;
				if (!renderer.tryCreateObjectMaterial(chasmTextureID, chasmTextureID, &chasmMaterialID)) // @todo: give side texture ID as well
				{
					DebugLogWarning("Couldn't create chasm material \"" + textureAssetRef.filename + "\".");
					return;
				}

				ScopedObjectMaterialRef chasmMaterialRef(chasmMaterialID, renderer);
				LoadedChasmMaterialList::Entry newMaterialListEntry;
				newMaterialListEntry.init(std::move(chasmTextureRef), ScopedObjectTextureRef(), std::move(chasmMaterialRef)); // @todo: allocate side texture ref
				newMaterialList.add(std::move(newMaterialListEntry));
			}

			this->chasmMaterialLists.emplace_back(std::move(newMaterialList));
		}
		else
		{
			// Dry chasms are just a single color, no texture asset available.
			ObjectTextureID dryChasmTextureID;
			if (!renderer.tryCreateObjectTexture(1, 1, false, &dryChasmTextureID))
			{
				DebugLogWarning("Couldn't create dry chasm texture.");
				return;
			}

			ScopedObjectTextureRef dryChasmTextureRef(dryChasmTextureID, renderer);
			LockedTexture lockedTexture = renderer.lockObjectTexture(dryChasmTextureID);
			if (!lockedTexture.isValid())
			{
				DebugLogWarning("Couldn't lock dry chasm texture for writing.");
				return;
			}

			DebugAssert(!lockedTexture.isTrueColor);
			uint8_t *texels = static_cast<uint8_t*>(lockedTexture.texels);
			*texels = ArenaRenderUtils::PALETTE_INDEX_DRY_CHASM_COLOR;
			renderer.unlockObjectTexture(dryChasmTextureID);

			ObjectMaterialID chasmMaterialID;
			if (!renderer.tryCreateObjectMaterial(dryChasmTextureID, dryChasmTextureID, &chasmMaterialID)) // @todo: give side texture ID as well
			{
				DebugLogWarning("Couldn't create dry chasm material.");
				return;
			}

			ScopedObjectMaterialRef dryChasmMaterialRef(chasmMaterialID, renderer);

			LoadedChasmMaterialList newMaterialList;
			newMaterialList.init(chasmType);

			LoadedChasmMaterialList::Entry newMaterialListEntry;
			newMaterialListEntry.init(std::move(dryChasmTextureRef), ScopedObjectTextureRef(), std::move(dryChasmMaterialRef)); // @todo: allocate side texture ref
			newMaterialList.add(std::move(newMaterialListEntry));
			this->chasmMaterialLists.emplace_back(std::move(newMaterialList));
		}
	};

	loadChasmTextures(ArenaTypes::ChasmType::Dry);
	loadChasmTextures(ArenaTypes::ChasmType::Wet);
	loadChasmTextures(ArenaTypes::ChasmType::Lava);

	auto loadPaletteTexture = [this, &textureManager, &renderer]()
	{
		const std::string &defaultPaletteFilename = ArenaPaletteName::Default;
		const std::optional<PaletteID> defaultPaletteID = textureManager.tryGetPaletteID(defaultPaletteFilename.c_str());
		if (!defaultPaletteID.has_value())
		{
			DebugLogError("Couldn't get default palette ID from \"" + defaultPaletteFilename + "\".");
			return false;
		}

		const Palette &defaultPalette = textureManager.getPaletteHandle(*defaultPaletteID);

		ObjectTextureID paletteTextureID;
		if (!renderer.tryCreateObjectTexture(static_cast<int>(defaultPalette.size()), 1, true, &paletteTextureID))
		{
			DebugLogError("Couldn't create default palette texture \"" + defaultPaletteFilename + "\".");
			return false;
		}

		this->paletteTextureRef.init(paletteTextureID, renderer);
		LockedTexture lockedPaletteTexture = this->paletteTextureRef.lockTexels();
		if (!lockedPaletteTexture.isValid())
		{
			DebugLogError("Couldn't lock palette texture \"" + defaultPaletteFilename + "\" for writing.");
			return false;
		}

		DebugAssert(lockedPaletteTexture.isTrueColor);
		uint32_t *paletteTexels = static_cast<uint32_t*>(lockedPaletteTexture.texels);
		std::transform(defaultPalette.begin(), defaultPalette.end(), paletteTexels,
			[](const Color &paletteColor)
		{
			return paletteColor.toARGB();
		});

		this->paletteTextureRef.unlockTexels();
		return true;
	};

	auto loadLightTableTexture = [this, &textureManager, &renderer]()
	{
		const std::string &lightTableFilename = ArenaTextureName::NormalLightTable;
		const std::optional<TextureBuilderID> lightTableTextureBuilderID = textureManager.tryGetTextureBuilderID(lightTableFilename.c_str());
		if (!lightTableTextureBuilderID.has_value())
		{
			DebugLogError("Couldn't get light table texture builder ID from \"" + lightTableFilename + "\".");
			return false;
		}

		const TextureBuilder &lightTableTextureBuilder = textureManager.getTextureBuilderHandle(*lightTableTextureBuilderID);

		ObjectTextureID lightTableTextureID;
		if (!renderer.tryCreateObjectTexture(lightTableTextureBuilder, &lightTableTextureID))
		{
			DebugLogError("Couldn't create light table texture \"" + lightTableFilename + "\".");
			return false;
		}

		this->lightTableTextureRef.init(lightTableTextureID, renderer);
		return true;
	};

	if (!loadPaletteTexture())
	{
		DebugLogError("Couldn't load palette texture.");
		return false;
	}

	if (!loadLightTableTexture())
	{
		DebugLogError("Couldn't load light table texture.");
		return false;
	}

	return true;
}

void LevelInstance::update(double dt, Game &game, const CoordDouble3 &playerCoord,
	const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const EntityGeneration::EntityGenInfo &entityGenInfo,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, int chunkDistance,
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, AudioManager &audioManager)
{
	const ChunkInt2 &centerChunk = playerCoord.chunk;
	this->chunkManager.update(dt, centerChunk, playerCoord, activeLevelIndex, mapDefinition, entityGenInfo,
		citizenGenInfo, this->ceilingScale, chunkDistance, entityDefLibrary, binaryAssetLibrary, textureManager,
		audioManager, this->entityManager);

	this->entityManager.tick(game, dt);
}
