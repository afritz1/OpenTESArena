#include <algorithm>
#include <map>
#include <unordered_map>

#include "InteriorLevelUtils.h"
#include "LevelDefinition.h"
#include "LevelInfoDefinition.h"
#include "LevelUtils.h"
#include "LockDefinition.h"
#include "MapGeneration.h"
#include "TriggerDefinition.h"
#include "VoxelDefinition.h"
#include "VoxelFacing.h"
#include "WorldType.h"
#include "../Assets/ArenaAnimUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/MIFUtils.h"
#include "../Entities/EntityDefinition.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Entities/EntityType.h"
#include "../Math/Random.h"

#include "components/debug/Debug.h"
#include "components/utilities/BufferView2D.h"
#include "components/utilities/String.h"

namespace std
{
	// std::map comparison for MIFLock.
	bool operator<(const ArenaTypes::MIFLock &a, const ArenaTypes::MIFLock &b)
	{
		return (a.x < b.x) || ((a.x == b.x) && (a.y < b.y)) ||
			((a.x == b.x) && (a.y == b.y) && (a.lockLevel < b.lockLevel));
	}

	// std::map comparison for MIFTrigger.
	bool operator<(const ArenaTypes::MIFTrigger &a, const ArenaTypes::MIFTrigger &b)
	{
		return (a.x < b.x) || ((a.x == b.x) && (a.y < b.y)) ||
			((a.x == b.x) && (a.y == b.y) && (a.textIndex < b.textIndex)) ||
			((a.x == b.x) && (a.y == b.y) && (a.textIndex == b.textIndex) && (a.soundIndex < b.soundIndex));
	}
}

namespace MapGeneration
{
	// Mapping caches of .MIF/.RMD voxels, etc. to modern level info entries.
	using ArenaVoxelMappingCache = std::unordered_map<ArenaTypes::VoxelID, LevelDefinition::VoxelDefID>;
	using ArenaEntityMappingCache = std::unordered_map<ArenaTypes::VoxelID, LevelDefinition::EntityDefID>;
	using ArenaLockMappingCache = std::map<ArenaTypes::MIFLock, LevelDefinition::LockDefID>;
	using ArenaTriggerMappingCache = std::map<ArenaTypes::MIFTrigger, LevelDefinition::TriggerDefID>;

	static_assert(sizeof(ArenaTypes::VoxelID) == sizeof(uint16_t));

