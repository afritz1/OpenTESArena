#include <algorithm>
#include <unordered_map>

#include "ArenaCityUtils.h"
#include "ArenaInteriorUtils.h"
#include "ArenaLevelUtils.h"
#include "ArenaMeshUtils.h"
#include "ArenaWildUtils.h"
#include "ChunkUtils.h"
#include "LevelDefinition.h"
#include "LevelInfoDefinition.h"
#include "LockDefinition.h"
#include "MapDefinition.h"
#include "MapGeneration.h"
#include "MapType.h"
#include "TransitionDefinition.h"
#include "../Assets/ArenaLevelLibrary.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/MIFUtils.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Entities/ArenaAnimUtils.h"
#include "../Entities/EntityDefinition.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Math/Random.h"
#include "../Voxels/ArenaChasmUtils.h"
#include "../Voxels/ArenaVoxelUtils.h"
#include "../Voxels/VoxelFacing2D.h"
#include "../Voxels/VoxelTriggerDefinition.h"
#include "../WorldMap/ArenaLocationUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/BufferView2D.h"
#include "components/utilities/String.h"

namespace MapGeneration
{
	// Each 2-byte Arena voxel maps to a tuple of definition IDs.
	struct ArenaVoxelMappingEntry
	{
		LevelVoxelShapeDefID shapeDefID;
		LevelVoxelTextureDefID textureDefID;
		LevelVoxelShadingDefID shadingDefID;
		LevelVoxelTraitsDefID traitsDefID;

		ArenaVoxelMappingEntry()
		{
			this->shapeDefID = -1;
			this->textureDefID = -1;
			this->shadingDefID = -1;
			this->traitsDefID = -1;
		}

		void init(LevelVoxelShapeDefID shapeDefID, LevelVoxelTextureDefID textureDefID, LevelVoxelShadingDefID shadingDefID, LevelVoxelTraitsDefID traitsDefID)
		{
			this->shapeDefID = shapeDefID;
			this->textureDefID = textureDefID;
			this->shadingDefID = shadingDefID;
			this->traitsDefID = traitsDefID;
		}
	};

	struct TransitionMappingKey
	{
		ArenaTypes::VoxelID voxelID;
		WorldInt2 levelXZ; // .MIF name generation depends on XY coordinate.

		TransitionMappingKey()
		{
			this->voxelID = 0;
		}
	};

	// Mapping caches of .MIF/.RMD voxels, etc. to modern level info entries.
	using ArenaVoxelMappingCache = std::unordered_map<ArenaTypes::VoxelID, ArenaVoxelMappingEntry>;
	using ArenaEntityMappingCache = std::unordered_map<ArenaTypes::VoxelID, LevelVoxelEntityDefID>;
	using ArenaLockMappingCache = std::vector<std::pair<ArenaTypes::MIFLock, LevelVoxelLockDefID>>;
	using ArenaTriggerMappingCache = std::vector<std::pair<ArenaTypes::MIFTrigger, LevelVoxelTriggerDefID>>;
	using ArenaTransitionMappingCache = std::vector<std::pair<TransitionMappingKey, LevelVoxelTransitionDefID>>;
	using ArenaBuildingNameMappingCache = std::unordered_map<std::string, LevelVoxelBuildingNameID>;
	using ArenaDoorMappingCache = std::unordered_map<ArenaTypes::VoxelID, LevelVoxelDoorDefID>;
	using ArenaChasmMappingCache = std::unordered_map<ArenaTypes::VoxelID, LevelVoxelChasmDefID>;

	// Converts the given Arena *MENU ID to a modern interior type, if any.
	std::optional<ArenaTypes::InteriorType> tryGetInteriorTypeFromMenuIndex(int menuIndex, MapType mapType)
	{
		if (mapType == MapType::City)
		{
			// Mappings of Arena city *MENU IDs to interiors.
			constexpr std::array<std::pair<int, ArenaTypes::InteriorType>, 11> CityMenuMappings =
			{
				{
					{ 0, ArenaTypes::InteriorType::Equipment },
					{ 1, ArenaTypes::InteriorType::Tavern },
					{ 2, ArenaTypes::InteriorType::MagesGuild },
					{ 3, ArenaTypes::InteriorType::Temple },
					{ 4, ArenaTypes::InteriorType::House },
					{ 5, ArenaTypes::InteriorType::House },
					{ 6, ArenaTypes::InteriorType::House },
					// 7 - city gate
					// 8 - city gate
					{ 9, ArenaTypes::InteriorType::Noble },
					// 10 - none
					{ 11, ArenaTypes::InteriorType::Palace },
					{ 12, ArenaTypes::InteriorType::Palace },
					{ 13, ArenaTypes::InteriorType::Palace }
				}
			};

			const auto iter = std::find_if(CityMenuMappings.begin(), CityMenuMappings.end(),
				[menuIndex](const auto &pair)
			{
				return pair.first == menuIndex;
			});

			if (iter != CityMenuMappings.end())
			{
				return iter->second;
			}
			else
			{
				return std::nullopt;
			}
		}
		else if (mapType == MapType::Wilderness)
		{
			// Mappings of Arena wilderness *MENU IDs to interiors.
			constexpr std::array<std::pair<int, ArenaTypes::InteriorType>, 7> WildMenuMappings =
			{
				{
					// 0 - none
					{ 1, ArenaTypes::InteriorType::Crypt },
					{ 2, ArenaTypes::InteriorType::House },
					{ 3, ArenaTypes::InteriorType::Tavern },
					{ 4, ArenaTypes::InteriorType::Temple },
					{ 5, ArenaTypes::InteriorType::Tower },
					// 6 - city gate
					// 7 - city gate
					{ 8, ArenaTypes::InteriorType::Dungeon },
					{ 9, ArenaTypes::InteriorType::Dungeon }
				}
			};

			const auto iter = std::find_if(WildMenuMappings.begin(), WildMenuMappings.end(),
				[menuIndex](const auto &pair)
			{
				return pair.first == menuIndex;
			});

			if (iter != WildMenuMappings.end())
			{
				return iter->second;
			}
			else
			{
				return std::nullopt;
			}
		}
		else
		{
			DebugUnhandledReturnMsg(std::optional<ArenaTypes::InteriorType>, std::to_string(static_cast<int>(mapType)));
		}
	}

	MapGeneration::InteriorGenInfo makePrefabInteriorGenInfo(ArenaTypes::InteriorType interiorType,
		const WorldInt3 &position, int menuID, uint32_t rulerSeed, const std::optional<bool> &rulerIsMale,
		bool palaceIsMainQuestDungeon, ArenaTypes::CityType cityType, MapType mapType, const ExeData &exeData)
	{
		const OriginalInt2 originalPos = VoxelUtils::worldVoxelToOriginalVoxel(WorldInt2(position.x, position.z));
		std::string mifName = ArenaLevelUtils::getDoorVoxelMifName(originalPos.x, originalPos.y, menuID,
			rulerSeed, palaceIsMainQuestDungeon, cityType, mapType, exeData);

		const ArenaTypes::InteriorType revisedInteriorType =
			(palaceIsMainQuestDungeon && interiorType == ArenaTypes::InteriorType::Palace) ?
			ArenaTypes::InteriorType::Dungeon : interiorType;

		MapGeneration::InteriorGenInfo interiorGenInfo;
		interiorGenInfo.initPrefab(mifName, revisedInteriorType, rulerIsMale);
		return interiorGenInfo;
	}

	MapGeneration::InteriorGenInfo makeProceduralInteriorGenInfo(
		const LocationDungeonDefinition &dungeonDef, bool isArtifactDungeon)
	{
		MapGeneration::InteriorGenInfo interiorGenInfo;
		interiorGenInfo.initDungeon(dungeonDef, isArtifactDungeon);
		return interiorGenInfo;
	}

	// Makes a modern entity definition from the given Arena FLAT index.
	// @todo: probably want this to be some 'LevelEntityDefinition' with no dependencies on runtime
	// textures and animations handles, instead using texture filenames for the bulk of things.
	bool tryMakeEntityDefFromArenaFlat(ArenaTypes::FlatIndex flatIndex, MapType mapType,
		const std::optional<ArenaTypes::InteriorType> &interiorType, const std::optional<bool> &rulerIsMale,
		const INFFile &inf, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, EntityDefinition *outDef)
	{
		const INFFlat &flatData = inf.getFlat(flatIndex);
		const bool isDynamicEntity = ArenaAnimUtils::isDynamicEntity(flatIndex, inf);
		const std::optional<ArenaTypes::ItemIndex> &optItemIndex = flatData.itemIndex;

		bool isCreature = false;
		bool isCreatureFinalBoss = false;
		bool isHumanEnemy = false;
		bool isPileContainer = false;
		bool isLockedHolderContainer = false;
		bool isUnlockedHolderContainer = false;
		bool isKey = false;
		bool isQuestItem = false;
		if (optItemIndex.has_value())
		{
			isCreature = ArenaAnimUtils::isCreatureIndex(*optItemIndex, &isCreatureFinalBoss);
			isHumanEnemy = ArenaAnimUtils::isHumanEnemyIndex(*optItemIndex);
			isPileContainer = ArenaAnimUtils::isTreasurePileContainerIndex(*optItemIndex);
			isLockedHolderContainer = ArenaAnimUtils::isLockedHolderContainerIndex(*optItemIndex);
			isUnlockedHolderContainer = ArenaAnimUtils::isUnlockedHolderContainerIndex(*optItemIndex);
			isKey = *optItemIndex == ArenaAnimUtils::KeyItemIndex;
			isQuestItem = *optItemIndex == ArenaAnimUtils::QuestItemIndex;
		}

		// Add entity animation data. Static entities have only idle animations (and maybe on/off
		// state for lampposts). Dynamic entities have several animation states and directions.
		EntityAnimationDefinition entityAnimDef;
		if (!isDynamicEntity)
		{
			if (!ArenaAnimUtils::tryMakeStaticEntityAnims(flatIndex, mapType, interiorType, rulerIsMale, inf, textureManager, &entityAnimDef))
			{
				DebugLogWarning("Couldn't make static entity anims for flat \"" + std::to_string(flatIndex) + "\".");
				return false;
			}
		}
		else
		{
			// Assume that human enemies in level data are male.
			const std::optional<bool> isMale = true;
			if (!ArenaAnimUtils::tryMakeDynamicEntityAnims(flatIndex, isMale, inf, charClassLibrary, binaryAssetLibrary, textureManager, &entityAnimDef))
			{
				DebugLogWarning("Couldn't make dynamic entity anims for flat \"" + std::to_string(flatIndex) + "\".");
				return false;
			}
		}

		DebugAssert(!String::isNullOrEmpty(entityAnimDef.initialStateName));

		if (isCreature)
		{
			const ArenaTypes::ItemIndex itemIndex = *optItemIndex;
			const int creatureID = isCreatureFinalBoss ? ArenaAnimUtils::FinalBossCreatureID : ArenaAnimUtils::getCreatureIDFromItemIndex(itemIndex);
			const int creatureIndex = ArenaAnimUtils::getCreatureIndexFromID(creatureID);

			// @todo: read from EntityDefinitionLibrary instead, and don't make anim def above.
			// Currently these are just going to be duplicates of defs in the library.
			EntityDefinitionKey entityDefKey;
			entityDefKey.initCreature(creatureIndex, isCreatureFinalBoss);

			EntityDefID entityDefID;
			if (!entityDefLibrary.tryGetDefinitionID(entityDefKey, &entityDefID))
			{
				DebugLogWarning("Couldn't get creature definition " + std::to_string(creatureIndex) + " from library.");
				return false;
			}

			*outDef = entityDefLibrary.getDefinition(entityDefID);
		}
		else if (isHumanEnemy)
		{
			const bool male = true; // Always male from map data.
			const int charClassID = ArenaAnimUtils::getCharacterClassIndexFromItemIndex(*optItemIndex);
			outDef->initEnemyHuman(male, charClassID, std::move(entityAnimDef));
		}
		else if (isPileContainer)
		{
			outDef->initContainerPile(std::move(entityAnimDef));
		}
		else if (isLockedHolderContainer || isUnlockedHolderContainer)
		{
			outDef->initContainerHolder(isLockedHolderContainer,  std::move(entityAnimDef));
		}
		else if (isKey)
		{
			outDef->initItemKey(std::move(entityAnimDef));
		}
		else if (isQuestItem)
		{
			outDef->initItemQuestItem(flatData.yOffset, std::move(entityAnimDef));
		}
		else
		{
			// Decoration.
			const bool streetLight = ArenaAnimUtils::isStreetLightFlatIndex(flatIndex, mapType);
			const double scale = ArenaAnimUtils::getDimensionModifier(flatData);
			const int lightIntensity = flatData.lightIntensity.has_value() ? *flatData.lightIntensity : 0;

			// @todo: TransitionDefID from flatIndex -- use MapGeneration::isMap1TransitionEntity().
			// @todo: support wild den transitions here. Might need to pass the transition cache here.

			outDef->initDecoration(flatData.yOffset, scale, flatData.collider, flatData.transparent, flatData.ceiling, streetLight,
				flatData.puddle, lightIntensity, std::move(entityAnimDef));
		}

		return true;
	}

