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

namespace
{
	// Loads the given voxel definition's textures into the voxel materials list if they haven't been loaded yet.
	void LoadVoxelDefTextures(const VoxelDefinition &voxelDef, std::vector<LevelInstance::LoadedVoxelMaterial> &voxelMaterials,
		TextureManager &textureManager, Renderer &renderer)
	{
		for (int i = 0; i < voxelDef.getTextureAssetReferenceCount(); i++)
		{
			const TextureAssetReference &textureAssetRef = voxelDef.getTextureAssetReference(i);
			const auto cacheIter = std::find_if(voxelMaterials.begin(), voxelMaterials.end(),
				[&textureAssetRef](const LevelInstance::LoadedVoxelMaterial &loadedMaterial)
			{
				return loadedMaterial.textureAssetRef == textureAssetRef;
			});

			if (cacheIter == voxelMaterials.end())
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
				LevelInstance::LoadedVoxelMaterial newMaterial;
				newMaterial.init(textureAssetRef, std::move(voxelTextureRef), std::move(voxelMaterialRef));
				voxelMaterials.emplace_back(std::move(newMaterial));
			}
		}
	}

	// Loads the given entity definition's textures into the entity materials list if they haven't been loaded yet.
	void LoadEntityDefTextures(const EntityDefinition &entityDef, std::vector<LevelInstance::LoadedEntityMaterial> &entityMaterials,
		TextureManager &textureManager, Renderer &renderer)
	{
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
		const bool reflective = (entityDef.getType() == EntityDefinition::Type::Doodad) && entityDef.getDoodad().puddle;

		auto processKeyframe = [&entityMaterials, &textureManager, &renderer, reflective](
			const EntityAnimationDefinition::Keyframe &keyframe, bool flipped)
		{
			const TextureAssetReference &textureAssetRef = keyframe.getTextureAssetRef();
			const auto cacheIter = std::find_if(entityMaterials.begin(), entityMaterials.end(),
				[&textureAssetRef, flipped, reflective](const LevelInstance::LoadedEntityMaterial &loadedMaterial)
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
					return;
				}

				const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
				const int textureWidth = textureBuilder.getWidth();
				const int textureHeight = textureBuilder.getHeight();

				ObjectTextureID entityTextureID;
				if (!renderer.tryCreateObjectTexture(textureWidth, textureHeight, false, &entityTextureID))
				{
					DebugLogWarning("Couldn't create entity texture \"" + textureAssetRef.filename + "\".");
					return;
				}

				ScopedObjectTextureRef entityTextureRef(entityTextureID, renderer);
				DebugAssert(textureBuilder.getType() == TextureBuilder::Type::Paletted);
				const TextureBuilder::PalettedTexture &srcTexture = textureBuilder.getPaletted();
				const uint8_t *srcTexels = srcTexture.texels.get();

				LockedTexture lockedEntityTexture = renderer.lockObjectTexture(entityTextureID);
				if (!lockedEntityTexture.isValid())
				{
					DebugLogWarning("Couldn't lock entity texture \"" + textureAssetRef.filename + "\".");
					return;
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
					return;
				}

				ScopedObjectMaterialRef entityMaterialRef(entityMaterialID, renderer);
				LevelInstance::LoadedEntityMaterial newMaterial;
				newMaterial.init(textureAssetRef, flipped, reflective, std::move(entityTextureRef), std::move(entityMaterialRef));
				entityMaterials.emplace_back(std::move(newMaterial));
			}
		};

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
					processKeyframe(keyframe, flipped);
				}
			}
		}
	}

	// Loads the chasm floor materials for the given chasm. This should be done before loading the chasm wall
	// as each wall material has a dependency on the floor texture.
	void LoadChasmFloorTextures(ArenaTypes::ChasmType chasmType, std::vector<LevelInstance::LoadedChasmMaterialList> &chasmMaterialLists,
		TextureManager &textureManager, Renderer &renderer)
	{
		const auto cacheIter = std::find_if(chasmMaterialLists.begin(), chasmMaterialLists.end(),
			[chasmType](const LevelInstance::LoadedChasmMaterialList &loadedMaterialList)
		{
			return loadedMaterialList.chasmType == chasmType;
		});

		DebugAssertMsg(cacheIter == chasmMaterialLists.end(), "Already loaded chasm floor materials for type \"" +
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

			LevelInstance::LoadedChasmMaterialList newMaterialList;
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
				if (!renderer.tryCreateObjectMaterial(chasmTextureID, &chasmMaterialID))
				{
					DebugLogWarning("Couldn't create chasm floor material \"" + textureAssetRef.filename + "\".");
					return;
				}

				ScopedObjectMaterialRef chasmMaterialRef(chasmMaterialID, renderer);

				// Populate chasmTextureRefs and floorMaterialRefs, leave entries empty.
				newMaterialList.chasmTextureRefs.emplace_back(std::move(chasmTextureRef));
				newMaterialList.floorMaterialRefs.emplace_back(std::move(chasmMaterialRef));
			}

			chasmMaterialLists.emplace_back(std::move(newMaterialList));
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
			if (!renderer.tryCreateObjectMaterial(dryChasmTextureID, &chasmMaterialID))
			{
				DebugLogWarning("Couldn't create dry chasm floor material.");
				return;
			}

			ScopedObjectMaterialRef dryChasmMaterialRef(chasmMaterialID, renderer);
			LevelInstance::LoadedChasmMaterialList newMaterialList;
			newMaterialList.init(chasmType);

			// Populate chasmTextureRefs and floorMaterialRefs, leave entries empty.
			newMaterialList.chasmTextureRefs.emplace_back(std::move(dryChasmTextureRef));
			newMaterialList.floorMaterialRefs.emplace_back(std::move(dryChasmMaterialRef));
			chasmMaterialLists.emplace_back(std::move(newMaterialList));
		}
	}

	// Loads the chasm wall material for the given chasm and texture asset. This expects the chasm floor material to
	// already be loaded and available for sharing.
	void LoadChasmWallTextures(ArenaTypes::ChasmType chasmType, const TextureAssetReference &textureAssetRef,
		std::vector<LevelInstance::LoadedChasmMaterialList> &chasmMaterialLists, TextureManager &textureManager, Renderer &renderer)
	{
		const auto listIter = std::find_if(chasmMaterialLists.begin(), chasmMaterialLists.end(),
			[chasmType](const LevelInstance::LoadedChasmMaterialList &loadedMaterialList)
		{
			return loadedMaterialList.chasmType == chasmType;
		});

		DebugAssertMsg(listIter != chasmMaterialLists.end() && listIter->floorMaterialRefs.size() > 0,
			"Expected loaded chasm floor material list for type \"" + std::to_string(static_cast<int>(chasmType)) + "\".");

		std::vector<LevelInstance::LoadedChasmMaterialList::Entry> &entries = listIter->entries;
		const auto entryIter = std::find_if(entries.begin(), entries.end(),
			[&textureAssetRef](const LevelInstance::LoadedChasmMaterialList::Entry &entry)
		{
			return entry.wallTextureAssetRef == textureAssetRef;
		});

		if (entryIter == entries.end())
		{
			const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
			if (!textureBuilderID.has_value())
			{
				DebugLogWarning("Couldn't load chasm wall texture \"" + textureAssetRef.filename + "\".");
				return;
			}

			const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
			ObjectTextureID wallTextureID;
			if (!renderer.tryCreateObjectTexture(textureBuilder, &wallTextureID))
			{
				DebugLogWarning("Couldn't create chasm wall texture \"" + textureAssetRef.filename + "\".");
				return;
			}

			LevelInstance::LoadedChasmMaterialList::Entry newEntry;
			newEntry.wallTextureAssetRef = textureAssetRef;
			newEntry.wallTextureRef.init(wallTextureID, renderer);

			const std::vector<ScopedObjectTextureRef> &chasmTextureRefs = listIter->chasmTextureRefs;
			const int chasmTextureCount = static_cast<int>(chasmTextureRefs.size());
			for (int i = 0; i < chasmTextureCount; i++)
			{
				const ObjectTextureID floorTextureID = chasmTextureRefs[i].get();
				ObjectMaterialID wallMaterialID;
				if (!renderer.tryCreateObjectMaterial(floorTextureID, wallTextureID, &wallMaterialID))
				{
					DebugLogWarning("Couldn't create chasm wall material \"" + textureAssetRef.filename + "\".");
					continue;
				}

				ScopedObjectMaterialRef wallMaterialRef(wallMaterialID, renderer);
				newEntry.wallMaterialRefs.emplace_back(std::move(wallMaterialRef));
			}

			entries.emplace_back(std::move(newEntry));
		}
	}

	bool TryPopulatePaletteTexture(ScopedObjectTextureRef &paletteTextureRef, TextureManager &textureManager, Renderer &renderer)
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

		paletteTextureRef.init(paletteTextureID, renderer);
		LockedTexture lockedPaletteTexture = paletteTextureRef.lockTexels();
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

		paletteTextureRef.unlockTexels();
		return true;
	}

	bool TryPopulateLightTableTexture(ScopedObjectTextureRef &lightTableTextureRef, TextureManager &textureManager, Renderer &renderer)
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

		lightTableTextureRef.init(lightTableTextureID, renderer);
		return true;
	}
}

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