	// Makes a modern entity definition from the given Arena FLAT index.
	// @todo: probably want this to be some 'LevelEntityDefinition' with no dependencies on runtime
	// textures and animations handles, instead using texture filenames for the bulk of things.
	bool tryMakeEntityDefFromArenaFlat(int flatIndex, WorldType worldType, bool isPalace,
		const std::optional<bool> &rulerIsMale, const INFFile &inf,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager,
		EntityDefinition *outDef)
	{
		const INFFile::FlatData &flatData = inf.getFlat(flatIndex);
		const EntityType entityType = ArenaAnimUtils::getEntityTypeFromFlat(flatIndex, inf);
		const std::optional<int> &optItemIndex = flatData.itemIndex;

		bool isFinalBoss;
		const bool isCreature = optItemIndex.has_value() &&
			ArenaAnimUtils::isCreatureIndex(*optItemIndex, &isFinalBoss);
		const bool isHumanEnemy = optItemIndex.has_value() &&
			ArenaAnimUtils::isHumanEnemyIndex(*optItemIndex);

		const bool isCity = worldType == WorldType::City;
		const ArenaAnimUtils::StaticAnimCondition staticAnimCondition = [isCity, isPalace]()
		{
			if (isPalace)
			{
				return ArenaAnimUtils::StaticAnimCondition::IsPalace;
			}
			else if (isCity)
			{
				return ArenaAnimUtils::StaticAnimCondition::IsCity;
			}
			else
			{
				return ArenaAnimUtils::StaticAnimCondition::None;
			}
		}();

		// Add entity animation data. Static entities have only idle animations (and maybe on/off
		// state for lampposts). Dynamic entities have several animation states and directions.
		//auto &entityAnimData = newEntityDef.getAnimationData();
		EntityAnimationDefinition entityAnimDef;
		EntityAnimationInstance entityAnimInst;
		if (entityType == EntityType::Static)
		{
			if (!ArenaAnimUtils::tryMakeStaticEntityAnims(flatIndex, staticAnimCondition,
				rulerIsMale, inf, textureManager, &entityAnimDef, &entityAnimInst))
			{
				DebugLogWarning("Couldn't make static entity anims for flat \"" +
					std::to_string(flatIndex) + "\".");
				return false;
			}

			// The entity can only be instantiated if there is at least an idle animation.
			int idleStateIndex;
			if (!entityAnimDef.tryGetStateIndex(EntityAnimationUtils::STATE_IDLE.c_str(), &idleStateIndex))
			{
				DebugLogWarning("Missing static entity idle anim state for flat \"" +
					std::to_string(flatIndex) + "\".");
				return false;
			}
		}
		else if (entityType == EntityType::Dynamic)
		{
			// Assume that human enemies in level data are male.
			const std::optional<bool> isMale = true;

			if (!ArenaAnimUtils::tryMakeDynamicEntityAnims(flatIndex, isMale, inf, charClassLibrary,
				binaryAssetLibrary, textureManager, &entityAnimDef, &entityAnimInst))
			{
				DebugLogWarning("Couldn't make dynamic entity anims for flat \"" +
					std::to_string(flatIndex) + "\".");
				return false;
			}

			// Must have at least an idle animation.
			int idleStateIndex;
			if (!entityAnimDef.tryGetStateIndex(EntityAnimationUtils::STATE_IDLE.c_str(), &idleStateIndex))
			{
				DebugLogWarning("Missing dynamic entity idle anim state for flat \"" +
					std::to_string(flatIndex) + "\".");
				return false;
			}
		}
		else
		{
			DebugCrash("Unrecognized entity type \"" +
				std::to_string(static_cast<int>(entityType)) + "\".");
		}

		// @todo: replace isCreature/etc. with some flatIndex -> EntityDefinition::Type function.
		// - Most likely also need location type, etc. because flatIndex is level-dependent.
		if (isCreature)
		{
			const int itemIndex = *optItemIndex;
			const int creatureID = isFinalBoss ?
				ArenaAnimUtils::getFinalBossCreatureID() :
				ArenaAnimUtils::getCreatureIDFromItemIndex(itemIndex);
			const int creatureIndex = creatureID - 1;

			// @todo: read from EntityDefinitionLibrary instead, and don't make anim def above.
			// Currently these are just going to be duplicates of defs in the library.
			EntityDefinitionLibrary::Key entityDefKey;
			entityDefKey.initCreature(creatureIndex, isFinalBoss);

			EntityDefID entityDefID;
			if (!entityDefLibrary.tryGetDefinitionID(entityDefKey, &entityDefID))
			{
				DebugLogWarning("Couldn't get creature definition " +
					std::to_string(creatureIndex) + " from library.");
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
		else // @todo: handle other entity definition types.
		{
			// Doodad.
			const bool streetLight = ArenaAnimUtils::isStreetLightFlatIndex(flatIndex, isCity);
			const double scale = ArenaAnimUtils::getDimensionModifier(flatData);
			const int lightIntensity = flatData.lightIntensity.has_value() ? *flatData.lightIntensity : 0;

			outDef->initDoodad(flatData.yOffset, scale, flatData.collider,
				flatData.transparent, flatData.ceiling, streetLight, flatData.puddle,
				lightIntensity, std::move(entityAnimDef));
		}

		return true;
	}

	VoxelDefinition makeVoxelDefFromFLOR(ArenaTypes::VoxelID florVoxel, const INFFile &inf)
	{
		const int textureID = (florVoxel & 0xFF00) >> 8;

		// Determine if the floor voxel is either solid or a chasm.
		if (!MIFUtils::isChasm(textureID))
		{
			return VoxelDefinition::makeFloor(textureID);
		}
		else
		{
			int chasmID;
			VoxelDefinition::ChasmData::Type chasmType;
			if (textureID == MIFUtils::DRY_CHASM)
			{
				const int *dryChasmIndex = inf.getDryChasmIndex();
				if (dryChasmIndex != nullptr)
				{
					chasmID = *dryChasmIndex;
				}
				else
				{
					DebugLogWarning("Missing *DRYCHASM ID.");
					chasmID = 0;
				}

				chasmType = VoxelDefinition::ChasmData::Type::Dry;
			}
			else if (textureID == MIFUtils::LAVA_CHASM)
			{
				const int *lavaChasmIndex = inf.getLavaChasmIndex();
				if (lavaChasmIndex != nullptr)
				{
					chasmID = *lavaChasmIndex;
				}
				else
				{
					DebugLogWarning("Missing *LAVACHASM ID.");
					chasmID = 0;
				}

				chasmType = VoxelDefinition::ChasmData::Type::Lava;
			}
			else if (textureID == MIFUtils::WET_CHASM)
			{
				const int *wetChasmIndex = inf.getWetChasmIndex();
				if (wetChasmIndex != nullptr)
				{
					chasmID = *wetChasmIndex;
				}
				else
				{
					DebugLogWarning("Missing *WETCHASM ID.");
					chasmID = 0;
				}

				chasmType = VoxelDefinition::ChasmData::Type::Wet;
			}
			else
			{
				DebugCrash("Unsupported chasm type \"" + std::to_string(textureID) + "\".");
			}

			return VoxelDefinition::makeChasm(chasmID, chasmType);
		}
	}

	VoxelDefinition makeVoxelDefFromMAP1(ArenaTypes::VoxelID map1Voxel, uint8_t mostSigNibble,
		WorldType worldType, const INFFile &inf, const ExeData &exeData)
	{
		DebugAssert(map1Voxel != 0);
		DebugAssert(mostSigNibble != 0x8);

		if ((map1Voxel & 0x8000) == 0)
		{
			// A voxel of some kind.
			const uint8_t mostSigByte = (map1Voxel & 0x7F00) >> 8;
			const uint8_t leastSigByte = map1Voxel & 0x007F;
			const bool voxelIsSolid = mostSigByte == leastSigByte;

			if (voxelIsSolid)
			{
				// Regular solid wall.
				const int textureIndex = mostSigByte - 1;

				// Menu index if the voxel has the *MENU tag, or -1 if it is not a *MENU voxel.
				const int menuIndex = inf.getMenuIndex(textureIndex);
				const bool isMenu = menuIndex != -1;

				// Determine what the type of the wall is (level up/down, menu, or just plain solid).
				const VoxelDefinition::WallData::Type type = [&inf, textureIndex, isMenu]()
				{
					// Returns whether the given index pointer is non-null and matches the current
					// texture index.
					auto matchesIndex = [textureIndex](const int *index)
					{
						return (index != nullptr) && (*index == textureIndex);
					};

					if (matchesIndex(inf.getLevelUpIndex()))
					{
						return VoxelDefinition::WallData::Type::LevelUp;
					}
					else if (matchesIndex(inf.getLevelDownIndex()))
					{
						return VoxelDefinition::WallData::Type::LevelDown;
					}
					else if (isMenu)
					{
						return VoxelDefinition::WallData::Type::Menu;
					}
					else
					{
						return VoxelDefinition::WallData::Type::Solid;
					}
				}();

				return VoxelDefinition::makeWall(textureIndex, textureIndex, textureIndex,
					isMenu ? &menuIndex : nullptr, type);
			}
			else
			{
				// Raised platform.
				const uint8_t wallTextureID = map1Voxel & 0x000F;
				const uint8_t capTextureID = (map1Voxel & 0x00F0) >> 4;

				const int sideID = [&inf, wallTextureID]()
				{
					const int *ptr = inf.getBoxSide(wallTextureID);
					if (ptr != nullptr)
					{
						return *ptr;
					}
					else
					{
						DebugLogWarning("Missing *BOXSIDE ID \"" + std::to_string(wallTextureID) +
							"\" for raised platform side.");
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
					const int *ptr = inf.getBoxCap(capTextureID);
					if (ptr != nullptr)
					{
						return *ptr;
					}
					else
					{
						DebugLogWarning("Missing *BOXCAP ID \"" + std::to_string(capTextureID) +
							"\" for raised platform ceiling.");
						return 0;
					}
				}();

				const auto &wallHeightTables = exeData.wallHeightTables;
				const int heightIndex = mostSigByte & 0x07;
				const int thicknessIndex = (mostSigByte & 0x78) >> 3;

				int baseOffset, baseSize;
				if (worldType == WorldType::City)
				{
					baseOffset = wallHeightTables.box1b.at(heightIndex);
					baseSize = wallHeightTables.box2b.at(thicknessIndex);
				}
				else if (worldType == WorldType::Interior)
				{
					baseOffset = wallHeightTables.box1a.at(heightIndex);

					const int boxSize = wallHeightTables.box2a.at(thicknessIndex);
					const auto &boxScale = inf.getCeiling().boxScale;
					baseSize = boxScale.has_value() ?
						((boxSize * (*boxScale)) / 256) : boxSize;
				}
				else if (worldType == WorldType::Wilderness)
				{
					baseOffset = wallHeightTables.box1c.at(heightIndex);

					constexpr int boxSize = 32;
					const auto &boxScale = inf.getCeiling().boxScale;
					baseSize = (boxSize * (boxScale.has_value() ? boxScale.value() : 192)) / 256;
				}
				else
				{
					DebugNotImplementedMsg(std::to_string(static_cast<int>(worldType)));
				}

				const double yOffset = static_cast<double>(baseOffset) / MIFUtils::ARENA_UNITS;
				const double ySize = static_cast<double>(baseSize) / MIFUtils::ARENA_UNITS;
				const double normalizedScale = static_cast<double>(inf.getCeiling().height) / MIFUtils::ARENA_UNITS;
				const double yOffsetNormalized = yOffset / normalizedScale;
				const double ySizeNormalized = ySize / normalizedScale;

				// @todo: might need some tweaking with box3/box4 values.
				const double vTop = std::max(0.0, 1.0 - yOffsetNormalized - ySizeNormalized);
				const double vBottom = std::min(vTop + ySizeNormalized, 1.0);

				return VoxelDefinition::makeRaised(sideID, floorID, ceilingID, yOffsetNormalized,
					ySizeNormalized, vTop, vBottom);
			}
		}
		else
		{
			if (mostSigNibble == 0x9)
			{
				// Transparent block with 1-sided texture on all sides, such as wooden arches in
				// dungeons. These do not have back-faces (especially when standing in the voxel
				// itself).
				const int textureIndex = (map1Voxel & 0x00FF) - 1;
				const bool collider = (map1Voxel & 0x0100) == 0;
				return VoxelDefinition::makeTransparentWall(textureIndex, collider);
			}
			else if (mostSigNibble == 0xA)
			{
				// Transparent block with 2-sided texture on one side (i.e. fence). Note that in
				// the center province's city, there is a temple voxel with zeroes for its texture
				// index, and it appears solid gray in the original game (presumably a silent bug).
				const int textureIndex = (map1Voxel & 0x003F) - 1;
				if (textureIndex < 0)
				{
					DebugLogWarning("Invalid texture index \"" + std::to_string(textureIndex) +
						"\" for type 0xA voxel.");
				}

				const double yOffset = [worldType, map1Voxel]()
				{
					const int baseOffset = (map1Voxel & 0x0E00) >> 9;
					const int fullOffset = (worldType == WorldType::Interior) ?
						(baseOffset * 8) : ((baseOffset * 32) - 8);

					return static_cast<double>(fullOffset) / MIFUtils::ARENA_UNITS;
				}();

				const bool collider = (map1Voxel & 0x0100) != 0;

				// "Flipped" is not present in the original game, but has been added
				// here so that all edge voxel texture coordinates (i.e., palace
				// graphics, store signs) can be correct. Currently only palace
				// graphics and gates are type 0xA colliders, I believe.
				const bool flipped = collider;

				const VoxelFacing facing = [map1Voxel]()
				{
					// Orientation is a multiple of 4 (0, 4, 8, C), where 0 is north
					// and C is east. It is stored in two bits above the texture index.
					const int orientation = (map1Voxel & 0x00C0) >> 4;
					if (orientation == 0x0)
					{
						return VoxelFacing::NegativeX;
					}
					else if (orientation == 0x4)
					{
						return VoxelFacing::PositiveZ;
					}
					else if (orientation == 0x8)
					{
						return VoxelFacing::PositiveX;
					}
					else
					{
						return VoxelFacing::NegativeZ;
					}
				}();

				return VoxelDefinition::makeEdge(textureIndex, yOffset, collider, flipped, facing);
			}
			else if (mostSigNibble == 0xB)
			{
				// Door voxel.
				const int textureIndex = (map1Voxel & 0x003F) - 1;
				const VoxelDefinition::DoorData::Type doorType = [map1Voxel]()
				{
					const int type = (map1Voxel & 0x00C0) >> 4;
					if (type == 0x0)
					{
						return VoxelDefinition::DoorData::Type::Swinging;
					}
					else if (type == 0x4)
					{
						return VoxelDefinition::DoorData::Type::Sliding;
					}
					else if (type == 0x8)
					{
						return VoxelDefinition::DoorData::Type::Raising;
					}
					else
					{
						// Arena doesn't seem to have splitting doors, but they are supported.
						DebugLogWarning("Unrecognized door type \"" + std::to_string(type) +
							"\", treating as splitting.");
						return VoxelDefinition::DoorData::Type::Splitting;
					}
				}();

				return VoxelDefinition::makeDoor(textureIndex, doorType);
			}
			else if (mostSigNibble == 0xC)
			{
				// Unknown.
				DebugLogWarning("Unrecognized voxel type 0xC.");
				return VoxelDefinition();
			}
			else if (mostSigNibble == 0xD)
			{
				// Diagonal wall.
				const int textureIndex = (map1Voxel & 0x00FF) - 1;
				const bool isRightDiag = (map1Voxel & 0x0100) == 0;
				return VoxelDefinition::makeDiagonal(textureIndex, isRightDiag);
			}
			else
			{
				DebugUnhandledReturnMsg(VoxelDefinition, std::to_string(mostSigNibble));
			}
		}
	}

	VoxelDefinition makeVoxelDefFromMAP2(ArenaTypes::VoxelID map2Voxel)
	{
		const int textureIndex = (map2Voxel & 0x007F) - 1;
		const int *menuID = nullptr;
		return VoxelDefinition::makeWall(textureIndex, textureIndex, textureIndex, menuID,
			VoxelDefinition::WallData::Type::Solid);
	}

	LockDefinition makeLockDefFromArenaLock(const ArenaTypes::MIFLock &lock)
	{
		const OriginalInt2 lockPos(lock.x, lock.y);
		const LevelInt2 newLockPos = VoxelUtils::originalVoxelToNewVoxel(lockPos);
		return LockDefinition::makeLeveledLock(newLockPos.x, 1, newLockPos.y, lock.lockLevel);
	}

	TriggerDefinition makeTriggerDefFromArenaTrigger(const ArenaTypes::MIFTrigger &trigger,
		const INFFile &inf)
	{
		const OriginalInt2 triggerPos(trigger.x, trigger.y);
		const LevelInt2 newTriggerPos = VoxelUtils::originalVoxelToNewVoxel(triggerPos);

		TriggerDefinition triggerDef;
		triggerDef.init(newTriggerPos.x, 1, newTriggerPos.y);

		// There can be a text trigger and sound trigger in the same voxel.
		const bool isTextTrigger = trigger.textIndex != -1;
		const bool isSoundTrigger = trigger.soundIndex != -1;

		// Make sure the text index points to a text value (i.e., not a key or riddle).
		if (isTextTrigger && inf.hasTextIndex(trigger.textIndex))
		{
			const INFFile::TextData &textData = inf.getText(trigger.textIndex);
			triggerDef.setTextDef(std::string(textData.text), textData.displayedOnce);
		}

		if (isSoundTrigger)
		{
			const char *soundName = inf.getSound(trigger.soundIndex);
			triggerDef.setSoundDef(String::toUppercase(soundName));
		}

		return triggerDef;
	}

	// Converts .MIF/.RMD FLOR voxels to modern voxel + entity format.
	void readArenaFLOR(const BufferView2D<const ArenaTypes::VoxelID> &flor, WorldType worldType,
		bool isPalace, const std::optional<bool> &rulerIsMale, const INFFile &inf,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager,
		LevelDefinition *outLevelDef, LevelInfoDefinition *outLevelInfoDef,
		ArenaVoxelMappingCache *voxelCache, ArenaEntityMappingCache *entityCache)
	{
		for (SNInt florZ = 0; florZ < flor.getHeight(); florZ++)
		{
			for (WEInt florX = 0; florX < flor.getWidth(); florX++)
			{
				const ArenaTypes::VoxelID florVoxel = flor.get(florX, florZ);

				// Get voxel def ID from cache or create a new one.
				LevelDefinition::VoxelDefID voxelDefID;
				const auto iter = voxelCache->find(florVoxel);
				if (iter != voxelCache->end())
				{
					voxelDefID = iter->second;
				}
				else
				{
					VoxelDefinition voxelDef = MapGeneration::makeVoxelDefFromFLOR(florVoxel, inf);
					voxelDefID = outLevelInfoDef->addVoxelDef(std::move(voxelDef));
					voxelCache->insert(std::make_pair(florVoxel, voxelDefID));
				}

				const SNInt levelX = florZ;
				const int levelY = 0;
				const WEInt levelZ = florX;
				outLevelDef->setVoxel(levelX, levelY, levelZ, voxelDefID);

				// Floor voxels can also contain data for raised platform flats.
				const int floorFlatID = florVoxel & 0x00FF;
				if (floorFlatID > 0)
				{
					// Get entity def ID from cache or create a new one.
					LevelDefinition::EntityDefID entityDefID;
					const auto iter = entityCache->find(florVoxel);
					if (iter != entityCache->end())
					{
						entityDefID = iter->second;
					}
					else
					{
						const int flatIndex = floorFlatID - 1;
						EntityDefinition entityDef;
						if (!MapGeneration::tryMakeEntityDefFromArenaFlat(flatIndex, worldType, isPalace,
							rulerIsMale, inf, charClassLibrary, entityDefLibrary, binaryAssetLibrary,
							textureManager, &entityDef))
						{
							DebugLogWarning("Couldn't make entity definition from FLAT \"" +
								std::to_string(flatIndex) + "\" with .INF \"" + inf.getName() + "\".");
							continue;
						}

						entityDefID = outLevelInfoDef->addEntityDef(std::move(entityDef));
						entityCache->insert(std::make_pair(florVoxel, entityDefID));
					}

					const LevelDouble3 entityPos(
						static_cast<SNDouble>(levelX) + 0.50,
						1.0, // Will probably be ignored in favor of raised platform top face.
						static_cast<WEDouble>(levelZ) + 0.50);
					outLevelDef->addEntity(entityDefID, entityPos);
				}
			}
		}
	}

	// Converts .MIF/.RMD MAP1 voxels to modern voxel + entity format.
	void readArenaMAP1(const BufferView2D<const ArenaTypes::VoxelID> &map1, WorldType worldType,
		bool isPalace, const std::optional<bool> &rulerIsMale, const INFFile &inf,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager,
		LevelDefinition *outLevelDef, LevelInfoDefinition *outLevelInfoDef,
		ArenaVoxelMappingCache *voxelCache, ArenaEntityMappingCache *entityCache)
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

				// Determine if this MAP1 voxel is for a voxel or entity.
				const uint8_t mostSigNibble = (map1Voxel & 0xF000) >> 12;
				const bool isVoxel = mostSigNibble != 0x8;

				const SNInt levelX = map1Z;
				const int levelY = 1;
				const WEInt levelZ = map1X;

				if (isVoxel)
				{
					// Get voxel def ID from cache or create a new one.
					LevelDefinition::VoxelDefID voxelDefID;
					const auto iter = voxelCache->find(map1Voxel);
					if (iter != voxelCache->end())
					{
						voxelDefID = iter->second;
					}
					else
					{
						VoxelDefinition voxelDef = MapGeneration::makeVoxelDefFromMAP1(
							map1Voxel, mostSigNibble, worldType, inf, binaryAssetLibrary.getExeData());
						voxelDefID = outLevelInfoDef->addVoxelDef(std::move(voxelDef));
						voxelCache->insert(std::make_pair(map1Voxel, voxelDefID));
					}

					outLevelDef->setVoxel(levelX, levelY, levelZ, voxelDefID);
				}
				else
				{
					// Get entity def ID from cache or create a new one.
					LevelDefinition::EntityDefID entityDefID;
					const auto iter = entityCache->find(map1Voxel);
					if (iter != entityCache->end())
					{
						entityDefID = iter->second;
					}
					else
					{
						const int flatIndex = map1Voxel & 0x00FF;
						EntityDefinition entityDef;
						if (!MapGeneration::tryMakeEntityDefFromArenaFlat(flatIndex, worldType,
							isPalace, rulerIsMale, inf, charClassLibrary, entityDefLibrary,
							binaryAssetLibrary, textureManager, &entityDef))
						{
							DebugLogWarning("Couldn't make entity definition from FLAT \"" +
								std::to_string(flatIndex) + "\" with .INF \"" + inf.getName() + "\".");
							continue;
						}

						entityDefID = outLevelInfoDef->addEntityDef(std::move(entityDef));
						entityCache->insert(std::make_pair(map1Voxel, entityDefID));
					}

					const LevelDouble3 entityPos(
						static_cast<SNDouble>(levelX) + 0.50,
						1.0,
						static_cast<WEDouble>(levelZ) + 0.50);
					outLevelDef->addEntity(entityDefID, entityPos);
				}
			}
		}
	}