	void writeVoxelInfoForFLOR(ArenaTypes::VoxelID florVoxel, MapType mapType, const INFFile &inf, ArenaTypes::VoxelType *outVoxelType,
		ArenaMeshUtils::ShapeInitCache *outShapeInitCache, TextureAsset *outTextureAsset, VertexShaderType *outVertexShaderType,
		BufferView<PixelShaderType> outPixelShaderTypes, int *outPixelShaderCount, bool *outIsChasm, bool *outIsWildWallColored,
		ArenaTypes::ChasmType *outChasmType)
	{
		const int textureID = (florVoxel & 0xFF00) >> 8;
		*outIsChasm = MIFUtils::isChasm(textureID);

		// Determine if the floor voxel is either solid or a chasm.
		if (!*outIsChasm)
		{
			*outVoxelType = ArenaTypes::VoxelType::Floor;

			outShapeInitCache->initDefaultBoxValues();
			ArenaMeshUtils::WriteFloorRendererGeometryBuffers(outShapeInitCache->verticesView, outShapeInitCache->normalsView, outShapeInitCache->texCoordsView);
			ArenaMeshUtils::WriteFloorRendererIndexBuffers(outShapeInitCache->indices0View);

			const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(textureID);
			*outTextureAsset = TextureAsset(
				ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
				ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
			*outVertexShaderType = VertexShaderType::Basic;
			outPixelShaderTypes[0] = PixelShaderType::Opaque;
			*outPixelShaderCount = 1;
			*outIsWildWallColored = ArenaVoxelUtils::isFloorWildWallColored(textureID, mapType);
			*outChasmType = static_cast<ArenaTypes::ChasmType>(-1);
		}
		else
		{
			*outVoxelType = ArenaTypes::VoxelType::Chasm;

			int chasmID;
			PixelShaderType chasmFloorPixelShaderType;
			PixelShaderType chasmWallPixelShaderType;
			if (textureID == MIFUtils::DRY_CHASM)
			{
				const std::optional<int> &dryChasmIndex = inf.getDryChasmIndex();
				if (dryChasmIndex.has_value())
				{
					chasmID = *dryChasmIndex;
				}
				else
				{
					DebugLogWarning("Missing *DRYCHASM ID.");
					chasmID = 0;
				}

				*outChasmType = ArenaTypes::ChasmType::Dry;
				chasmFloorPixelShaderType = PixelShaderType::Opaque;
				chasmWallPixelShaderType = PixelShaderType::OpaqueWithAlphaTestLayer;
			}
			else if (textureID == MIFUtils::LAVA_CHASM)
			{
				const std::optional<int> &lavaChasmIndex = inf.getLavaChasmIndex();
				if (lavaChasmIndex.has_value())
				{
					chasmID = *lavaChasmIndex;
				}
				else
				{
					DebugLogWarning("Missing *LAVACHASM ID.");
					chasmID = 0;
				}

				*outChasmType = ArenaTypes::ChasmType::Lava;
				chasmFloorPixelShaderType = PixelShaderType::OpaqueScreenSpaceAnimation;
				chasmWallPixelShaderType = PixelShaderType::OpaqueScreenSpaceAnimationWithAlphaTestLayer;
			}
			else if (textureID == MIFUtils::WET_CHASM)
			{
				const std::optional<int> &wetChasmIndex = inf.getWetChasmIndex();
				if (wetChasmIndex.has_value())
				{
					chasmID = *wetChasmIndex;
				}
				else
				{
					DebugLogWarning("Missing *WETCHASM ID.");
					chasmID = 0;
				}

				*outChasmType = ArenaTypes::ChasmType::Wet;
				chasmFloorPixelShaderType = PixelShaderType::OpaqueScreenSpaceAnimation;
				chasmWallPixelShaderType = PixelShaderType::OpaqueScreenSpaceAnimationWithAlphaTestLayer;
			}
			else
			{
				DebugCrash("Unsupported chasm type \"" + std::to_string(textureID) + "\".");
			}

			const bool isDryChasm = !ArenaChasmUtils::allowsSwimming(*outChasmType);
			outShapeInitCache->initChasmBoxValues(isDryChasm);
			ArenaMeshUtils::WriteChasmRendererGeometryBuffers(*outChasmType, outShapeInitCache->verticesView, outShapeInitCache->normalsView, outShapeInitCache->texCoordsView);
			ArenaMeshUtils::WriteChasmFloorRendererIndexBuffers(outShapeInitCache->indices0View);

			const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(chasmID);
			*outTextureAsset = TextureAsset(
				ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
				ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
			*outVertexShaderType = VertexShaderType::Basic;
			outPixelShaderTypes[0] = chasmFloorPixelShaderType;
			outPixelShaderTypes[1] = chasmWallPixelShaderType;
			*outPixelShaderCount = 2;
			*outIsWildWallColored = false;
		}
	}

	void writeDefsForFLOR(ArenaTypes::VoxelID florVoxel, MapType mapType, const INFFile &inf, VoxelShapeDefinition *outShapeDef,
		VoxelTextureDefinition *outTextureDef, VoxelShadingDefinition *outShadingDef, VoxelTraitsDefinition *outTraitsDef)
	{
		ArenaTypes::VoxelType voxelType;
		ArenaMeshUtils::ShapeInitCache shapeInitCache;
		TextureAsset textureAsset;
		VertexShaderType vertexShaderType;
		PixelShaderType pixelShaderTypes[VoxelShadingDefinition::MAX_PIXEL_SHADERS];
		int pixelShaderCount;
		bool isChasm;
		bool isWildWallColored;
		ArenaTypes::ChasmType chasmType;
		MapGeneration::writeVoxelInfoForFLOR(florVoxel, mapType, inf, &voxelType, &shapeInitCache, &textureAsset, &vertexShaderType, pixelShaderTypes, &pixelShaderCount,
			&isChasm, &isWildWallColored, &chasmType);

		VoxelShapeScaleType scaleType = VoxelShapeScaleType::ScaledFromMin;
		if (isChasm && (chasmType != ArenaTypes::ChasmType::Dry))
		{
			scaleType = VoxelShapeScaleType::UnscaledFromMax;
		}

		outShapeDef->initBoxFromClassic(voxelType, scaleType, shapeInitCache);
		outTextureDef->addTextureAsset(std::move(textureAsset));
		
		outShadingDef->init(vertexShaderType);
		for (int i = 0; i < pixelShaderCount; i++)
		{
			outShadingDef->addPixelShaderType(pixelShaderTypes[i]);
		}

		switch (voxelType)
		{
		case ArenaTypes::VoxelType::Floor:
			outTraitsDef->initFloor(isWildWallColored);
			break;
		case ArenaTypes::VoxelType::Chasm:
			outTraitsDef->initChasm(chasmType);
			break;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(voxelType)));
		}
	}