void LevelInstance::LoadedChasmMaterialList::init(ArenaTypes::ChasmType chasmType)
{
	this->chasmType = chasmType;
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

ObjectMaterialID LevelInstance::getChasmFloorMaterialID(ArenaTypes::ChasmType chasmType, double chasmAnimPercent) const
{
	const auto iter = std::find_if(this->chasmMaterialLists.begin(), this->chasmMaterialLists.end(),
		[chasmType](const LoadedChasmMaterialList &loadedMaterialList)
	{
		return loadedMaterialList.chasmType == chasmType;
	});

	DebugAssertMsg(iter != this->chasmMaterialLists.end(), "No loaded chasm floor material for type \"" +
		std::to_string(static_cast<int>(chasmType)) + "\".");
	const std::vector<ScopedObjectMaterialRef> &floorMaterialRefs = iter->floorMaterialRefs;
	const int textureCount = static_cast<int>(floorMaterialRefs.size());
	const int index = std::clamp(static_cast<int>(static_cast<double>(textureCount) * chasmAnimPercent), 0, textureCount - 1);
	DebugAssertIndex(floorMaterialRefs, index);
	const ScopedObjectMaterialRef &floorMaterialRef = floorMaterialRefs[index];
	return floorMaterialRef.get();
}

ObjectMaterialID LevelInstance::getChasmWallMaterialID(ArenaTypes::ChasmType chasmType, double chasmAnimPercent,
	const TextureAssetReference &textureAssetRef) const
{
	const auto listIter = std::find_if(this->chasmMaterialLists.begin(), this->chasmMaterialLists.end(),
		[chasmType, &textureAssetRef](const LoadedChasmMaterialList &loadedMaterialList)
	{
		return (loadedMaterialList.chasmType == chasmType);
	});

	DebugAssertMsg(listIter != this->chasmMaterialLists.end(), "No loaded chasm floor material for type \"" +
		std::to_string(static_cast<int>(chasmType)) + "\".");

	const std::vector<LoadedChasmMaterialList::Entry> &entries = listIter->entries;
	const auto entryIter = std::find_if(entries.begin(), entries.end(),
		[&textureAssetRef](const LoadedChasmMaterialList::Entry &entry)
	{
		return entry.wallTextureAssetRef == textureAssetRef;
	});

	DebugAssertMsg(entryIter != entries.end(), "No loaded chasm wall material for type \"" +
		std::to_string(static_cast<int>(chasmType)) + "\" and texture \"" + textureAssetRef.filename + "\".");

	const std::vector<ScopedObjectMaterialRef> &wallMaterialRefs = entryIter->wallMaterialRefs;
	const int textureCount = static_cast<int>(wallMaterialRefs.size());
	const int index = std::clamp(static_cast<int>(static_cast<double>(textureCount) * chasmAnimPercent), 0, textureCount - 1);
	DebugAssertIndex(wallMaterialRefs, index);
	const ScopedObjectMaterialRef &wallMaterialRef = wallMaterialRefs[index];
	return wallMaterialRef.get();
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

	// Load chasm floor textures, independent of voxels in the level. Do this before chasm wall texture loading
	// because walls are multi-textured and depend on the chasm animation textures.
	LoadChasmFloorTextures(ArenaTypes::ChasmType::Dry, this->chasmMaterialLists, textureManager, renderer);
	LoadChasmFloorTextures(ArenaTypes::ChasmType::Wet, this->chasmMaterialLists, textureManager, renderer);
	LoadChasmFloorTextures(ArenaTypes::ChasmType::Lava, this->chasmMaterialLists, textureManager, renderer);

	// Load textures known at level load time. Note that none of the object texture IDs allocated here are
	// matched with voxel/entity instances until the chunks containing them are created.
	auto loadLevelDefTextures = [this, &mapDefinition, &textureManager, &renderer](int levelIndex)
	{
		const LevelInfoDefinition &levelInfoDef = mapDefinition.getLevelInfoForLevel(levelIndex);

		for (int i = 0; i < levelInfoDef.getVoxelDefCount(); i++)
		{
			const VoxelDefinition &voxelDef = levelInfoDef.getVoxelDef(i);
			LoadVoxelDefTextures(voxelDef, this->voxelMaterials, textureManager, renderer);

			if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
			{
				const VoxelDefinition::ChasmData &chasm = voxelDef.chasm;
				LoadChasmWallTextures(chasm.type, chasm.textureAssetRef, this->chasmMaterialLists, textureManager, renderer);
			}
		}

		for (int i = 0; i < levelInfoDef.getEntityDefCount(); i++)
		{
			const EntityDefinition &entityDef = levelInfoDef.getEntityDef(i);
			LoadEntityDefTextures(entityDef, this->entityMaterials, textureManager, renderer);
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
		LoadEntityDefTextures(maleEntityDef, this->entityMaterials, textureManager, renderer);
		LoadEntityDefTextures(femaleEntityDef, this->entityMaterials, textureManager, renderer);
	}

	if (!TryPopulatePaletteTexture(this->paletteTextureRef, textureManager, renderer))
	{
		DebugLogError("Couldn't load palette texture.");
		return false;
	}

	if (!TryPopulateLightTableTexture(this->lightTableTextureRef, textureManager, renderer))
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

void LevelInstance::cleanUp()
{
	this->chunkManager.cleanUp();
}