	// Converts .MIF/.RMD MAP2 voxels to modern voxel + entity format.
	void readArenaMAP2(const BufferView2D<const ArenaTypes::VoxelID> &map2, const INFFile &inf,
		LevelDefinition *outLevelDef, LevelInfoDefinition *outLevelInfoDef,
		ArenaVoxelMappingCache *voxelCache)
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
				LevelDefinition::VoxelDefID voxelDefID;
				const auto iter = voxelCache->find(map2Voxel);
				if (iter != voxelCache->end())
				{
					voxelDefID = iter->second;
				}
				else
				{
					VoxelDefinition voxelDef = MapGeneration::makeVoxelDefFromMAP2(map2Voxel);
					voxelDefID = outLevelInfoDef->addVoxelDef(std::move(voxelDef));
					voxelCache->insert(std::make_pair(map2Voxel, voxelDefID));
				}

				// Duplicate voxels upward based on calculated height.
				const int yStart = 2;
				const int yEnd = yStart + LevelUtils::getMap2VoxelHeight(map2Voxel);
				for (int y = yStart; y < yEnd; y++)
				{
					const SNInt levelX = map2Z;
					const WEInt levelZ = map2X;
					outLevelDef->setVoxel(levelX, y, levelZ, voxelDefID);
				}
			}
		}
	}

	// Fills the equivalent MAP2 layer with duplicates of the ceiling block for a .MIF level
	// without MAP2 data.
	void readArenaCeiling(const INFFile &inf, LevelDefinition *outLevelDef,
		LevelInfoDefinition *outLevelInfoDef)
	{
		const INFFile::CeilingData &ceiling = inf.getCeiling();

		// @todo: get ceiling from .INFs without *CEILING (like START.INF). Maybe
		// hardcoding index 1 is enough?
		const int textureIndex = ceiling.textureIndex.value_or(1);

		VoxelDefinition voxelDef = VoxelDefinition::makeCeiling(textureIndex);
		LevelDefinition::VoxelDefID voxelDefID = outLevelInfoDef->addVoxelDef(std::move(voxelDef));

		for (SNInt levelX = 0; levelX < outLevelDef->getWidth(); levelX++)
		{
			for (WEInt levelZ = 0; levelZ < outLevelDef->getDepth(); levelZ++)
			{
				outLevelDef->setVoxel(levelX, 2, levelZ, voxelDefID);
			}
		}
	}

	void readArenaLock(const ArenaTypes::MIFLock &lock, const INFFile &inf, LevelDefinition *outLevelDef,
		LevelInfoDefinition *outLevelInfoDef, ArenaLockMappingCache *lockMappings)
	{
		// @todo: see if .INF file key data is relevant here.

		// Get lock def ID from cache or create a new one.
		LevelDefinition::LockDefID lockDefID;
		const auto iter = lockMappings->find(lock);
		if (iter != lockMappings->end())
		{
			lockDefID = iter->second;
		}
		else
		{
			LockDefinition lockDef = MapGeneration::makeLockDefFromArenaLock(lock);
			lockDefID = outLevelInfoDef->addLockDef(std::move(lockDef));
			lockMappings->insert(std::make_pair(lock, lockDefID));
		}

		const LockDefinition &lockDef = outLevelInfoDef->getLockDef(lockDefID);
		const SNInt x = lockDef.getX();
		const int y = lockDef.getY();
		const WEInt z = lockDef.getZ();
		outLevelDef->addLock(lockDefID, LevelInt3(x, y, z));
	}

	void readArenaTrigger(const ArenaTypes::MIFTrigger &trigger, const INFFile &inf,
		LevelDefinition *outLevelDef, LevelInfoDefinition *outLevelInfoDef,
		ArenaTriggerMappingCache *triggerMappings)
	{
		// Get trigger def ID from cache or create a new one.
		LevelDefinition::TriggerDefID triggerDefID;
		const auto iter = triggerMappings->find(trigger);
		if (iter != triggerMappings->end())
		{
			triggerDefID = iter->second;
		}
		else
		{
			TriggerDefinition triggerDef = MapGeneration::makeTriggerDefFromArenaTrigger(trigger, inf);
			triggerDefID = outLevelInfoDef->addTriggerDef(std::move(triggerDef));
			triggerMappings->insert(std::make_pair(trigger, triggerDefID));
		}

		const TriggerDefinition &triggerDef = outLevelInfoDef->getTriggerDef(triggerDefID);
		const SNInt x = triggerDef.getX();
		const int y = triggerDef.getY();
		const WEInt z = triggerDef.getZ();
		outLevelDef->addTrigger(triggerDefID, LevelInt3(x, y, z));
	}

	void generateArenaDungeonLevel(const MIFFile &mif, WEInt widthChunks, SNInt depthChunks,
		int levelUpBlock, const std::optional<int> &levelDownBlock, ArenaRandom &random,
		WorldType worldType, bool isPalace, const std::optional<bool> &rulerIsMale, const INFFile &inf,
		const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager,
		LevelDefinition *outLevelDef, LevelInfoDefinition *outLevelInfoDef,
		ArenaVoxelMappingCache *florMappings, ArenaVoxelMappingCache *map1Mappings,
		ArenaEntityMappingCache *entityMappings, ArenaLockMappingCache *lockMappings,
		ArenaTriggerMappingCache *triggerMappings)
	{
		// Create buffers for level blocks.
		Buffer2D<ArenaTypes::VoxelID> levelFLOR(mif.getWidth() * widthChunks, mif.getDepth() * depthChunks);
		Buffer2D<ArenaTypes::VoxelID> levelMAP1(levelFLOR.getWidth(), levelFLOR.getHeight());
		levelFLOR.fill(0);
		levelMAP1.fill(0);

		const int tileSet = random.next() % 4;

		for (SNInt row = 0; row < depthChunks; row++)
		{
			const SNInt zOffset = row * InteriorLevelUtils::DUNGEON_CHUNK_DIM;
			for (WEInt column = 0; column < widthChunks; column++)
			{
				const WEInt xOffset = column * InteriorLevelUtils::DUNGEON_CHUNK_DIM;

				// Get the selected level from the random chunks .MIF file.
				const int blockIndex = (tileSet * 8) + (random.next() % 8);
				const auto &blockLevel = mif.getLevel(blockIndex);
				const BufferView2D<const ArenaTypes::VoxelID> &blockFLOR = blockLevel.getFLOR();
				const BufferView2D<const ArenaTypes::VoxelID> &blockMAP1 = blockLevel.getMAP1();

				// Copy block data to temp buffers.
				for (SNInt z = 0; z < InteriorLevelUtils::DUNGEON_CHUNK_DIM; z++)
				{
					for (WEInt x = 0; x < InteriorLevelUtils::DUNGEON_CHUNK_DIM; x++)
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
				const BufferView<const ArenaTypes::MIFLock> &blockLOCK = blockLevel.getLOCK();
				for (int i = 0; i < blockLOCK.getCount(); i++)
				{
					const auto &lock = blockLOCK.get(i);

					ArenaTypes::MIFLock tempLock;
					tempLock.x = xOffset + lock.x;
					tempLock.y = zOffset + lock.y;
					tempLock.lockLevel = lock.lockLevel;

					MapGeneration::readArenaLock(tempLock, inf, outLevelDef, outLevelInfoDef, lockMappings);
				}

				// Assign text/sound triggers to the current block.
				const BufferView<const ArenaTypes::MIFTrigger> &blockTRIG = blockLevel.getTRIG();
				for (int i = 0; i < blockTRIG.getCount(); i++)
				{
					const auto &trigger = blockTRIG.get(i);

					ArenaTypes::MIFTrigger tempTrigger;
					tempTrigger.x = xOffset + trigger.x;
					tempTrigger.y = zOffset + trigger.y;
					tempTrigger.textIndex = trigger.textIndex;
					tempTrigger.soundIndex = trigger.soundIndex;

					MapGeneration::readArenaTrigger(tempTrigger, inf, outLevelDef, outLevelInfoDef,
						triggerMappings);
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
		InteriorLevelUtils::unpackLevelChangeVoxel(levelUpBlock, &levelUpX, &levelUpZ);
		levelMAP1.set(InteriorLevelUtils::offsetLevelChangeVoxel(levelUpX),
			InteriorLevelUtils::offsetLevelChangeVoxel(levelUpZ),
			InteriorLevelUtils::convertLevelChangeVoxel(levelUpVoxelByte));

		if (levelDownBlock.has_value())
		{
			const uint8_t levelDownVoxelByte = *inf.getLevelDownIndex() + 1;
			WEInt levelDownX;
			SNInt levelDownZ;
			InteriorLevelUtils::unpackLevelChangeVoxel(*levelDownBlock, &levelDownX, &levelDownZ);
			levelMAP1.set(InteriorLevelUtils::offsetLevelChangeVoxel(levelDownX),
				InteriorLevelUtils::offsetLevelChangeVoxel(levelDownZ),
				InteriorLevelUtils::convertLevelChangeVoxel(levelDownVoxelByte));
		}

		// Convert temp voxel buffers to the modern format.
		const BufferView2D<const ArenaTypes::VoxelID> levelFlorView(
			levelFLOR.get(), levelFLOR.getWidth(), levelFLOR.getHeight());
		const BufferView2D<const ArenaTypes::VoxelID> levelMap1View(
			levelMAP1.get(), levelMAP1.getWidth(), levelMAP1.getHeight());
		MapGeneration::readArenaFLOR(levelFlorView, worldType, isPalace, rulerIsMale, inf,
			charClassLibrary, entityDefLibrary, binaryAssetLibrary, textureManager, outLevelDef,
			outLevelInfoDef, florMappings, entityMappings);
		MapGeneration::readArenaMAP1(levelMap1View, worldType, isPalace, rulerIsMale, inf,
			charClassLibrary, entityDefLibrary, binaryAssetLibrary, textureManager, outLevelDef,
			outLevelInfoDef, map1Mappings, entityMappings);

		// Generate ceiling (if any).
		if (!inf.getCeiling().outdoorDungeon)
		{
			MapGeneration::readArenaCeiling(inf, outLevelDef, outLevelInfoDef);
		}
	}
}

void MapGeneration::readMifVoxels(const BufferView<const MIFFile::Level> &levels, WorldType worldType,
	bool isPalace, const std::optional<bool> &rulerIsMale, const INFFile &inf,
	const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager,
	BufferView<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef)
{
	// Each .MIF level voxel is unpacked into either a voxel or entity. These caches point to
	// previously-added definitions in the level info def.
	ArenaVoxelMappingCache florMappings, map1Mappings, map2Mappings;
	ArenaEntityMappingCache entityMappings;

	for (int i = 0; i < levels.getCount(); i++)
	{
		const MIFFile::Level &level = levels.get(i);
		LevelDefinition &levelDef = outLevelDefs.get(i);
		MapGeneration::readArenaFLOR(level.getFLOR(), worldType, isPalace, rulerIsMale, inf,
			charClassLibrary, entityDefLibrary, binaryAssetLibrary, textureManager, &levelDef,
			outLevelInfoDef, &florMappings, &entityMappings);
		MapGeneration::readArenaMAP1(level.getMAP1(), worldType, isPalace, rulerIsMale, inf,
			charClassLibrary, entityDefLibrary, binaryAssetLibrary, textureManager, &levelDef,
			outLevelInfoDef, &map1Mappings, &entityMappings);

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
	SNInt depthChunks, const INFFile &inf, ArenaRandom &random, WorldType worldType, bool isPalace,
	const std::optional<bool> &rulerIsMale, const CharacterClassLibrary &charClassLibrary,
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, BufferView<LevelDefinition> &outLevelDefs,
	LevelInfoDefinition *outLevelInfoDef, LevelInt2 *outStartPoint)
{
	ArenaVoxelMappingCache florMappings, map1Mappings;
	ArenaEntityMappingCache entityMappings;
	ArenaLockMappingCache lockMappings;
	ArenaTriggerMappingCache triggerMappings;

	// Store the seed for later, to be used with block selection.
	const uint32_t seed2 = random.getSeed();

	// Determine transition blocks (*LEVELUP/*LEVELDOWN) that will appear in the dungeon.
	auto getNextTransBlock = [widthChunks, depthChunks, &random]()
	{
		const SNInt tY = random.next() % depthChunks;
		const WEInt tX = random.next() % widthChunks;
		return InteriorLevelUtils::packLevelChangeVoxel(tX, tY);
	};

	// Packed coordinates for transition blocks.
	// @todo: maybe this could be an int pair so packing is not required.
	std::vector<int> transitions;

	// Handle initial case where transitions list is empty (for i == 0).
	transitions.push_back(getNextTransBlock());

	// Handle general case for transitions list additions.
	for (int i = 1; i < levelCount; i++)
	{
		int transBlock;
		do
		{
			transBlock = getNextTransBlock();
		} while (transBlock == transitions.back());

		transitions.push_back(transBlock);
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
			levelDownBlock, random, worldType, isPalace, rulerIsMale, inf, charClassLibrary,
			entityDefLibrary, binaryAssetLibrary, textureManager, &levelDef, outLevelInfoDef,
			&florMappings, &map1Mappings, &entityMappings, &lockMappings, &triggerMappings);
	}

	// The start point depends on where the level up voxel is on the first level.
	DebugAssertIndex(transitions, 0);
	const int firstTransition = transitions[0];
	WEInt firstTransitionChunkX;
	SNInt firstTransitionChunkZ;
	InteriorLevelUtils::unpackLevelChangeVoxel(
		firstTransition, &firstTransitionChunkX, &firstTransitionChunkZ);

	// Convert it from the old coordinate system to the new one.
	const OriginalInt2 startPoint(
		InteriorLevelUtils::offsetLevelChangeVoxel(firstTransitionChunkX),
		InteriorLevelUtils::offsetLevelChangeVoxel(firstTransitionChunkZ));
	*outStartPoint = VoxelUtils::originalVoxelToNewVoxel(startPoint);
}

void MapGeneration::readMifLocks(const BufferView<const MIFFile::Level> &levels, const INFFile &inf,
	BufferView<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef)
{
	ArenaLockMappingCache lockMappings;

	for (int i = 0; i < levels.getCount(); i++)
	{
		const MIFFile::Level &level = levels.get(i);
		LevelDefinition &levelDef = outLevelDefs.get(i);
		const BufferView<const ArenaTypes::MIFLock> locks = level.getLOCK();

		for (int j = 0; j < locks.getCount(); j++)
		{
			const ArenaTypes::MIFLock &lock = locks.get(j);
			MapGeneration::readArenaLock(lock, inf, &levelDef, outLevelInfoDef, &lockMappings);
		}
	}
}

void MapGeneration::readMifTriggers(const BufferView<const MIFFile::Level> &levels, const INFFile &inf,
	BufferView<LevelDefinition> &outLevelDefs, LevelInfoDefinition *outLevelInfoDef)
{
	ArenaTriggerMappingCache triggerMappings;

	for (int i = 0; i < levels.getCount(); i++)
	{
		const MIFFile::Level &level = levels.get(i);
		LevelDefinition &levelDef = outLevelDefs.get(i);
		const BufferView<const ArenaTypes::MIFTrigger> triggers = level.getTRIG();

		for (int j = 0; j < triggers.getCount(); j++)
		{
			const ArenaTypes::MIFTrigger &trigger = triggers.get(j);
			MapGeneration::readArenaTrigger(trigger, inf, &levelDef, outLevelInfoDef, &triggerMappings);
		}
	}
}