	void writeVoxelInfoForFloorReplacement(const INFFile &inf, ArenaTypes::ChasmType chasmType,
		ArenaMeshUtils::ShapeInitCache *outShapeInitCache, TextureAsset *outTextureAsset)
	{
		const bool isDryChasm = !ArenaChasmUtils::allowsSwimming(chasmType);
		outShapeInitCache->initChasmBoxValues(isDryChasm);
		ArenaMeshUtils::WriteChasmRendererGeometryBuffers(chasmType, outShapeInitCache->verticesView, outShapeInitCache->normalsView, outShapeInitCache->texCoordsView);
		ArenaMeshUtils::WriteChasmFloorRendererIndexBuffers(outShapeInitCache->indices0View);

		std::optional<int> textureIndex = inf.getWetChasmIndex();
		if (!textureIndex.has_value())
		{
			DebugLogError("Missing *WETCHASM index for floor replacement voxel info, defaulting to 0.");
			textureIndex = 0;
		}

		const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(*textureIndex);
		*outTextureAsset = TextureAsset(
			ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
			ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
	}

	void writeDefsForFloorReplacement(const INFFile &inf, TextureManager &textureManager, VoxelShapeDefinition *outShapeDef,
		VoxelTextureDefinition *outTextureDef, VoxelShadingDefinition *outShadingDef, VoxelTraitsDefinition *outTraitsDef, VoxelChasmDefinition *outChasmDef)
	{
		constexpr ArenaTypes::VoxelType voxelType = ArenaTypes::VoxelType::Chasm;
		constexpr ArenaTypes::ChasmType chasmType = ArenaTypes::ChasmType::Wet;

		ArenaMeshUtils::ShapeInitCache shapeInitCache;
		TextureAsset textureAsset;
		MapGeneration::writeVoxelInfoForFloorReplacement(inf, chasmType, &shapeInitCache, &textureAsset);

		constexpr VoxelShapeScaleType scaleType = VoxelShapeScaleType::UnscaledFromMax;
		outShapeDef->initBoxFromClassic(voxelType, scaleType, shapeInitCache);
		outTextureDef->addTextureAsset(TextureAsset(textureAsset));
		
		outShadingDef->init(VertexShaderType::Basic);
		outShadingDef->addPixelShaderType(PixelShaderType::OpaqueScreenSpaceAnimation);
		outShadingDef->addPixelShaderType(PixelShaderType::OpaqueScreenSpaceAnimationWithAlphaTestLayer);

		outTraitsDef->initChasm(chasmType);
		outChasmDef->initClassic(chasmType, textureAsset, textureManager);
	}

	void writeVoxelInfoForMAP1(ArenaTypes::VoxelID map1Voxel, uint8_t mostSigNibble, MapType mapType, const INFFile &inf,
		const ExeData &exeData, ArenaTypes::VoxelType *outVoxelType, ArenaMeshUtils::ShapeInitCache *outShapeInitCache,
		TextureAsset *outTextureAsset0, TextureAsset *outTextureAsset1, TextureAsset *outTextureAsset2, VertexShaderType *outVertexShaderType,
		BufferView<PixelShaderType> outPixelShaderTypes, int *outPixelShaderCount, bool *outIsCollider, VoxelFacing2D *outFacing)
	{
		DebugAssert(map1Voxel != 0);
		DebugAssert(mostSigNibble != 0x8);

		if ((map1Voxel & 0x8000) == 0)
		{
			const uint8_t mostSigByte = ArenaLevelUtils::getVoxelMostSigByte(map1Voxel);
			const uint8_t leastSigByte = ArenaLevelUtils::getVoxelLeastSigByte(map1Voxel);
			const bool voxelIsSolid = mostSigByte == leastSigByte;

			if (voxelIsSolid)
			{
				// Regular solid wall.
				*outVoxelType = ArenaTypes::VoxelType::Wall;
				outShapeInitCache->initDefaultBoxValues();
				ArenaMeshUtils::WriteWallRendererGeometryBuffers(outShapeInitCache->verticesView, outShapeInitCache->normalsView, outShapeInitCache->texCoordsView);
				ArenaMeshUtils::WriteWallRendererIndexBuffers(outShapeInitCache->indices0View, outShapeInitCache->indices1View, outShapeInitCache->indices2View);

				const int textureIndex = mostSigByte - 1;
				const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(textureIndex);
				std::string voxelTextureFilename = ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf);
				const std::optional<int> voxelTextureSetIndex = ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf);
				*outTextureAsset0 = TextureAsset(std::move(voxelTextureFilename), voxelTextureSetIndex);
				*outTextureAsset1 = *outTextureAsset0;
				*outTextureAsset2 = *outTextureAsset0;
				*outVertexShaderType = VertexShaderType::Basic;
				outPixelShaderTypes[0] = PixelShaderType::Opaque;
				outPixelShaderTypes[1] = PixelShaderType::Opaque;
				outPixelShaderTypes[2] = PixelShaderType::Opaque;
				*outPixelShaderCount = 3;
			}
			else
			{
				// Raised platform.
				*outVoxelType = ArenaTypes::VoxelType::Raised;

				const uint8_t wallTextureID = map1Voxel & 0x000F;
				const uint8_t capTextureID = (map1Voxel & 0x00F0) >> 4;

				const int sideID = [&inf, wallTextureID]()
				{
					const std::optional<int> &id = inf.getBoxSide(wallTextureID);
					if (id.has_value())
					{
						return *id;
					}
					else
					{
						DebugLogWarning("Missing *BOXSIDE ID \"" + std::to_string(wallTextureID) + "\" for raised platform side.");
						return 0;
					}
				}();

				const int floorID = [&inf]()
				{
					const auto &id = inf.getCeiling().textureIndex;
					if (id.has_value())
					{
						return id.value();
					}
					else
					{
						DebugLogWarning("Missing *CEILING texture ID for raised platform floor.");
						return 0;
					}
				}();

				const int ceilingID = [&inf, capTextureID]()
				{
					const std::optional<int> &id = inf.getBoxCap(capTextureID);
					if (id.has_value())
					{
						return *id;
					}
					else
					{
						DebugLogWarning("Missing *BOXCAP ID \"" + std::to_string(capTextureID) + "\" for raised platform ceiling.");
						return 0;
					}
				}();

				const auto &raisedPlatforms = exeData.raisedPlatforms;
				const int heightIndex = mostSigByte & 0x07;
				const int thicknessIndex = (mostSigByte & 0x78) >> 3;

				int baseOffset, baseSize;
				if (mapType == MapType::Interior)
				{
					baseOffset = raisedPlatforms.heightsInterior.get(heightIndex);

					const int boxSize = raisedPlatforms.thicknessesInterior.get(thicknessIndex);
					const auto &boxScale = inf.getCeiling().boxScale;
					baseSize = boxScale.has_value() ? ((boxSize * (*boxScale)) / 256) : boxSize;
				}
				else if (mapType == MapType::City)
				{
					baseOffset = raisedPlatforms.heightsCity.get(heightIndex);
					baseSize = raisedPlatforms.thicknessesCity.get(thicknessIndex);
				}
				else if (mapType == MapType::Wilderness)
				{
					baseOffset = raisedPlatforms.heightsWild.get(heightIndex);

					constexpr int boxSize = 32;
					const auto &boxScale = inf.getCeiling().boxScale;
					baseSize = (boxSize * (boxScale.has_value() ? boxScale.value() : 192)) / 256;
				}
				else
				{
					DebugNotImplementedMsg(std::to_string(static_cast<int>(mapType)));
				}

				const double yOffset = static_cast<double>(baseOffset) / MIFUtils::ARENA_UNITS;
				const double ySize = static_cast<double>(baseSize) / MIFUtils::ARENA_UNITS;
				const double normalizedScale = static_cast<double>(inf.getCeiling().height) / MIFUtils::ARENA_UNITS;
				const double yOffsetNormalized = yOffset / normalizedScale;
				const double ySizeNormalized = ySize / normalizedScale;

				// @todo: might need some tweaking with box3/box4 values.
				const double vTop = std::max(0.0, 1.0 - yOffsetNormalized - ySizeNormalized);
				const double vBottom = std::min(vTop + ySizeNormalized, 1.0);

				outShapeInitCache->initRaisedBoxValues(ySize, yOffset);
				ArenaMeshUtils::WriteRaisedRendererGeometryBuffers(yOffset, ySize, vBottom, vTop, outShapeInitCache->verticesView, outShapeInitCache->normalsView, outShapeInitCache->texCoordsView);
				ArenaMeshUtils::WriteRaisedRendererIndexBuffers(outShapeInitCache->indices0View, outShapeInitCache->indices1View, outShapeInitCache->indices2View);

				const int clampedSideID = ArenaVoxelUtils::clampVoxelTextureID(sideID);
				const int clampedFloorID = ArenaVoxelUtils::clampVoxelTextureID(floorID);
				const int clampedCeilingID = ArenaVoxelUtils::clampVoxelTextureID(ceilingID);
				*outTextureAsset0 = TextureAsset(
					ArenaVoxelUtils::getVoxelTextureFilename(clampedSideID, inf),
					ArenaVoxelUtils::getVoxelTextureSetIndex(clampedSideID, inf));
				*outTextureAsset1 = TextureAsset(
					ArenaVoxelUtils::getVoxelTextureFilename(clampedFloorID, inf),
					ArenaVoxelUtils::getVoxelTextureSetIndex(clampedFloorID, inf));
				*outTextureAsset2 = TextureAsset(
					ArenaVoxelUtils::getVoxelTextureFilename(clampedCeilingID, inf),
					ArenaVoxelUtils::getVoxelTextureSetIndex(clampedCeilingID, inf));
				*outVertexShaderType = VertexShaderType::Basic;
				outPixelShaderTypes[0] = PixelShaderType::AlphaTested;
				outPixelShaderTypes[1] = PixelShaderType::Opaque;
				outPixelShaderTypes[2] = PixelShaderType::Opaque;
				*outPixelShaderCount = 3;
			}
		}
		else
		{
			if (mostSigNibble == 0x9)
			{
				// Transparent block with 1-sided texture on all sides, such as wooden arches in dungeons.
				// These do not have back-faces (especially when standing in the voxel itself).
				*outVoxelType = ArenaTypes::VoxelType::TransparentWall;

				outShapeInitCache->initDefaultBoxValues();
				ArenaMeshUtils::WriteTransparentWallRendererGeometryBuffers(outShapeInitCache->verticesView, outShapeInitCache->normalsView, outShapeInitCache->texCoordsView);
				ArenaMeshUtils::WriteTransparentWallRendererIndexBuffers(outShapeInitCache->indices0View);

				const int textureIndex = (map1Voxel & 0x00FF) - 1;
				const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(textureIndex);
				*outTextureAsset0 = TextureAsset(
					ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
					ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
				*outVertexShaderType = VertexShaderType::Basic;
				outPixelShaderTypes[0] = PixelShaderType::AlphaTested;
				*outPixelShaderCount = 1;
				*outIsCollider = (map1Voxel & 0x0100) == 0;
			}
			else if (mostSigNibble == 0xA)
			{
				// Transparent block with 2-sided texture on one side (i.e. fence). Note that in
				// the center province's city, there is a temple voxel with zeroes for its texture
				// index, and it appears solid gray in the original game (presumably a silent bug).
				*outVoxelType = ArenaTypes::VoxelType::Edge;

				int textureIndex = (map1Voxel & 0x003F) - 1;
				if (textureIndex < 0)
				{
					DebugLogWarning("Invalid texture index \"" + std::to_string(textureIndex) + "\" for type 0xA voxel; defaulting to 0.");
					textureIndex = 0;
				}

				const double yOffset = [mapType, map1Voxel]()
				{
					const int baseOffset = (map1Voxel & 0x0E00) >> 9;
					const int fullOffset = (mapType == MapType::Interior) ?
						(baseOffset * 8) : ((baseOffset * 32) - 8);

					return static_cast<double>(fullOffset) / MIFUtils::ARENA_UNITS;
				}();

				const bool collider = (map1Voxel & 0x0100) != 0;

				// "Flipped" is not present in the original game, but has been added
				// here so that all edge voxel texture coordinates (i.e., palace
				// graphics, store signs) can be correct. Currently only palace
				// graphics and gates are type 0xA colliders, I believe.
				const bool flipped = collider;

				const VoxelFacing2D facing = [map1Voxel]()
				{
					// Orientation is a multiple of 4 (0, 4, 8, C), where 0 is north
					// and C is east. It is stored in two bits above the texture index.
					const int orientation = (map1Voxel & 0x00C0) >> 4;
					if (orientation == 0x0)
					{
						return VoxelFacing2D::NegativeX;
					}
					else if (orientation == 0x4)
					{
						return VoxelFacing2D::PositiveZ;
					}
					else if (orientation == 0x8)
					{
						return VoxelFacing2D::PositiveX;
					}
					else
					{
						return VoxelFacing2D::NegativeZ;
					}
				}();

				outShapeInitCache->initDefaultBoxValues();
				ArenaMeshUtils::WriteEdgeRendererGeometryBuffers(facing, yOffset, flipped, outShapeInitCache->verticesView, outShapeInitCache->normalsView, outShapeInitCache->texCoordsView);
				ArenaMeshUtils::WriteEdgeRendererIndexBuffers(outShapeInitCache->indices0View);

				const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(textureIndex);
				*outTextureAsset0 = TextureAsset(
					ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
					ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
				*outVertexShaderType = VertexShaderType::Basic;
				outPixelShaderTypes[0] = PixelShaderType::AlphaTested;
				*outPixelShaderCount = 1;
				*outIsCollider = collider;
				*outFacing = facing;
			}
			else if (mostSigNibble == 0xB)
			{
				// Door voxel.
				*outVoxelType = ArenaTypes::VoxelType::Door;

				const int textureIndex = (map1Voxel & 0x003F) - 1;

				ArenaTypes::DoorType doorType;
				VertexShaderType doorVertexShaderType;
				PixelShaderType doorPixelShaderType;
				const int type = (map1Voxel & 0x00C0) >> 4;
				if (type == 0x0)
				{
					doorType = ArenaTypes::DoorType::Swinging;
					doorVertexShaderType = VertexShaderType::Basic;
					doorPixelShaderType = PixelShaderType::AlphaTested;
				}
				else if (type == 0x4)
				{
					doorType = ArenaTypes::DoorType::Sliding;
					doorVertexShaderType = VertexShaderType::Basic;
					doorPixelShaderType = PixelShaderType::AlphaTestedWithVariableTexCoordUMin;
				}
				else if (type == 0x8)
				{
					doorType = ArenaTypes::DoorType::Raising;
					doorVertexShaderType = VertexShaderType::RaisingDoor;
					doorPixelShaderType = PixelShaderType::AlphaTestedWithVariableTexCoordVMin;
				}
				else
				{
					// Arena doesn't seem to have splitting doors, but they are supported.
					DebugLogWarningFormat("Unrecognized door type \"%d\", treating as splitting.", type);
					doorType = ArenaTypes::DoorType::Splitting;
					doorVertexShaderType = VertexShaderType::Basic;
					doorPixelShaderType = PixelShaderType::AlphaTested;
				}

				outShapeInitCache->initDefaultBoxValues();
				ArenaMeshUtils::WriteDoorRendererGeometryBuffers(outShapeInitCache->verticesView, outShapeInitCache->normalsView, outShapeInitCache->texCoordsView);
				ArenaMeshUtils::WriteDoorRendererIndexBuffers(outShapeInitCache->indices0View);

				const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(textureIndex);
				*outTextureAsset0 = TextureAsset(
					ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
					ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
				*outVertexShaderType = doorVertexShaderType;
				outPixelShaderTypes[0] = doorPixelShaderType;
				*outPixelShaderCount = 1;
			}
			else if (mostSigNibble == 0xC)
			{
				// Unknown.
				DebugLogWarning("Unrecognized voxel type 0xC.");
				*outVoxelType = ArenaTypes::VoxelType::None;
			}
			else if (mostSigNibble == 0xD)
			{
				// Diagonal wall.
				*outVoxelType = ArenaTypes::VoxelType::Diagonal;

				const bool isRightDiag = (map1Voxel & 0x0100) == 0;
				outShapeInitCache->initDiagonalBoxValues(isRightDiag);
				ArenaMeshUtils::WriteDiagonalRendererGeometryBuffers(isRightDiag, outShapeInitCache->verticesView, outShapeInitCache->normalsView, outShapeInitCache->texCoordsView);
				ArenaMeshUtils::WriteDiagonalRendererIndexBuffers(outShapeInitCache->indices0View);

				const int textureIndex = (map1Voxel & 0x00FF) - 1;
				const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(textureIndex);
				*outTextureAsset0 = TextureAsset(
					ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
					ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
				*outVertexShaderType = VertexShaderType::Basic;
				outPixelShaderTypes[0] = PixelShaderType::Opaque;
				*outPixelShaderCount = 1;
			}
			else
			{
				DebugNotImplementedMsg(std::to_string(mostSigNibble));
			}
		}
	}

	void writeDefsForMAP1(ArenaTypes::VoxelID map1Voxel, uint8_t mostSigNibble, MapType mapType, const INFFile &inf, const ExeData &exeData,
		VoxelShapeDefinition *outShapeDef, VoxelTextureDefinition *outTextureDef, VoxelShadingDefinition *outShadingDef, VoxelTraitsDefinition *outTraitsDef)
	{
		ArenaTypes::VoxelType voxelType;
		ArenaMeshUtils::ShapeInitCache shapeInitCache;
		TextureAsset textureAsset0, textureAsset1, textureAsset2;
		VertexShaderType vertexShaderType;
		PixelShaderType pixelShaderTypes[VoxelShadingDefinition::MAX_PIXEL_SHADERS];
		int pixelShaderCount;
		bool isCollider;
		VoxelFacing2D facing;
		MapGeneration::writeVoxelInfoForMAP1(map1Voxel, mostSigNibble, mapType, inf, exeData, &voxelType, &shapeInitCache,
			&textureAsset0, &textureAsset1, &textureAsset2, &vertexShaderType, pixelShaderTypes, &pixelShaderCount,
			&isCollider, &facing);

		VoxelShapeScaleType scaleType = VoxelShapeScaleType::ScaledFromMin;
		if (voxelType == ArenaTypes::VoxelType::Raised)
		{
			scaleType = VoxelShapeScaleType::UnscaledFromMin;
		}

		outShapeDef->initBoxFromClassic(voxelType, scaleType, shapeInitCache);

		const TextureAsset *textureAssetPtrs[] = { &textureAsset0, &textureAsset1, &textureAsset2 };
		for (const TextureAsset *textureAsset : textureAssetPtrs)
		{
			if (!textureAsset->filename.empty())
			{
				outTextureDef->addTextureAsset(TextureAsset(*textureAsset));
			}
		}

		outShadingDef->init(vertexShaderType);
		for (int i = 0; i < pixelShaderCount; i++)
		{
			outShadingDef->addPixelShaderType(pixelShaderTypes[i]);
		}

		switch (voxelType)
		{
		case ArenaTypes::VoxelType::TransparentWall:
			outTraitsDef->initTransparentWall(isCollider);
			break;
		case ArenaTypes::VoxelType::Edge:
			outTraitsDef->initEdge(facing, isCollider);
			break;
		default:
			outTraitsDef->initGeneral(voxelType);
			break;
		}
	}

	void writeVoxelInfoForMAP2(ArenaTypes::VoxelID map2Voxel, const INFFile &inf, ArenaTypes::VoxelType *outVoxelType,
		ArenaMeshUtils::ShapeInitCache *outShapeInitCache, TextureAsset *outTextureAsset0, TextureAsset *outTextureAsset1,
		TextureAsset *outTextureAsset2)
	{
		*outVoxelType = ArenaTypes::VoxelType::Wall;
		outShapeInitCache->initDefaultBoxValues();
		ArenaMeshUtils::WriteWallRendererGeometryBuffers(outShapeInitCache->verticesView, outShapeInitCache->normalsView, outShapeInitCache->texCoordsView);
		ArenaMeshUtils::WriteWallRendererIndexBuffers(outShapeInitCache->indices0View, outShapeInitCache->indices1View, outShapeInitCache->indices2View);

		const int textureIndex = (map2Voxel & 0x007F) - 1;
		const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(textureIndex);
		*outTextureAsset0 = TextureAsset(
			ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
			ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
		*outTextureAsset1 = *outTextureAsset0;
		*outTextureAsset2 = *outTextureAsset0;
	}

	void writeDefsForMAP2(ArenaTypes::VoxelID map2Voxel, const INFFile &inf, VoxelShapeDefinition *outShapeDef,
		VoxelTextureDefinition *outTextureDef, VoxelShadingDefinition *outShadingDef, VoxelTraitsDefinition *outTraitsDef)
	{
		ArenaTypes::VoxelType voxelType;
		ArenaMeshUtils::ShapeInitCache shapeInitCache;
		TextureAsset textureAsset0, textureAsset1, textureAsset2;
		MapGeneration::writeVoxelInfoForMAP2(map2Voxel, inf, &voxelType, &shapeInitCache, &textureAsset0, &textureAsset1, &textureAsset2);

		constexpr VoxelShapeScaleType scaleType = VoxelShapeScaleType::ScaledFromMin;
		outShapeDef->initBoxFromClassic(voxelType, scaleType, shapeInitCache);
		outTextureDef->addTextureAsset(std::move(textureAsset0));
		outTextureDef->addTextureAsset(std::move(textureAsset1));
		outTextureDef->addTextureAsset(std::move(textureAsset2));
		
		outShadingDef->init(VertexShaderType::Basic);
		for (int i = 0; i < 3; i++)
		{
			outShadingDef->addPixelShaderType(PixelShaderType::Opaque);
		}

		outTraitsDef->initGeneral(voxelType);
	}

	void writeVoxelInfoForCeiling(const INFFile &inf, ArenaTypes::VoxelType *outVoxelType,
		ArenaMeshUtils::ShapeInitCache *outShapeInitCache, TextureAsset *outTextureAsset)
	{
		*outVoxelType = ArenaTypes::VoxelType::Ceiling;
		outShapeInitCache->initDefaultBoxValues();
		ArenaMeshUtils::WriteCeilingRendererGeometryBuffers(outShapeInitCache->verticesView, outShapeInitCache->normalsView, outShapeInitCache->texCoordsView);
		ArenaMeshUtils::WriteCeilingRendererIndexBuffers(outShapeInitCache->indices0View);

		// @todo: get ceiling from .INFs without *CEILING (like START.INF). Maybe hardcoding index 1 is enough?
		const INFCeiling &ceiling = inf.getCeiling();
		const int textureIndex = ceiling.textureIndex.value_or(1);

		const int clampedTextureID = ArenaVoxelUtils::clampVoxelTextureID(textureIndex);
		*outTextureAsset = TextureAsset(
			ArenaVoxelUtils::getVoxelTextureFilename(clampedTextureID, inf),
			ArenaVoxelUtils::getVoxelTextureSetIndex(clampedTextureID, inf));
	}

	void writeDefsForCeiling(const INFFile &inf, VoxelShapeDefinition *outShapeDef, VoxelTextureDefinition *outTextureDef, VoxelShadingDefinition *outShadingDef,
		VoxelTraitsDefinition *outTraitsDef)
	{
		ArenaTypes::VoxelType voxelType;
		ArenaMeshUtils::ShapeInitCache shapeInitCache;
		TextureAsset textureAsset;
		MapGeneration::writeVoxelInfoForCeiling(inf, &voxelType, &shapeInitCache, &textureAsset);

		constexpr VoxelShapeScaleType scaleType = VoxelShapeScaleType::ScaledFromMin;
		outShapeDef->initBoxFromClassic(voxelType, scaleType, shapeInitCache);
		outTextureDef->addTextureAsset(std::move(textureAsset));
		outShadingDef->init(VertexShaderType::Basic, PixelShaderType::Opaque);
		outTraitsDef->initGeneral(voxelType);
	}

	LockDefinition makeLockDefFromArenaLock(const ArenaTypes::MIFLock &lock)
	{
		const OriginalInt2 lockPos(lock.x, lock.y);
		const WorldInt2 newLockPos = VoxelUtils::originalVoxelToWorldVoxel(lockPos);
		LockDefinition lockDef;
		lockDef.init(newLockPos.x, 1, newLockPos.y, lock.lockLevel);
		return lockDef;
	}

	VoxelTriggerDefinition makeTriggerDefFromArenaTrigger(const ArenaTypes::MIFTrigger &trigger, const INFFile &inf)
	{
		const OriginalInt2 triggerPos(trigger.x, trigger.y);
		const WorldInt2 newTriggerPos = VoxelUtils::originalVoxelToWorldVoxel(triggerPos);

		VoxelTriggerDefinition triggerDef;
		triggerDef.init(newTriggerPos.x, 1, newTriggerPos.y);

		// There can be a *TEXT trigger and @SOUND trigger in the same voxel.
		const bool isValidTextTrigger = trigger.textIndex != -1;
		const bool isValidSoundTrigger = trigger.soundIndex != -1;

		if (isValidTextTrigger)
		{
			if (inf.hasLoreTextIndex(trigger.textIndex))
			{
				const INFLoreText &loreTextData = inf.getLoreText(trigger.textIndex);
				triggerDef.loreText.init(loreTextData.text, loreTextData.isDisplayedOnce);
			}
			else if (inf.hasRiddleIndex(trigger.textIndex))
			{
				const INFRiddle &riddleData = inf.getRiddle(trigger.textIndex);
				DebugLogWarningFormat("Riddles not implemented, trigger (%d, %d).", newTriggerPos.x, newTriggerPos.y);
				// @todo
			}
			else if (inf.hasKeyIndex(trigger.textIndex))
			{
				const INFKey &keyData = inf.getKey(trigger.textIndex);
				triggerDef.key.init(keyData.revisedID);
			}
		}

		if (isValidSoundTrigger)
		{
			const char *soundName = inf.getSound(trigger.soundIndex);
			triggerDef.sound.init(String::toUppercase(soundName));
		}

		return triggerDef;
	}

	std::optional<MapGeneration::TransitionDefGenInfo> tryMakeVoxelTransitionDefGenInfo(
		ArenaTypes::VoxelID map1Voxel, uint8_t mostSigNibble, MapType mapType, const INFFile &inf)
	{
		const uint8_t mostSigByte = ArenaLevelUtils::getVoxelMostSigByte(map1Voxel);
		const uint8_t leastSigByte = ArenaLevelUtils::getVoxelLeastSigByte(map1Voxel);
		const bool isWall = mostSigByte == leastSigByte;
		const bool isEdge = mostSigNibble == 0xA;
		if (!isWall && !isEdge)
		{
			return std::nullopt;
		}

		const int textureIndex = isEdge ? ((leastSigByte & 0x3F) - 1) : (mostSigByte - 1);
		const std::optional<int> menuIndex = inf.getMenuIndex(textureIndex);

		if (mapType == MapType::Interior)
		{
			const std::optional<int> &levelUpIndex = inf.getLevelUpIndex();
			const std::optional<int> &levelDownIndex = inf.getLevelDownIndex();
			const bool matchesLevelUp = levelUpIndex.has_value() && (*levelUpIndex == textureIndex);
			const bool matchesLevelDown = levelDownIndex.has_value() && (*levelDownIndex == textureIndex);
			const bool isMenu = menuIndex.has_value();
			const bool isValid = matchesLevelUp || matchesLevelDown || isMenu;

			if (isValid)
			{
				const bool isInteriorLevelChange = matchesLevelUp || matchesLevelDown;
				const TransitionType transitionType = isInteriorLevelChange ? TransitionType::InteriorLevelChange : TransitionType::ExitInterior;

				constexpr std::optional<ArenaTypes::InteriorType> interiorType; // Can't have interiors in interiors.
				const std::optional<bool> isInteriorLevelUp = [matchesLevelUp, isInteriorLevelChange]() -> std::optional<bool>
				{
					if (isInteriorLevelChange)
					{
						return matchesLevelUp;
					}
					else
					{
						return std::nullopt;
					}
				}();

				MapGeneration::TransitionDefGenInfo transitionDefGenInfo;
				transitionDefGenInfo.init(transitionType, interiorType, menuIndex, isInteriorLevelUp);
				return transitionDefGenInfo;
			}
			else
			{
				return std::nullopt;
			}
		}
		else if ((mapType == MapType::City) || (mapType == MapType::Wilderness))
		{
			const bool isValid = menuIndex.has_value();

			if (isValid)
			{
				// Either city gates or an interior entrance.
				const bool isCityGate = ArenaVoxelUtils::isCityGateMenuIndex(*menuIndex, mapType);

				// Can't guarantee that an Arena *MENU block that isn't a city gate is a valid transition?
				// I thought there were some wild dungeon voxels that resulted in bad values or something.
				const std::optional<ArenaTypes::InteriorType> interiorType =
					MapGeneration::tryGetInteriorTypeFromMenuIndex(*menuIndex, mapType);

				// This is optional because of the interior type issue above.
				const std::optional<TransitionType> transitionType = [isCityGate, &interiorType]()
					-> std::optional<TransitionType>
				{
					if (isCityGate)
					{
						return TransitionType::CityGate;
					}
					else if (interiorType.has_value())
					{
						return TransitionType::EnterInterior;
					}
					else
					{
						return std::nullopt;
					}
				}();

				if (transitionType.has_value())
				{
					constexpr std::optional<bool> isLevelUp; // No level changes outside of interiors.

					MapGeneration::TransitionDefGenInfo transitionDefGenInfo;
					transitionDefGenInfo.init(*transitionType, interiorType, menuIndex, isLevelUp);
					return transitionDefGenInfo;
				}
				else
				{
					return std::nullopt;
				}
			}
			else
			{
				return std::nullopt;
			}
		}
		else
		{
			DebugUnhandledReturnMsg(std::optional<MapGeneration::TransitionDefGenInfo>,
				std::to_string(static_cast<int>(mapType)));
		}
	}

	// Returns transition gen info if the MAP1 flat index is a transition entity for the given world type.
	std::optional<MapGeneration::TransitionDefGenInfo> tryMakeEntityTransitionGenInfo(
		ArenaTypes::FlatIndex flatIndex, MapType mapType)
	{
		// Only wild dens are entities with transition data.
		if (mapType != MapType::Wilderness)
		{
			return std::nullopt;
		}

		const bool isWildDen = flatIndex == ArenaWildUtils::WILD_DEN_FLAT_INDEX;
		if (!isWildDen)
		{
			return std::nullopt;
		}

		MapGeneration::TransitionDefGenInfo transitionDefGenInfo;
		transitionDefGenInfo.init(TransitionType::EnterInterior, ArenaTypes::InteriorType::Dungeon,
			std::nullopt, std::nullopt);
		return transitionDefGenInfo;
	}

	TransitionDefinition makeTransitionDef(const MapGeneration::TransitionDefGenInfo &transitionDefGenInfo,
		const WorldInt3 &position, const std::optional<int> &menuID, const std::optional<uint32_t> &rulerSeed,
		const std::optional<bool> &rulerIsMale, const std::optional<bool> &palaceIsMainQuestDungeon,
		const std::optional<ArenaTypes::CityType> &cityType, const LocationDungeonDefinition *dungeonDef,
		const std::optional<bool> &isArtifactDungeon, MapType mapType, const ExeData &exeData)
	{
		TransitionDefinition transitionDef;

		if (transitionDefGenInfo.transitionType == TransitionType::CityGate)
		{
			transitionDef.initCityGate();
		}
		else if (transitionDefGenInfo.transitionType == TransitionType::EnterInterior)
		{
			DebugAssert(transitionDefGenInfo.interiorType.has_value());
			const ArenaTypes::InteriorType interiorType = *transitionDefGenInfo.interiorType;
			MapGeneration::InteriorGenInfo interiorGenInfo = [&position, menuID, rulerSeed, &rulerIsMale,
				palaceIsMainQuestDungeon, cityType, dungeonDef, &isArtifactDungeon, mapType,
				&exeData, interiorType]()
			{
				if (ArenaInteriorUtils::isPrefabInterior(interiorType))
				{
					DebugAssert(menuID.has_value());
					DebugAssert(rulerSeed.has_value());
					DebugAssert(palaceIsMainQuestDungeon.has_value());
					DebugAssert(cityType.has_value());
					return MapGeneration::makePrefabInteriorGenInfo(interiorType, position, *menuID,
						*rulerSeed, rulerIsMale, *palaceIsMainQuestDungeon, *cityType, mapType, exeData);
				}
				else if (ArenaInteriorUtils::isProceduralInterior(interiorType))
				{
					DebugAssert(dungeonDef != nullptr);
					DebugAssert(isArtifactDungeon.has_value());
					return MapGeneration::makeProceduralInteriorGenInfo(*dungeonDef, *isArtifactDungeon);
				}
				else
				{
					DebugUnhandledReturnMsg(MapGeneration::InteriorGenInfo,
						std::to_string(static_cast<int>(interiorType)));
				}
			}();

			transitionDef.initInteriorEntrance(std::move(interiorGenInfo));
		}
		else if (transitionDefGenInfo.transitionType == TransitionType::ExitInterior)
		{
			transitionDef.initInteriorExit();
		}
		else if (transitionDefGenInfo.transitionType == TransitionType::InteriorLevelChange)
		{
			DebugAssert(transitionDefGenInfo.isLevelUp.has_value());
			transitionDef.initInteriorLevelChange(*transitionDefGenInfo.isLevelUp);
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(
				static_cast<int>(transitionDefGenInfo.transitionType)));
		}

		return transitionDef;
	}

	std::optional<MapGeneration::DoorDefGenInfo> tryMakeDoorDefGenInfo(ArenaTypes::VoxelID map1Voxel,
		uint8_t mostSigNibble)
	{
		if (((map1Voxel & 0x8000) == 0) || (mostSigNibble != 0xB))
		{
			return std::nullopt;
		}

		const ArenaTypes::DoorType doorType = [map1Voxel]()
		{
			const int type = (map1Voxel & 0x00C0) >> 4;
			if (type == 0x0)
			{
				return ArenaTypes::DoorType::Swinging;
			}
			else if (type == 0x4)
			{
				return ArenaTypes::DoorType::Sliding;
			}
			else if (type == 0x8)
			{
				return ArenaTypes::DoorType::Raising;
			}
			else
			{
				// Arena doesn't seem to have splitting doors, but they are supported.
				DebugLogWarning("Unrecognized door type \"" + std::to_string(type) + "\", treating as splitting.");
				return ArenaTypes::DoorType::Splitting;
			}
		}();

		const std::optional<int> openSoundIndex = ArenaVoxelUtils::tryGetOpenSoundIndex(doorType);
		if (!openSoundIndex.has_value())
		{
			DebugLogWarning("Couldn't get open sound index for door type \"" + std::to_string(static_cast<int>(doorType)) + "\".");
			return std::nullopt;
		}

		const std::optional<int> closeSoundIndex = ArenaVoxelUtils::tryGetCloseSoundIndex(doorType);
		if (!closeSoundIndex.has_value())
		{
			DebugLogWarning("Couldn't get close sound index for door type \"" + std::to_string(static_cast<int>(doorType)) + "\".");
			return std::nullopt;
		}

		const std::optional<VoxelDoorCloseType> closeType = [doorType]() -> std::optional<VoxelDoorCloseType>
		{
			if (ArenaVoxelUtils::doorHasSoundOnClosed(doorType))
			{
				return VoxelDoorCloseType::OnClosed;
			}
			else if (ArenaVoxelUtils::doorHasSoundOnClosing(doorType))
			{
				return VoxelDoorCloseType::OnClosing;
			}
			else
			{
				return std::nullopt;
			}
		}();

		if (!closeType.has_value())
		{
			DebugLogWarning("Can't determine close sound type for door type \"" + std::to_string(static_cast<int>(doorType)) + "\".");
			return std::nullopt;
		}

		MapGeneration::DoorDefGenInfo doorDefGenInfo;
		doorDefGenInfo.init(doorType, *openSoundIndex, *closeSoundIndex, *closeType);
		return doorDefGenInfo;
	}

	VoxelDoorDefinition makeDoorDef(const MapGeneration::DoorDefGenInfo &doorDefGenInfo, const INFFile &inf)
	{
		std::string openSoundFilename = inf.getSound(doorDefGenInfo.openSoundIndex);
		std::string closeSoundFilename = inf.getSound(doorDefGenInfo.closeSoundIndex);

		VoxelDoorDefinition doorDef;
		doorDef.init(doorDefGenInfo.doorType, openSoundFilename, doorDefGenInfo.closeType, closeSoundFilename);
		return doorDef;
	}

	// Converts .MIF/.RMD FLOR voxels to modern voxel + entity format.
	void readArenaFLOR(BufferView2D<const ArenaTypes::VoxelID> flor, MapType mapType,
		const std::optional<ArenaTypes::InteriorType> &interiorType, const std::optional<bool> &rulerIsMale,
		const INFFile &inf, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, LevelDefinition *outLevelDef, LevelInfoDefinition *outLevelInfoDef,
		ArenaVoxelMappingCache *voxelCache, ArenaEntityMappingCache *entityCache,
		ArenaChasmMappingCache *chasmCache)
	{
		for (SNInt florZ = 0; florZ < flor.getHeight(); florZ++)
		{
			for (WEInt florX = 0; florX < flor.getWidth(); florX++)
			{
				const ArenaTypes::VoxelID florVoxel = flor.get(florX, florZ);

				// Get voxel def IDs from cache or create a new entry.
				LevelVoxelShapeDefID voxelShapeDefID;
				LevelVoxelTextureDefID voxelTextureDefID;
				LevelVoxelShadingDefID voxelShadingDefID;
				LevelVoxelTraitsDefID voxelTraitsDefID;
				const auto defIter = voxelCache->find(florVoxel);
				if (defIter != voxelCache->end())
				{
					const ArenaVoxelMappingEntry &entry = defIter->second;
					voxelShapeDefID = entry.shapeDefID;
					voxelTextureDefID = entry.textureDefID;
					voxelShadingDefID = entry.shadingDefID;
					voxelTraitsDefID = entry.traitsDefID;
				}
				else
				{
					VoxelShapeDefinition voxelShapeDef;
					VoxelTextureDefinition voxelTextureDef;
					VoxelShadingDefinition voxelShadingDef;
					VoxelTraitsDefinition voxelTraitsDef;
					MapGeneration::writeDefsForFLOR(florVoxel, mapType, inf, &voxelShapeDef, &voxelTextureDef, &voxelShadingDef, &voxelTraitsDef);
					voxelShapeDefID = outLevelInfoDef->addVoxelShapeDef(std::move(voxelShapeDef));
					voxelTextureDefID = outLevelInfoDef->addVoxelTextureDef(std::move(voxelTextureDef));
					voxelShadingDefID = outLevelInfoDef->addVoxelShadingDef(std::move(voxelShadingDef));
					voxelTraitsDefID = outLevelInfoDef->addVoxelTraitsDef(std::move(voxelTraitsDef));

					ArenaVoxelMappingEntry newEntry;
					newEntry.init(voxelShapeDefID, voxelTextureDefID, voxelShadingDefID, voxelTraitsDefID);
					voxelCache->emplace(florVoxel, newEntry);
				}

				const SNInt levelX = florZ;
				const int levelY = 0;
				const WEInt levelZ = florX;
				outLevelDef->setVoxelShapeID(levelX, levelY, levelZ, voxelShapeDefID);
				outLevelDef->setVoxelTextureID(levelX, levelY, levelZ, voxelTextureDefID);
				outLevelDef->setVoxelShadingID(levelX, levelY, levelZ, voxelShadingDefID);
				outLevelDef->setVoxelTraitsID(levelX, levelY, levelZ, voxelTraitsDefID);

				// Floor voxels can also contain data for raised platform flats.
				const int floorFlatID = florVoxel & 0x00FF;
				if (floorFlatID > 0)
				{
					// Get entity def ID from cache or create a new one.
					LevelVoxelEntityDefID entityDefID;
					const auto iter = entityCache->find(florVoxel);
					if (iter != entityCache->end())
					{
						entityDefID = iter->second;
					}
					else
					{
						const ArenaTypes::FlatIndex flatIndex = floorFlatID - 1;
						EntityDefinition entityDef;
						if (!MapGeneration::tryMakeEntityDefFromArenaFlat(flatIndex, mapType,
							interiorType, rulerIsMale, inf, charClassLibrary, entityDefLibrary,
							binaryAssetLibrary, textureManager, &entityDef))
						{
							DebugLogWarning("Couldn't make entity definition from FLAT \"" +
								std::to_string(flatIndex) + "\" with .INF \"" + inf.getName() + "\".");
							continue;
						}

						entityDefID = outLevelInfoDef->addEntityDef(std::move(entityDef));
						entityCache->emplace(florVoxel, entityDefID);
					}

					const WorldDouble2 entityPos(
						static_cast<SNDouble>(levelX) + 0.50,
						static_cast<WEDouble>(levelZ) + 0.50);
					outLevelDef->addEntity(entityDefID, entityPos);
				}

				// Add chasm definition if any.
				// @todo: the traits def look-up should be replaced by just getting an isChasm from the florVoxel decoding function,
				// because all users of chasm look-ups in the engine should go through VoxelChasmDefinition, not VoxelTraitsDefinition.
				const VoxelTraitsDefinition &traitsDef = outLevelInfoDef->getVoxelTraitsDef(voxelTraitsDefID);
				if (traitsDef.type == ArenaTypes::VoxelType::Chasm)
				{
					const VoxelTraitsChasmDefinition &chasm = traitsDef.chasm;

					LevelVoxelChasmDefID chasmDefID;
					const auto chasmIter = chasmCache->find(florVoxel);
					if (chasmIter != chasmCache->end())
					{
						chasmDefID = chasmIter->second;
					}
					else
					{
						const VoxelTextureDefinition &voxelTextureDef = outLevelInfoDef->getVoxelTextureDef(voxelTextureDefID);
						const TextureAsset &chasmWallTextureAsset = voxelTextureDef.getTextureAsset(0);

						VoxelChasmDefinition chasmDef;
						chasmDef.initClassic(chasm.type, chasmWallTextureAsset, textureManager);

						chasmDefID = outLevelInfoDef->addChasmDef(std::move(chasmDef));
						chasmCache->emplace(florVoxel, chasmDefID);
					}

					outLevelDef->addChasm(chasmDefID, VoxelInt3(levelX, levelY, levelZ));
				}
			}
		}

		// Add floor replacement defs. They are not necessarily associated with an existing XYZ coordinate, and might
		// not even be in the original data.
		VoxelShapeDefinition floorReplacementShapeDef;
		VoxelTextureDefinition floorReplacementTextureDef;
		VoxelShadingDefinition floorReplacementShadingDef;
		VoxelTraitsDefinition floorReplacementTraitsDef;
		VoxelChasmDefinition floorReplacementChasmDef;
		MapGeneration::writeDefsForFloorReplacement(inf, textureManager, &floorReplacementShapeDef, &floorReplacementTextureDef, &floorReplacementShadingDef,
			&floorReplacementTraitsDef, &floorReplacementChasmDef);

		const LevelVoxelShapeDefID floorReplacementVoxelShapeDefID = outLevelInfoDef->addVoxelShapeDef(std::move(floorReplacementShapeDef));
		const LevelVoxelTextureDefID floorReplacementVoxelTextureDefID = outLevelInfoDef->addVoxelTextureDef(std::move(floorReplacementTextureDef));
		const LevelVoxelShadingDefID floorReplacementVoxelShadingDefID = outLevelInfoDef->addVoxelShadingDef(std::move(floorReplacementShadingDef));
		const LevelVoxelTraitsDefID floorReplacementVoxelTraitsDefID = outLevelInfoDef->addVoxelTraitsDef(std::move(floorReplacementTraitsDef));
		const LevelVoxelChasmDefID floorReplacementChasmDefID = outLevelInfoDef->addChasmDef(std::move(floorReplacementChasmDef));
		outLevelDef->setFloorReplacementShapeDefID(floorReplacementVoxelShapeDefID);
		outLevelDef->setFloorReplacementTextureDefID(floorReplacementVoxelTextureDefID);
		outLevelDef->setFloorReplacementShadingDefID(floorReplacementVoxelShadingDefID);
		outLevelDef->setFloorReplacementTraitsDefID(floorReplacementVoxelTraitsDefID);
		outLevelDef->setFloorReplacementChasmDefID(floorReplacementChasmDefID);
	}

	// Converts .MIF/.RMD MAP1 voxels to modern voxel + entity format.
	void readArenaMAP1(BufferView2D<const ArenaTypes::VoxelID> map1, MapType mapType,
		const std::optional<ArenaTypes::InteriorType> &interiorType, const std::optional<uint32_t> &rulerSeed,
		const std::optional<bool> &rulerIsMale, const std::optional<bool> &palaceIsMainQuestDungeon,
		const std::optional<ArenaTypes::CityType> &cityType,
		const LocationDungeonDefinition *dungeonDef, const std::optional<bool> &isArtifactDungeon,
		const INFFile &inf, const CharacterClassLibrary &charClassLibrary,
		const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, LevelDefinition *outLevelDef, LevelInfoDefinition *outLevelInfoDef,
		ArenaVoxelMappingCache *voxelCache, ArenaEntityMappingCache *entityCache,
		ArenaTransitionMappingCache *transitionCache, ArenaDoorMappingCache *doorCache)
	{
		for (SNInt map1Z = 0; map1Z < map1.getHeight(); map1Z++)
		{
			for (WEInt map1X = 0; map1X < map1.getWidth(); map1X++)
			{
				const ArenaTypes::VoxelID map1Voxel = map1.get(map1X, map1Z);

				// Skip air voxels.
				if (map1Voxel == 0)
				{
					continue;
				}

				const SNInt levelX = map1Z;
				constexpr int levelY = 1;
				const WEInt levelZ = map1X;

				// Determine if this MAP1 voxel is for a voxel or entity.
				const uint8_t mostSigNibble = (map1Voxel & 0xF000) >> 12;
				const bool isVoxel = mostSigNibble != 0x8;

				if (isVoxel)
				{
					// Get voxel def IDs from cache or create a new entry.
					LevelVoxelShapeDefID voxelShapeDefID;
					LevelVoxelTextureDefID voxelTextureDefID;
					LevelVoxelShadingDefID voxelShadingDefID;
					LevelVoxelTraitsDefID voxelTraitsDefID;
					const auto defIter = voxelCache->find(map1Voxel);
					if (defIter != voxelCache->end())
					{
						const ArenaVoxelMappingEntry &entry = defIter->second;
						voxelShapeDefID = entry.shapeDefID;
						voxelTextureDefID = entry.textureDefID;
						voxelShadingDefID = entry.shadingDefID;
						voxelTraitsDefID = entry.traitsDefID;
					}
					else
					{
						const ExeData &exeData = binaryAssetLibrary.getExeData();

						VoxelShapeDefinition voxelShapeDef;
						VoxelTextureDefinition voxelTextureDef;
						VoxelShadingDefinition voxelShadingDef;
						VoxelTraitsDefinition voxelTraitsDef;
						MapGeneration::writeDefsForMAP1(map1Voxel, mostSigNibble, mapType, inf, exeData, &voxelShapeDef, &voxelTextureDef, &voxelShadingDef, &voxelTraitsDef);
						voxelShapeDefID = outLevelInfoDef->addVoxelShapeDef(std::move(voxelShapeDef));
						voxelTextureDefID = outLevelInfoDef->addVoxelTextureDef(std::move(voxelTextureDef));
						voxelShadingDefID = outLevelInfoDef->addVoxelShadingDef(std::move(voxelShadingDef));
						voxelTraitsDefID = outLevelInfoDef->addVoxelTraitsDef(std::move(voxelTraitsDef));

						ArenaVoxelMappingEntry newEntry;
						newEntry.init(voxelShapeDefID, voxelTextureDefID, voxelShadingDefID, voxelTraitsDefID);
						voxelCache->emplace(map1Voxel, newEntry);
					}

					outLevelDef->setVoxelShapeID(levelX, levelY, levelZ, voxelShapeDefID);
					outLevelDef->setVoxelTextureID(levelX, levelY, levelZ, voxelTextureDefID);
					outLevelDef->setVoxelShadingID(levelX, levelY, levelZ, voxelShadingDefID);
					outLevelDef->setVoxelTraitsID(levelX, levelY, levelZ, voxelTraitsDefID);

					const WorldInt3 levelPosition(levelX, levelY, levelZ);
					const WorldInt2 levelPositionXZ(levelX, levelZ);

					// Try to make transition info if this MAP1 voxel is a transition.
					const std::optional<MapGeneration::TransitionDefGenInfo> transitionDefGenInfo =
						MapGeneration::tryMakeVoxelTransitionDefGenInfo(map1Voxel, mostSigNibble, mapType, inf);

					if (transitionDefGenInfo.has_value())
					{
						// Get transition def ID from cache or create a new one. These rely on XY coordinates
						// for interior .MIF name generation so can't really reuse them.
						LevelVoxelTransitionDefID transitionDefID;
						const auto iter = std::find_if(transitionCache->begin(), transitionCache->end(),
							[map1Voxel, levelPositionXZ](const std::pair<TransitionMappingKey, LevelVoxelTransitionDefID> &pair)
						{
							const TransitionMappingKey &key = pair.first;
							return (key.voxelID == map1Voxel) && (key.levelXZ == levelPositionXZ);
						});

						if (iter != transitionCache->end())
						{
							transitionDefID = iter->second;
						}
						else
						{
							TransitionDefinition transitionDef = MapGeneration::makeTransitionDef(
								*transitionDefGenInfo, levelPosition, transitionDefGenInfo->menuID, rulerSeed,
								rulerIsMale, palaceIsMainQuestDungeon, cityType, dungeonDef, isArtifactDungeon,
								mapType, binaryAssetLibrary.getExeData());
							transitionDefID = outLevelInfoDef->addTransitionDef(std::move(transitionDef));

							TransitionMappingKey transitionMappingKey;
							transitionMappingKey.voxelID = map1Voxel;
							transitionMappingKey.levelXZ = levelPositionXZ;
							transitionCache->emplace_back(transitionMappingKey, transitionDefID);
						}

						outLevelDef->addTransition(transitionDefID, levelPosition);
					}

					// Try to make door info if this MAP1 voxel is a door.
					const std::optional<MapGeneration::DoorDefGenInfo> doorDefGenInfo =
						MapGeneration::tryMakeDoorDefGenInfo(map1Voxel, mostSigNibble);

					if (doorDefGenInfo.has_value())
					{
						// Get door def ID from cache or create a new one.
						LevelVoxelDoorDefID doorDefID;
						const auto iter = doorCache->find(map1Voxel);
						if (iter != doorCache->end())
						{
							doorDefID = iter->second;
						}
						else
						{
							VoxelDoorDefinition doorDef = MapGeneration::makeDoorDef(*doorDefGenInfo, inf);
							doorDefID = outLevelInfoDef->addDoorDef(std::move(doorDef));
							doorCache->emplace(map1Voxel, doorDefID);
						}

						outLevelDef->addDoor(doorDefID, levelPosition);
					}
				}
				else
				{
					// Get entity def ID from cache or create a new one.
					LevelVoxelEntityDefID entityDefID;
					const auto iter = entityCache->find(map1Voxel);
					if (iter != entityCache->end())
					{
						entityDefID = iter->second;
					}
					else
					{
						const ArenaTypes::FlatIndex flatIndex = map1Voxel & 0x00FF;
						EntityDefinition entityDef;
						if (!MapGeneration::tryMakeEntityDefFromArenaFlat(flatIndex, mapType,
							interiorType, rulerIsMale, inf, charClassLibrary, entityDefLibrary,
							binaryAssetLibrary, textureManager, &entityDef))
						{
							DebugLogWarning("Couldn't make entity definition from FLAT \"" +
								std::to_string(flatIndex) + "\" with .INF \"" + inf.getName() + "\".");
							continue;
						}

						entityDefID = outLevelInfoDef->addEntityDef(std::move(entityDef));
						entityCache->emplace(map1Voxel, entityDefID);
					}

					const WorldDouble2 entityPos(
						static_cast<SNDouble>(levelX) + 0.50,
						static_cast<WEDouble>(levelZ) + 0.50);
					outLevelDef->addEntity(entityDefID, entityPos);
				}
			}
		}
	}

	// Converts .MIF/.RMD MAP2 voxels to modern voxel + entity format.
	void readArenaMAP2(BufferView2D<const ArenaTypes::VoxelID> map2, const INFFile &inf,
		LevelDefinition *outLevelDef, LevelInfoDefinition *outLevelInfoDef, ArenaVoxelMappingCache *voxelCache)
	{
		for (SNInt map2Z = 0; map2Z < map2.getHeight(); map2Z++)
		{
			for (WEInt map2X = 0; map2X < map2.getWidth(); map2X++)
			{
				const ArenaTypes::VoxelID map2Voxel = map2.get(map2X, map2Z);

				// Skip air voxels.
				if (map2Voxel == 0)
				{
					continue;
				}

				// Get voxel def ID from cache or create a new one.
				LevelVoxelShapeDefID voxelShapeDefID;
				LevelVoxelTextureDefID voxelTextureDefID;
				LevelVoxelShadingDefID voxelShadingDefID;
				LevelVoxelTraitsDefID voxelTraitsDefID;
				const auto defIter = voxelCache->find(map2Voxel);
				if (defIter != voxelCache->end())
				{
					const ArenaVoxelMappingEntry &entry = defIter->second;
					voxelShapeDefID = entry.shapeDefID;
					voxelTextureDefID = entry.textureDefID;
					voxelShadingDefID = entry.shadingDefID;
					voxelTraitsDefID = entry.traitsDefID;
				}
				else
				{
					VoxelShapeDefinition voxelShapeDef;
					VoxelTextureDefinition voxelTextureDef;
					VoxelShadingDefinition voxelShadingDef;
					VoxelTraitsDefinition voxelTraitsDef;
					MapGeneration::writeDefsForMAP2(map2Voxel, inf, &voxelShapeDef, &voxelTextureDef, &voxelShadingDef, &voxelTraitsDef);
					voxelShapeDefID = outLevelInfoDef->addVoxelShapeDef(std::move(voxelShapeDef));
					voxelTextureDefID = outLevelInfoDef->addVoxelTextureDef(std::move(voxelTextureDef));
					voxelShadingDefID = outLevelInfoDef->addVoxelShadingDef(std::move(voxelShadingDef));
					voxelTraitsDefID = outLevelInfoDef->addVoxelTraitsDef(std::move(voxelTraitsDef));

					ArenaVoxelMappingEntry newEntry;
					newEntry.init(voxelShapeDefID, voxelTextureDefID, voxelShadingDefID, voxelTraitsDefID);
					voxelCache->emplace(map2Voxel, newEntry);
				}

				// Duplicate voxels upward based on calculated height.
				const int yStart = 2;
				const int yEnd = yStart + ArenaLevelUtils::getMap2VoxelHeight(map2Voxel);
				for (int y = yStart; y < yEnd; y++)
				{
					const SNInt levelX = map2Z;
					const WEInt levelZ = map2X;
					outLevelDef->setVoxelShapeID(levelX, y, levelZ, voxelShapeDefID);
					outLevelDef->setVoxelTextureID(levelX, y, levelZ, voxelTextureDefID);
					outLevelDef->setVoxelShadingID(levelX, y, levelZ, voxelShadingDefID);
					outLevelDef->setVoxelTraitsID(levelX, y, levelZ, voxelTraitsDefID);
				}
			}
		}
	}

	// Fills the equivalent MAP2 layer with duplicates of the ceiling block for a .MIF level
	// without MAP2 data.
	void readArenaCeiling(const INFFile &inf, LevelDefinition *outLevelDef, LevelInfoDefinition *outLevelInfoDef)
	{
		VoxelShapeDefinition voxelShapeDef;
		VoxelTextureDefinition voxelTextureDef;
		VoxelShadingDefinition voxelShadingDef;
		VoxelTraitsDefinition voxelTraitsDef;
		MapGeneration::writeDefsForCeiling(inf, &voxelShapeDef, &voxelTextureDef, &voxelShadingDef, &voxelTraitsDef);
		const LevelVoxelShapeDefID voxelShapeDefID = outLevelInfoDef->addVoxelShapeDef(std::move(voxelShapeDef));
		const LevelVoxelTextureDefID voxelTextureDefID = outLevelInfoDef->addVoxelTextureDef(std::move(voxelTextureDef));
		const LevelVoxelShadingDefID voxelShadingDefID = outLevelInfoDef->addVoxelShadingDef(std::move(voxelShadingDef));
		const LevelVoxelTraitsDefID voxelTraitsDefID = outLevelInfoDef->addVoxelTraitsDef(std::move(voxelTraitsDef));

		for (SNInt levelX = 0; levelX < outLevelDef->getWidth(); levelX++)
		{
			for (WEInt levelZ = 0; levelZ < outLevelDef->getDepth(); levelZ++)
			{
				constexpr int y = 2;
				outLevelDef->setVoxelShapeID(levelX, y, levelZ, voxelShapeDefID);
				outLevelDef->setVoxelTextureID(levelX, y, levelZ, voxelTextureDefID);
				outLevelDef->setVoxelShadingID(levelX, y, levelZ, voxelShadingDefID);
				outLevelDef->setVoxelTraitsID(levelX, y, levelZ, voxelTraitsDefID);
			}
		}
	}

	void readArenaLock(const ArenaTypes::MIFLock &lock, const INFFile &inf, LevelDefinition *outLevelDef,
		LevelInfoDefinition *outLevelInfoDef, ArenaLockMappingCache *lockMappings)
	{
		// Get lock def ID from cache or create a new one.
		LevelVoxelLockDefID lockDefID;
		const auto iter = std::find_if(lockMappings->begin(), lockMappings->end(),
			[&lock](const std::pair<ArenaTypes::MIFLock, LevelVoxelLockDefID> &pair)
		{
			const ArenaTypes::MIFLock &mifLock = pair.first;
			return (mifLock.x == lock.x) && (mifLock.y == lock.y) && (mifLock.lockLevel == lock.lockLevel);
		});

		if (iter != lockMappings->end())
		{
			lockDefID = iter->second;
		}
		else
		{
			LockDefinition lockDef = MapGeneration::makeLockDefFromArenaLock(lock);
			lockDefID = outLevelInfoDef->addLockDef(std::move(lockDef));
			lockMappings->emplace_back(std::make_pair(lock, lockDefID));
		}

		const LockDefinition &lockDef = outLevelInfoDef->getLockDef(lockDefID);
		const SNInt x = lockDef.x;
		const int y = lockDef.y;
		const WEInt z = lockDef.z;
		outLevelDef->addLock(lockDefID, WorldInt3(x, y, z));
	}

	void readArenaTrigger(const ArenaTypes::MIFTrigger &trigger, const INFFile &inf,
		LevelDefinition *outLevelDef, LevelInfoDefinition *outLevelInfoDef,
		ArenaTriggerMappingCache *triggerMappings)
	{
		// See if the trigger has already been added. Prevent duplicate triggers since they exist in at least
		// one of the original game's main quest dungeons.
		const auto iter = std::find_if(triggerMappings->begin(), triggerMappings->end(),
			[&trigger](const std::pair<ArenaTypes::MIFTrigger, LevelVoxelTriggerDefID> &pair)
		{
			const ArenaTypes::MIFTrigger &mifTrigger = pair.first;
			return (mifTrigger.x == trigger.x) && (mifTrigger.y == trigger.y) &&
				(mifTrigger.textIndex == trigger.textIndex) && (mifTrigger.soundIndex == trigger.soundIndex);
		});

		if (iter == triggerMappings->end())
		{
			const LevelVoxelTriggerDefID triggerDefID = outLevelInfoDef->addTriggerDef(
				MapGeneration::makeTriggerDefFromArenaTrigger(trigger, inf));
			triggerMappings->emplace_back(std::make_pair(trigger, triggerDefID));

			const VoxelTriggerDefinition &triggerDef = outLevelInfoDef->getTriggerDef(triggerDefID);
			const SNInt x = triggerDef.x;
			const int y = triggerDef.y;
			const WEInt z = triggerDef.z;
			outLevelDef->addTrigger(triggerDefID, WorldInt3(x, y, z));
		}
	}

	void generateArenaDungeonLevel(const MIFFile &mif, WEInt widthChunks, SNInt depthChunks,
		int levelUpBlock, const std::optional<int> &levelDownBlock, ArenaRandom &random,
		MapType mapType, ArenaTypes::InteriorType interiorType, const std::optional<bool> &rulerIsMale,
		const std::optional<bool> &isArtifactDungeon, const INFFile &inf,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager,
		LevelDefinition *outLevelDef, LevelInfoDefinition *outLevelInfoDef,
		ArenaVoxelMappingCache *florMappings, ArenaVoxelMappingCache *map1Mappings,
		ArenaEntityMappingCache *entityMappings, ArenaLockMappingCache *lockMappings,
		ArenaTriggerMappingCache *triggerMappings, ArenaTransitionMappingCache *transitionMappings,
		ArenaDoorMappingCache *doorMappings, ArenaChasmMappingCache *chasmMappings)
	{
		// Create buffers for level blocks.
		Buffer2D<ArenaTypes::VoxelID> levelFLOR(mif.getWidth() * widthChunks, mif.getDepth() * depthChunks);
		Buffer2D<ArenaTypes::VoxelID> levelMAP1(levelFLOR.getWidth(), levelFLOR.getHeight());
		levelFLOR.fill(0);
		levelMAP1.fill(0);

		const int tileSet = random.next() % 4;

		for (SNInt row = 0; row < depthChunks; row++)
		{
			const SNInt zOffset = row * ArenaInteriorUtils::DUNGEON_CHUNK_DIM;
			for (WEInt column = 0; column < widthChunks; column++)
			{
				const WEInt xOffset = column * ArenaInteriorUtils::DUNGEON_CHUNK_DIM;

				// Get the selected level from the random chunks .MIF file.
				const int blockIndex = (tileSet * 8) + (random.next() % 8);
				const auto &blockLevel = mif.getLevel(blockIndex);
				const BufferView2D<const ArenaTypes::VoxelID> blockFLOR = blockLevel.getFLOR();
				const BufferView2D<const ArenaTypes::VoxelID> blockMAP1 = blockLevel.getMAP1();

				// Copy block data to temp buffers.
				for (SNInt z = 0; z < ArenaInteriorUtils::DUNGEON_CHUNK_DIM; z++)
				{
					for (WEInt x = 0; x < ArenaInteriorUtils::DUNGEON_CHUNK_DIM; x++)
					{
						const ArenaTypes::VoxelID srcFlorVoxel = blockFLOR.get(x, z);
						const ArenaTypes::VoxelID srcMap1Voxel = blockMAP1.get(x, z);
						const WEInt dstX = xOffset + x;
						const SNInt dstZ = zOffset + z;
						levelFLOR.set(dstX, dstZ, srcFlorVoxel);
						levelMAP1.set(dstX, dstZ, srcMap1Voxel);
					}
				}

				// Assign locks to the current block.
				for (const ArenaTypes::MIFLock &lock : blockLevel.getLOCK())
				{
					ArenaTypes::MIFLock tempLock;
					tempLock.x = xOffset + lock.x;
					tempLock.y = zOffset + lock.y;
					tempLock.lockLevel = lock.lockLevel;

					MapGeneration::readArenaLock(tempLock, inf, outLevelDef, outLevelInfoDef, lockMappings);
				}

				// Assign text/sound triggers to the current block.
				for (const ArenaTypes::MIFTrigger &trigger : blockLevel.getTRIG())
				{
					ArenaTypes::MIFTrigger tempTrigger;
					tempTrigger.x = xOffset + trigger.x;
					tempTrigger.y = zOffset + trigger.y;
					tempTrigger.textIndex = trigger.textIndex;
					tempTrigger.soundIndex = trigger.soundIndex;

					MapGeneration::readArenaTrigger(tempTrigger, inf, outLevelDef, outLevelInfoDef, triggerMappings);
				}
			}
		}

		// Draw perimeter blocks. First top and bottom, then right and left.
		constexpr ArenaTypes::VoxelID perimeterVoxel = 0x7800;
		for (WEInt x = 0; x < levelMAP1.getWidth(); x++)
		{
			levelMAP1.set(x, 0, perimeterVoxel);
			levelMAP1.set(x, levelMAP1.getHeight() - 1, perimeterVoxel);
		}

		for (SNInt z = 1; z < (levelMAP1.getHeight() - 1); z++)
		{
			levelMAP1.set(0, z, perimeterVoxel);
			levelMAP1.set(levelMAP1.getWidth() - 1, z, perimeterVoxel);
		}

		// Put transition block(s).
		const uint8_t levelUpVoxelByte = *inf.getLevelUpIndex() + 1;
		WEInt levelUpX;
		SNInt levelUpZ;
		ArenaInteriorUtils::unpackLevelChangeVoxel(levelUpBlock, &levelUpX, &levelUpZ);
		levelMAP1.set(ArenaInteriorUtils::offsetLevelChangeVoxel(levelUpX),
			ArenaInteriorUtils::offsetLevelChangeVoxel(levelUpZ),
			ArenaInteriorUtils::convertLevelChangeVoxel(levelUpVoxelByte));

		if (levelDownBlock.has_value())
		{
			const uint8_t levelDownVoxelByte = *inf.getLevelDownIndex() + 1;
			WEInt levelDownX;
			SNInt levelDownZ;
			ArenaInteriorUtils::unpackLevelChangeVoxel(*levelDownBlock, &levelDownX, &levelDownZ);
			levelMAP1.set(ArenaInteriorUtils::offsetLevelChangeVoxel(levelDownX),
				ArenaInteriorUtils::offsetLevelChangeVoxel(levelDownZ),
				ArenaInteriorUtils::convertLevelChangeVoxel(levelDownVoxelByte));
		}

		// Convert temp voxel buffers to the modern format.
		const BufferView2D<const ArenaTypes::VoxelID> levelFlorView(levelFLOR);
		const BufferView2D<const ArenaTypes::VoxelID> levelMap1View(levelMAP1);
		MapGeneration::readArenaFLOR(levelFlorView, mapType, interiorType, rulerIsMale, inf,
			charClassLibrary, entityDefLibrary, binaryAssetLibrary, textureManager, outLevelDef,
			outLevelInfoDef, florMappings, entityMappings, chasmMappings);

		constexpr std::optional<uint32_t> rulerSeed; // Not necessary for dungeons.
		constexpr std::optional<bool> palaceIsMainQuestDungeon; // Not necessary for dungeons.
		constexpr std::optional<ArenaTypes::CityType> cityType; // Not necessary for dungeons.
		constexpr LocationDungeonDefinition *dungeonDef = nullptr; // Not necessary for dungeons.

		MapGeneration::readArenaMAP1(levelMap1View, mapType, interiorType, rulerSeed, rulerIsMale,
			palaceIsMainQuestDungeon, cityType, dungeonDef, isArtifactDungeon, inf, charClassLibrary,
			entityDefLibrary, binaryAssetLibrary, textureManager, outLevelDef, outLevelInfoDef,
			map1Mappings, entityMappings, transitionMappings, doorMappings);

		// Generate ceiling (if any).
		if (!inf.getCeiling().outdoorDungeon)
		{
			MapGeneration::readArenaCeiling(inf, outLevelDef, outLevelInfoDef);
		}
	}

	void generateArenaCityBuildingNames(uint32_t citySeed, int raceID, bool coastal,
		const std::string_view cityTypeName,
		const LocationCityDefinition::MainQuestTempleOverride *mainQuestTempleOverride,
		ArenaRandom &random, const BinaryAssetLibrary &binaryAssetLibrary,
		const TextAssetLibrary &textAssetLibrary, LevelDefinition *outLevelDef,
		LevelInfoDefinition *outLevelInfoDef)
	{
		const auto &exeData = binaryAssetLibrary.getExeData();
		const Int2 localCityPoint = ArenaLocationUtils::getLocalCityPoint(citySeed);

		auto tryGetInteriorType = [outLevelDef, outLevelInfoDef](SNInt x, WEInt z)
			-> std::optional<ArenaTypes::InteriorType>
		{
			auto tryGetTransitionDefID = [outLevelDef](SNInt x, WEInt z)
				-> std::optional<LevelVoxelTransitionDefID>
			{
				// Find the associated transition for this voxel (if any).
				const WorldInt3 voxel(x, 1, z);
				for (int i = 0; i < outLevelDef->getTransitionPlacementDefCount(); i++)
				{
					const auto &placementDef = outLevelDef->getTransitionPlacementDef(i);
					for (const WorldInt3 position : placementDef.positions)
					{
						if (position == voxel)
						{
							return placementDef.id;
						}
					}
				}

				return std::nullopt;
			};

			const std::optional<LevelVoxelTransitionDefID> transitionDefID = tryGetTransitionDefID(x, z);
			if (!transitionDefID.has_value())
			{
				// No transition at this voxel.
				return std::nullopt;
			}

			const TransitionDefinition &transitionDef = outLevelInfoDef->getTransitionDef(*transitionDefID);
			const TransitionType transitionType = transitionDef.type;
			if (transitionType != TransitionType::EnterInterior)
			{
				// Not a transition to an interior.
				return std::nullopt;
			}

			const InteriorEntranceTransitionDefinition &interiorEntranceDef = transitionDef.interiorEntrance;
			const InteriorGenInfo &interiorGenInfo = interiorEntranceDef.interiorGenInfo;
			return interiorGenInfo.interiorType;
		};

		// Lambda for looping through main-floor voxels and generating names for *MENU blocks that
		// match the given menu type.
		auto generateNames = [&citySeed, raceID, coastal, &cityTypeName, mainQuestTempleOverride,
			&random, &textAssetLibrary, outLevelDef, outLevelInfoDef, &exeData, &localCityPoint,
			&tryGetInteriorType](ArenaTypes::InteriorType interiorType)
		{
			if ((interiorType == ArenaTypes::InteriorType::Equipment) ||
				(interiorType == ArenaTypes::InteriorType::Temple))
			{
				citySeed = (localCityPoint.x << 16) + localCityPoint.y;
				random.srand(citySeed);
			}

			std::vector<int> seen;
			auto hashInSeen = [&seen](int hash)
			{
				return std::find(seen.begin(), seen.end(), hash) != seen.end();
			};

			// Lambdas for creating tavern, equipment store, and temple building names.
			auto createTavernName = [coastal, &exeData](int prefixIndex, int suffixIndex)
			{
				const auto &tavernPrefixes = exeData.cityGen.tavernPrefixes;
				const auto &tavernSuffixes = coastal ?
					exeData.cityGen.tavernMarineSuffixes : exeData.cityGen.tavernSuffixes;
				DebugAssertIndex(tavernPrefixes, prefixIndex);
				DebugAssertIndex(tavernSuffixes, suffixIndex);
				return tavernPrefixes[prefixIndex] + ' ' + tavernSuffixes[suffixIndex];
			};

			auto createEquipmentName = [raceID, &cityTypeName, &random, &textAssetLibrary, &exeData](
				int prefixIndex, int suffixIndex, SNInt x, WEInt z)
			{
				const auto &equipmentPrefixes = exeData.cityGen.equipmentPrefixes;
				const auto &equipmentSuffixes = exeData.cityGen.equipmentSuffixes;

				// Equipment store names can have variables in them.
				DebugAssertIndex(equipmentPrefixes, prefixIndex);
				DebugAssertIndex(equipmentSuffixes, suffixIndex);
				std::string str = equipmentPrefixes[prefixIndex] + ' ' + equipmentSuffixes[suffixIndex];

				// Replace %ct with city type name.
				size_t index = str.find("%ct");
				if (index != std::string::npos)
				{
					str.replace(index, 3, cityTypeName);
				}

				// Replace %ef with generated male first name from (y<<16)+x seed. Use a local RNG for
				// modifications to building names. Swap and reverse the XZ dimensions so they fit the
				// original XY values in Arena.
				index = str.find("%ef");
				if (index != std::string::npos)
				{
					ArenaRandom nameRandom((x << 16) + z);
					const std::string maleFirstName = [raceID, &textAssetLibrary, &nameRandom]()
					{
						constexpr bool isMale = true;
						const std::string name = textAssetLibrary.generateNpcName(raceID, isMale, nameRandom);
						const std::string firstName = String::split(name)[0];
						return firstName;
					}();

					str.replace(index, 3, maleFirstName);
				}

				// Replace %n with generated male name from (x<<16)+y seed.
				index = str.find("%n");
				if (index != std::string::npos)
				{
					ArenaRandom nameRandom((z << 16) + x);
					constexpr bool isMale = true;
					const std::string maleName = textAssetLibrary.generateNpcName(raceID, isMale, nameRandom);
					str.replace(index, 2, maleName);
				}

				return str;
			};

			auto createTempleName = [&exeData](int model, int suffixIndex)
			{
				const auto &templePrefixes = exeData.cityGen.templePrefixes;
				const auto &temple1Suffixes = exeData.cityGen.temple1Suffixes;
				const auto &temple2Suffixes = exeData.cityGen.temple2Suffixes;
				const auto &temple3Suffixes = exeData.cityGen.temple3Suffixes;

				const std::string &templeSuffix = [&temple1Suffixes, &temple2Suffixes, &temple3Suffixes,
					model, suffixIndex]() -> const std::string&
				{
					if (model == 0)
					{
						DebugAssertIndex(temple1Suffixes, suffixIndex);
						return temple1Suffixes[suffixIndex];
					}
					else if (model == 1)
					{
						DebugAssertIndex(temple2Suffixes, suffixIndex);
						return temple2Suffixes[suffixIndex];
					}
					else
					{
						DebugAssertIndex(temple3Suffixes, suffixIndex);
						return temple3Suffixes[suffixIndex];
					}
				}();

				DebugAssertIndex(templePrefixes, model);
				return templePrefixes[model] + templeSuffix;
			};

			// The lambda called for each main-floor voxel in the area.
			auto tryGenerateBlockName = [interiorType, &random, outLevelDef, outLevelInfoDef, &tryGetInteriorType,
				&seen, &hashInSeen, &createTavernName, &createEquipmentName, &createTempleName](SNInt x, WEInt z)
			{
				// See if the current voxel is a *MENU block and matches the target menu type.
				const bool matchesTargetType = [x, z, interiorType, outLevelDef, outLevelInfoDef,
					&tryGetInteriorType]()
				{
					const std::optional<ArenaTypes::InteriorType> curInteriorType = tryGetInteriorType(x, z);
					return curInteriorType.has_value() && (*curInteriorType == interiorType);
				}();

				if (matchesTargetType)
				{
					// Get the *MENU block's display name.
					int hash;
					std::string name;

					if (interiorType == ArenaTypes::InteriorType::Tavern)
					{
						// Tavern.
						int prefixIndex, suffixIndex;
						do
						{
							prefixIndex = random.next() % 23;
							suffixIndex = random.next() % 23;
							hash = (prefixIndex << 8) + suffixIndex;
						} while (hashInSeen(hash));

						name = createTavernName(prefixIndex, suffixIndex);
					}
					else if (interiorType == ArenaTypes::InteriorType::Equipment)
					{
						// Equipment store.
						int prefixIndex, suffixIndex;
						do
						{
							prefixIndex = random.next() % 20;
							suffixIndex = random.next() % 10;
							hash = (prefixIndex << 8) + suffixIndex;
						} while (hashInSeen(hash));

						name = createEquipmentName(prefixIndex, suffixIndex, x, z);
					}
					else
					{
						// Temple.
						int model, suffixIndex;
						do
						{
							model = random.next() % 3;
							constexpr std::array<int, 3> ModelVars = { 5, 9, 10 };
							const int vars = ModelVars.at(model);
							suffixIndex = random.next() % vars;
							hash = (model << 8) + suffixIndex;
						} while (hashInSeen(hash));

						name = createTempleName(model, suffixIndex);
					}

					const LevelVoxelBuildingNameID buildingNameID =
						outLevelInfoDef->addBuildingName(std::move(name));
					outLevelDef->addBuildingName(buildingNameID, WorldInt3(x, 1, z));
					seen.emplace_back(hash);
				}
			};

			// Start at the top-right corner of the map, running right to left and top to bottom.
			for (SNInt x = 0; x < outLevelDef->getWidth(); x++)
			{
				for (WEInt z = 0; z < outLevelDef->getDepth(); z++)
				{
					tryGenerateBlockName(x, z);
				}
			}

			// Fix some edge cases with main quest cities.
			if ((interiorType == ArenaTypes::InteriorType::Temple) && (mainQuestTempleOverride != nullptr))
			{
				const int modelIndex = mainQuestTempleOverride->modelIndex;
				const int suffixIndex = mainQuestTempleOverride->suffixIndex;

				// Added an index variable in this solution since the original game seems to store
				// its building names in a way other than with a vector.
				const LevelVoxelBuildingNameID buildingNameID =
					mainQuestTempleOverride->menuNamesIndex;

				std::string buildingName = createTempleName(modelIndex, suffixIndex);
				outLevelInfoDef->setBuildingNameOverride(buildingNameID, std::move(buildingName));
			}
		};

		generateNames(ArenaTypes::InteriorType::Tavern);
		generateNames(ArenaTypes::InteriorType::Equipment);
		generateNames(ArenaTypes::InteriorType::Temple);

		// Add names for any mage's guild entrances.
		std::optional<LevelVoxelBuildingNameID> magesGuildBuildingNameID;
		for (WEInt z = 0; z < outLevelDef->getDepth(); z++)
		{
			for (SNInt x = 0; x < outLevelDef->getWidth(); x++)
			{
				const std::optional<ArenaTypes::InteriorType> interiorType = tryGetInteriorType(x, z);
				if (interiorType.has_value() && (*interiorType == ArenaTypes::InteriorType::MagesGuild))
				{
					if (!magesGuildBuildingNameID.has_value())
					{
						const std::string &magesGuildBuildingName = exeData.cityGen.magesGuildMenuName;
						magesGuildBuildingNameID = outLevelInfoDef->addBuildingName(std::string(magesGuildBuildingName));
					}

					const WorldInt3 position(x, 1, z);
					outLevelDef->addBuildingName(*magesGuildBuildingNameID, position);
				}
			}
		}
	}

	// Using a separate building name info struct because the same level definition might be
	// used in multiple places in the wild, so it can't store the building name IDs.
	void generateArenaWildChunkBuildingNames(uint32_t wildChunkSeed, const LevelDefinition &levelDef,
		const BinaryAssetLibrary &binaryAssetLibrary, MapGeneration::WildChunkBuildingNameInfo *outBuildingNameInfo,
		LevelInfoDefinition *outLevelInfoDef, ArenaBuildingNameMappingCache *buildingNameMappings)
	{
		const auto &exeData = binaryAssetLibrary.getExeData();

		// Lambda for searching for an interior entrance voxel of the given type in the chunk
		// and generating a name for it if found.
		auto tryGenerateBuildingNameForChunk = [wildChunkSeed, &levelDef, outBuildingNameInfo,
			outLevelInfoDef, buildingNameMappings, &exeData](ArenaTypes::InteriorType interiorType)
		{
			auto createTavernName = [&exeData](int prefixIndex, int suffixIndex)
			{
				const auto &tavernPrefixes = exeData.cityGen.tavernPrefixes;
				const auto &tavernSuffixes = exeData.cityGen.tavernSuffixes;
				DebugAssertIndex(tavernPrefixes, prefixIndex);
				DebugAssertIndex(tavernSuffixes, suffixIndex);
				return tavernPrefixes[prefixIndex] + ' ' + tavernSuffixes[suffixIndex];
			};

			auto createTempleName = [&exeData](int model, int suffixIndex)
			{
				const auto &templePrefixes = exeData.cityGen.templePrefixes;
				const auto &temple1Suffixes = exeData.cityGen.temple1Suffixes;
				const auto &temple2Suffixes = exeData.cityGen.temple2Suffixes;
				const auto &temple3Suffixes = exeData.cityGen.temple3Suffixes;

				const std::string &templeSuffix = [&temple1Suffixes, &temple2Suffixes,
					&temple3Suffixes, model, suffixIndex]() -> const std::string&
				{
					if (model == 0)
					{
						DebugAssertIndex(temple1Suffixes, suffixIndex);
						return temple1Suffixes[suffixIndex];
					}
					else if (model == 1)
					{
						DebugAssertIndex(temple2Suffixes, suffixIndex);
						return temple2Suffixes[suffixIndex];
					}
					else
					{
						DebugAssertIndex(temple3Suffixes, suffixIndex);
						return temple3Suffixes[suffixIndex];
					}
				}();

				DebugAssertIndex(templePrefixes, model);
				return templePrefixes[model] + templeSuffix;
			};

			// Iterate transition voxels in the chunk and stop once a relevant voxel for generating
			// the name has been found.
			for (int i = 0; i < levelDef.getTransitionPlacementDefCount(); i++)
			{
				const LevelTransitionPlacementDefinition &placementDef = levelDef.getTransitionPlacementDef(i);
				const TransitionDefinition &transitionDef = outLevelInfoDef->getTransitionDef(placementDef.id);
				const TransitionType transitionType = transitionDef.type;
				if (transitionType != TransitionType::EnterInterior)
				{
					// Not a transition to an interior.
					continue;
				}

				const InteriorEntranceTransitionDefinition &interiorEntranceDef = transitionDef.interiorEntrance;
				const InteriorGenInfo &interiorGenInfo = interiorEntranceDef.interiorGenInfo;
				if (interiorGenInfo.interiorType != interiorType)
				{
					// Not the interior we're generating a name for.
					continue;
				}

				ArenaRandom random(wildChunkSeed);

				// Get the *MENU block's display name.
				std::string buildingName = [interiorType, &random, &createTavernName, &createTempleName]()
				{
					if (interiorType == ArenaTypes::InteriorType::Tavern)
					{
						const int prefixIndex = random.next() % 23;
						const int suffixIndex = random.next() % 23;
						return createTavernName(prefixIndex, suffixIndex);
					}
					else if (interiorType == ArenaTypes::InteriorType::Temple)
					{
						const int model = random.next() % 3;
						constexpr std::array<int, 3> ModelVars = { 5, 9, 10 };
						DebugAssertIndex(ModelVars, model);
						const int vars = ModelVars[model];
						const int suffixIndex = random.next() % vars;
						return createTempleName(model, suffixIndex);
					}
					else
					{
						DebugUnhandledReturnMsg(std::string, std::to_string(static_cast<int>(interiorType)));
					}
				}();

				// Set building name info for the given menu type.
				const auto iter = buildingNameMappings->find(buildingName);
				if (iter != buildingNameMappings->end())
				{
					outBuildingNameInfo->setBuildingNameID(interiorType, iter->second);
				}
				else
				{
					const LevelVoxelBuildingNameID buildingNameID = outLevelInfoDef->addBuildingName(std::string(buildingName));
					outBuildingNameInfo->setBuildingNameID(interiorType, buildingNameID);
					buildingNameMappings->emplace(std::move(buildingName), buildingNameID);
				}

				break;
			}
		};

		tryGenerateBuildingNameForChunk(ArenaTypes::InteriorType::Tavern);
		tryGenerateBuildingNameForChunk(ArenaTypes::InteriorType::Temple);
	}
}

void MapGeneration::InteriorPrefabGenInfo::init(const std::string &mifName, ArenaTypes::InteriorType interiorType, const std::optional<bool> &rulerIsMale)
{
	this->mifName = mifName;
	this->interiorType = interiorType;
	this->rulerIsMale = rulerIsMale;
}

void MapGeneration::InteriorDungeonGenInfo::init(const LocationDungeonDefinition &dungeonDef, bool isArtifactDungeon)
{
	this->dungeonDef = dungeonDef;
	this->isArtifactDungeon = isArtifactDungeon;
}

MapGeneration::InteriorGenInfo::InteriorGenInfo()
{
	this->type = static_cast<InteriorGenType>(-1);
}

void MapGeneration::InteriorGenInfo::initPrefab(const std::string &mifName, ArenaTypes::InteriorType interiorType, const std::optional<bool> &rulerIsMale)
{
	this->type = InteriorGenType::Prefab;
	this->interiorType = interiorType;
	this->prefab.init(mifName, interiorType, rulerIsMale);
}

void MapGeneration::InteriorGenInfo::initDungeon(const LocationDungeonDefinition &dungeonDef, bool isArtifactDungeon)
{
	this->type = InteriorGenType::Dungeon;
	this->interiorType = ArenaTypes::InteriorType::Dungeon;
	this->dungeon.init(dungeonDef, isArtifactDungeon);
}

void MapGeneration::CityGenInfo::init(std::string &&mifName, std::string &&cityTypeName, ArenaTypes::CityType cityType,
	uint32_t citySeed, uint32_t rulerSeed, int raceID, bool isPremade, bool coastal, bool rulerIsMale,
	bool palaceIsMainQuestDungeon, Buffer<uint8_t> &&reservedBlocks,
	const std::optional<LocationCityDefinition::MainQuestTempleOverride> &mainQuestTempleOverride,
	WEInt blockStartPosX, SNInt blockStartPosY, int cityBlocksPerSide)
{
	this->mifName = std::move(mifName);
	this->cityTypeName = std::move(cityTypeName);
	this->cityType = cityType;
	this->citySeed = citySeed;
	this->rulerSeed = rulerSeed;
	this->raceID = raceID;
	this->isPremade = isPremade;
	this->coastal = coastal;
	this->rulerIsMale = rulerIsMale;
	this->palaceIsMainQuestDungeon = palaceIsMainQuestDungeon;
	this->reservedBlocks = std::move(reservedBlocks);
	this->mainQuestTempleOverride = mainQuestTempleOverride;
	this->blockStartPosX = blockStartPosX;
	this->blockStartPosY = blockStartPosY;
	this->cityBlocksPerSide = cityBlocksPerSide;
}

void MapGeneration::WildGenInfo::init(Buffer2D<ArenaWildUtils::WildBlockID> &&wildBlockIDs,
	const LocationCityDefinition &cityDef, uint32_t fallbackSeed)
{
	this->wildBlockIDs = std::move(wildBlockIDs);
	this->cityDef = &cityDef;
	this->fallbackSeed = fallbackSeed;
}

void MapGeneration::WildChunkBuildingNameInfo::init(const ChunkInt2 &chunk)
{
	this->chunk = chunk;
}

const ChunkInt2 &MapGeneration::WildChunkBuildingNameInfo::getChunk() const
{
	return this->chunk;
}

bool MapGeneration::WildChunkBuildingNameInfo::hasBuildingNames() const
{
	return this->ids.size() > 0;
}

bool MapGeneration::WildChunkBuildingNameInfo::tryGetBuildingNameID(
	ArenaTypes::InteriorType interiorType, LevelVoxelBuildingNameID *outID) const
{
	const auto iter = this->ids.find(interiorType);
	if (iter != this->ids.end())
	{
		*outID = iter->second;
		return true;
	}
	else
	{
		return false;
	}
}

void MapGeneration::WildChunkBuildingNameInfo::setBuildingNameID(
	ArenaTypes::InteriorType interiorType, LevelVoxelBuildingNameID id)
{
	const auto iter = this->ids.find(interiorType);
	if (iter != this->ids.end())
	{
		iter->second = id;
	}
	else
	{
		this->ids.emplace(interiorType, id);
	}
}

void MapGeneration::TransitionDefGenInfo::init(TransitionType transitionType,
	const std::optional<ArenaTypes::InteriorType> &interiorType, const std::optional<int> &menuID,
	const std::optional<bool> &isLevelUp)
{
	this->transitionType = transitionType;
	this->interiorType = interiorType;
	this->menuID = menuID;
	this->isLevelUp = isLevelUp;
}

void MapGeneration::DoorDefGenInfo::init(ArenaTypes::DoorType doorType, int openSoundIndex, int closeSoundIndex, VoxelDoorCloseType closeType)
{
	this->doorType = doorType;
	this->openSoundIndex = openSoundIndex;
	this->closeSoundIndex = closeSoundIndex;
	this->closeType = closeType;
}

void MapGeneration::readMifVoxels(BufferView<const MIFLevel> levels, MapType mapType,
	const std::optional<ArenaTypes::InteriorType> &interiorType, const std::optional<uint32_t> &rulerSeed,
	const std::optional<bool> &rulerIsMale, const std::optional<bool> &palaceIsMainQuestDungeon,
	const std::optional<ArenaTypes::CityType> &cityType, const LocationDungeonDefinition *dungeonDef,
	const std::optional<bool> &isArtifactDungeon, const INFFile &inf, const CharacterClassLibrary &charClassLibrary,
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, BufferView<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef)
{
	// Each .MIF level voxel is unpacked into either a voxel or entity. These caches point to
	// previously-added definitions in the level info def.
	ArenaVoxelMappingCache florMappings, map1Mappings, map2Mappings;
	ArenaEntityMappingCache entityMappings;
	ArenaTransitionMappingCache transitionMappings;
	ArenaDoorMappingCache doorMappings;
	ArenaChasmMappingCache chasmMappings;

	for (int i = 0; i < levels.getCount(); i++)
	{
		const MIFLevel &level = levels.get(i);
		LevelDefinition &levelDef = outLevelDefs.get(i);
		MapGeneration::readArenaFLOR(level.getFLOR(), mapType, interiorType, rulerIsMale, inf,
			charClassLibrary, entityDefLibrary, binaryAssetLibrary, textureManager, &levelDef,
			outLevelInfoDef, &florMappings, &entityMappings, &chasmMappings);
		MapGeneration::readArenaMAP1(level.getMAP1(), mapType, interiorType, rulerSeed, rulerIsMale,
			palaceIsMainQuestDungeon, cityType, dungeonDef, isArtifactDungeon, inf, charClassLibrary,
			entityDefLibrary, binaryAssetLibrary, textureManager, &levelDef, outLevelInfoDef, &map1Mappings,
			&entityMappings, &transitionMappings, &doorMappings);

		// If there is MAP2 data, use it for the ceiling layer, otherwise replicate a single ceiling
		// block across the whole ceiling if not in an outdoor dungeon.
		if (level.getMAP2().isValid())
		{
			MapGeneration::readArenaMAP2(level.getMAP2(), inf, &levelDef, outLevelInfoDef, &map2Mappings);
		}
		else if (!inf.getCeiling().outdoorDungeon)
		{
			MapGeneration::readArenaCeiling(inf, &levelDef, outLevelInfoDef);
		}
	}
}

void MapGeneration::generateMifDungeon(const MIFFile &mif, int levelCount, WEInt widthChunks,
	SNInt depthChunks, const INFFile &inf, ArenaRandom &random, MapType mapType,
	ArenaTypes::InteriorType interiorType, const std::optional<bool> &rulerIsMale,
	const std::optional<bool> &isArtifactDungeon, const CharacterClassLibrary &charClassLibrary,
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, BufferView<LevelDefinition> &outLevelDefs,
	LevelInfoDefinition *outLevelInfoDef, WorldInt2 *outStartPoint)
{
	ArenaVoxelMappingCache florMappings, map1Mappings;
	ArenaEntityMappingCache entityMappings;
	ArenaLockMappingCache lockMappings;
	ArenaTriggerMappingCache triggerMappings;
	ArenaTransitionMappingCache transitionMappings;
	ArenaDoorMappingCache doorMappings;
	ArenaChasmMappingCache chasmMappings;

	// Store the seed for later, to be used with block selection.
	const uint32_t seed2 = random.getSeed();

	// Determine transition blocks (*LEVELUP/*LEVELDOWN) that will appear in the dungeon.
	auto getNextTransBlock = [widthChunks, depthChunks, &random]()
	{
		const SNInt tY = random.next() % depthChunks;
		const WEInt tX = random.next() % widthChunks;
		return ArenaInteriorUtils::packLevelChangeVoxel(tX, tY);
	};

	// Packed coordinates for transition blocks.
	// @todo: maybe this could be an int pair so packing is not required.
	std::vector<int> transitions;

	// Handle initial case where transitions list is empty (for i == 0).
	transitions.emplace_back(getNextTransBlock());

	// Handle general case for transitions list additions.
	for (int i = 1; i < levelCount; i++)
	{
		int transBlock;
		do
		{
			transBlock = getNextTransBlock();
		} while (transBlock == transitions.back());

		transitions.emplace_back(transBlock);
	}

	// Generate each level, deciding which dungeon blocks to use.
	for (int i = 0; i < levelCount; i++)
	{
		random.srand(seed2 + i);

		// Determine level up/down blocks.
		DebugAssertIndex(transitions, i);
		const int levelUpBlock = transitions[i];
		const std::optional<int> levelDownBlock = [&transitions, levelCount, i]() -> std::optional<int>
		{
			if (i < (levelCount - 1))
			{
				const int index = DebugMakeIndex(transitions, i + 1);
				return transitions[index];
			}
			else
			{
				// No *LEVELDOWN block on the lowest level.
				return std::nullopt;
			}
		}();

		LevelDefinition &levelDef = outLevelDefs.get(i);
		MapGeneration::generateArenaDungeonLevel(mif, widthChunks, depthChunks, levelUpBlock,
			levelDownBlock, random, mapType, interiorType, rulerIsMale, isArtifactDungeon,
			inf, charClassLibrary, entityDefLibrary, binaryAssetLibrary, textureManager, &levelDef,
			outLevelInfoDef, &florMappings, &map1Mappings, &entityMappings, &lockMappings,
			&triggerMappings, &transitionMappings, &doorMappings, &chasmMappings);
	}

	// The start point depends on where the level up voxel is on the first level.
	DebugAssertIndex(transitions, 0);
	const int firstTransition = transitions[0];
	WEInt firstTransitionChunkX;
	SNInt firstTransitionChunkZ;
	ArenaInteriorUtils::unpackLevelChangeVoxel(
		firstTransition, &firstTransitionChunkX, &firstTransitionChunkZ);

	// Convert it from the old coordinate system to the new one.
	const OriginalInt2 startPoint(
		ArenaInteriorUtils::offsetLevelChangeVoxel(firstTransitionChunkX),
		ArenaInteriorUtils::offsetLevelChangeVoxel(firstTransitionChunkZ));
	*outStartPoint = VoxelUtils::originalVoxelToWorldVoxel(startPoint);
}

void MapGeneration::generateMifCity(const MIFFile &mif, uint32_t citySeed, uint32_t rulerSeed, int raceID,
	bool isPremade, bool rulerIsMale, bool palaceIsMainQuestDungeon, BufferView<const uint8_t> reservedBlocks,
	WEInt blockStartPosX, SNInt blockStartPosY, int cityBlocksPerSide, bool coastal, const std::string_view cityTypeName,
	ArenaTypes::CityType cityType, const LocationCityDefinition::MainQuestTempleOverride *mainQuestTempleOverride,
	const INFFile &inf, const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, const TextAssetLibrary &textAssetLibrary, TextureManager &textureManager,
	LevelDefinition *outLevelDef, LevelInfoDefinition *outLevelInfoDef)
{
	ArenaVoxelMappingCache florMappings, map1Mappings, map2Mappings;
	ArenaEntityMappingCache entityMappings;
	ArenaTransitionMappingCache transitionMappings;
	ArenaDoorMappingCache doorMappings;
	ArenaChasmMappingCache chasmMappings;

	// Only one level in a city .MIF.
	const MIFLevel &mifLevel = mif.getLevel(0);

	// Create temp voxel data buffers and write the city skeleton data to them.
	Buffer2D<ArenaTypes::VoxelID> tempFlor(mif.getWidth(), mif.getDepth());
	Buffer2D<ArenaTypes::VoxelID> tempMap1(mif.getWidth(), mif.getDepth());
	Buffer2D<ArenaTypes::VoxelID> tempMap2(mif.getWidth(), mif.getDepth());
	BufferView2D<ArenaTypes::VoxelID> tempFlorView(tempFlor);
	BufferView2D<ArenaTypes::VoxelID> tempMap1View(tempMap1);
	BufferView2D<ArenaTypes::VoxelID> tempMap2View(tempMap2);
	ArenaCityUtils::writeSkeleton(mifLevel, tempFlorView, tempMap1View, tempMap2View);

	// Use the city's seed for random chunk generation. It is modified later during building
	// name generation.
	ArenaRandom random(citySeed);

	if (!isPremade)
	{
		// Generate procedural city data and write it into the temp buffers.
		const OriginalInt2 blockStartPosition(blockStartPosX, blockStartPosY);
		ArenaCityUtils::generateCity(citySeed, cityBlocksPerSide, mif.getWidth(), reservedBlocks,
			blockStartPosition, random, binaryAssetLibrary, tempFlor, tempMap1, tempMap2);
	}

	// Run the palace gate graphic algorithm over the perimeter of the MAP1 data.
	ArenaCityUtils::revisePalaceGraphics(tempMap1, mif.getDepth(), mif.getWidth());

	const BufferView2D<const ArenaTypes::VoxelID> tempFlorConstView(tempFlor);
	const BufferView2D<const ArenaTypes::VoxelID> tempMap1ConstView(tempMap1);
	const BufferView2D<const ArenaTypes::VoxelID> tempMap2ConstView(tempMap2);

	constexpr MapType mapType = MapType::City;
	constexpr std::optional<ArenaTypes::InteriorType> interiorType; // City is not an interior.
	constexpr LocationDungeonDefinition *dungeonDef = nullptr; // Not necessary for city.
	constexpr std::optional<bool> isArtifactDungeon; // Not necessary for city.

	MapGeneration::readArenaFLOR(tempFlorConstView, mapType, interiorType, rulerIsMale, inf,
		charClassLibrary, entityDefLibrary, binaryAssetLibrary, textureManager, outLevelDef,
		outLevelInfoDef, &florMappings, &entityMappings, &chasmMappings);
	MapGeneration::readArenaMAP1(tempMap1ConstView, mapType, interiorType, rulerSeed, rulerIsMale,
		palaceIsMainQuestDungeon, cityType, dungeonDef, isArtifactDungeon, inf, charClassLibrary,
		entityDefLibrary, binaryAssetLibrary, textureManager, outLevelDef, outLevelInfoDef, &map1Mappings,
		&entityMappings, &transitionMappings, &doorMappings);
	MapGeneration::readArenaMAP2(tempMap2ConstView, inf, outLevelDef, outLevelInfoDef, &map2Mappings);
	MapGeneration::generateArenaCityBuildingNames(citySeed, raceID, coastal, cityTypeName,
		mainQuestTempleOverride, random, binaryAssetLibrary, textAssetLibrary, outLevelDef,
		outLevelInfoDef);
}

void MapGeneration::generateRmdWilderness(BufferView<const ArenaWildUtils::WildBlockID> uniqueWildBlockIDs,
	BufferView2D<const int> levelDefIndices, const LocationCityDefinition &cityDef,
	const INFFile &inf, const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager,
	BufferView<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef,
	std::vector<MapGeneration::WildChunkBuildingNameInfo> *outBuildingNameInfos)
{
	DebugAssert(uniqueWildBlockIDs.getCount() == outLevelDefs.getCount());

	ArenaVoxelMappingCache florMappings, map1Mappings, map2Mappings;
	ArenaEntityMappingCache entityMappings;
	ArenaTransitionMappingCache transitionMappings;
	ArenaBuildingNameMappingCache buildingNameMappings;
	ArenaDoorMappingCache doorMappings;
	ArenaChasmMappingCache chasmMappings;

	// Create temp voxel data buffers to be used by each wilderness chunk.
	constexpr int chunkDim = ChunkUtils::CHUNK_DIM;
	Buffer2D<ArenaTypes::VoxelID> tempFlor(chunkDim, chunkDim);
	Buffer2D<ArenaTypes::VoxelID> tempMap1(chunkDim, chunkDim);
	Buffer2D<ArenaTypes::VoxelID> tempMap2(chunkDim, chunkDim);

	for (int i = 0; i < uniqueWildBlockIDs.getCount(); i++)
	{
		const ArenaWildUtils::WildBlockID wildBlockID = uniqueWildBlockIDs.get(i);
		const auto &rmdFiles = ArenaLevelLibrary::getInstance().getWildernessChunks();
		const int rmdIndex = DebugMakeIndex(rmdFiles, wildBlockID - 1);
		const RMDFile &rmd = rmdFiles[rmdIndex];
		const BufferView2D<const ArenaTypes::VoxelID> rmdFLOR = rmd.getFLOR();
		const BufferView2D<const ArenaTypes::VoxelID> rmdMAP1 = rmd.getMAP1();
		const BufferView2D<const ArenaTypes::VoxelID> rmdMAP2 = rmd.getMAP2();

		// Copy .RMD voxels into temp buffers.
		for (int y = 0; y < tempFlor.getHeight(); y++)
		{
			for (int x = 0; x < tempFlor.getWidth(); x++)
			{
				const ArenaTypes::VoxelID rmdFlorID = rmdFLOR.get(x, y);
				const ArenaTypes::VoxelID rmdMap1ID = rmdMAP1.get(x, y);
				const ArenaTypes::VoxelID rmdMap2ID = rmdMAP2.get(x, y);
				tempFlor.set(x, y, rmdFlorID);
				tempMap1.set(x, y, rmdMap1ID);
				tempMap2.set(x, y, rmdMap2ID);
			}
		}

		if (ArenaWildUtils::isWildCityBlock(wildBlockID))
		{
			// Change the placeholder WILD00{1..4}.RMD block to the one for the given city.
			BufferView2D<ArenaTypes::VoxelID> tempFlorView(tempFlor);
			BufferView2D<ArenaTypes::VoxelID> tempMap1View(tempMap1);
			BufferView2D<ArenaTypes::VoxelID> tempMap2View(tempMap2);
			ArenaWildUtils::reviseWildCityBlock(wildBlockID, tempFlorView, tempMap1View, tempMap2View, cityDef, binaryAssetLibrary);
		}

		LevelDefinition &levelDef = outLevelDefs.get(i);

		const BufferView2D<const ArenaTypes::VoxelID> tempFlorConstView(tempFlor);
		const BufferView2D<const ArenaTypes::VoxelID> tempMap1ConstView(tempMap1);
		const BufferView2D<const ArenaTypes::VoxelID> tempMap2ConstView(tempMap2);

		constexpr MapType mapType = MapType::Wilderness;
		constexpr std::optional<ArenaTypes::InteriorType> interiorType; // Wilderness is not an interior.

		// Dungeon definition if this chunk has any dungeons.
		const uint32_t dungeonSeed = cityDef.provinceSeed;
		LocationDungeonDefinition dungeonDef;
		dungeonDef.init(dungeonSeed, ArenaWildUtils::WILD_DUNGEON_WIDTH_CHUNKS, ArenaWildUtils::WILD_DUNGEON_HEIGHT_CHUNKS);

		constexpr std::optional<bool> isArtifactDungeon = false; // No artifacts in wild dungeons.

		MapGeneration::readArenaFLOR(tempFlorConstView, mapType, interiorType, cityDef.rulerIsMale, inf,
			charClassLibrary, entityDefLibrary, binaryAssetLibrary, textureManager, &levelDef,
			outLevelInfoDef, &florMappings, &entityMappings, &chasmMappings);
		MapGeneration::readArenaMAP1(tempMap1ConstView, mapType, interiorType, cityDef.rulerSeed, cityDef.rulerIsMale,
			cityDef.palaceIsMainQuestDungeon, cityDef.type, &dungeonDef, isArtifactDungeon, inf, charClassLibrary,
			entityDefLibrary, binaryAssetLibrary, textureManager, &levelDef, outLevelInfoDef, &map1Mappings,
			&entityMappings, &transitionMappings, &doorMappings);
		MapGeneration::readArenaMAP2(tempMap2ConstView, inf, &levelDef, outLevelInfoDef, &map2Mappings);
	}

	// Generate chunk-wise building names for the wilderness.
	for (WEInt z = 0; z < levelDefIndices.getHeight(); z++)
	{
		for (SNInt x = 0; x < levelDefIndices.getWidth(); x++)
		{
			const int levelDefIndex = levelDefIndices.get(x, z);
			const LevelDefinition &levelDef = outLevelDefs.get(levelDefIndex);
			const ChunkInt2 chunk(x, z);
			const uint32_t chunkSeed = ArenaWildUtils::makeWildChunkSeed(chunk.y, chunk.x);
			MapGeneration::WildChunkBuildingNameInfo buildingNameInfo;
			buildingNameInfo.init(chunk);

			MapGeneration::generateArenaWildChunkBuildingNames(chunkSeed, levelDef, binaryAssetLibrary,
				&buildingNameInfo, outLevelInfoDef, &buildingNameMappings);

			// Register the chunk if it has any buildings with names.
			if (buildingNameInfo.hasBuildingNames())
			{
				outBuildingNameInfos->emplace_back(std::move(buildingNameInfo));
			}
		}
	}
}

void MapGeneration::readMifLocks(BufferView<const MIFLevel> levels, const INFFile &inf,
	BufferView<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef)
{
	ArenaLockMappingCache lockMappings;

	for (int i = 0; i < levels.getCount(); i++)
	{
		const MIFLevel &level = levels.get(i);
		LevelDefinition &levelDef = outLevelDefs.get(i);
		const BufferView<const ArenaTypes::MIFLock> locks = level.getLOCK();

		for (const ArenaTypes::MIFLock &lock : locks)
		{
			MapGeneration::readArenaLock(lock, inf, &levelDef, outLevelInfoDef, &lockMappings);
		}
	}
}

void MapGeneration::readMifTriggers(BufferView<const MIFLevel> levels, const INFFile &inf,
	BufferView<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef)
{
	ArenaTriggerMappingCache triggerMappings;

	for (int i = 0; i < levels.getCount(); i++)
	{
		const MIFLevel &level = levels.get(i);
		LevelDefinition &levelDef = outLevelDefs.get(i);
		const BufferView<const ArenaTypes::MIFTrigger> triggers = level.getTRIG();

		for (const ArenaTypes::MIFTrigger &trigger : triggers)
		{
			MapGeneration::readArenaTrigger(trigger, inf, &levelDef, outLevelInfoDef, &triggerMappings);
		}
	}
}
