#ifndef ARENA_TYPES_H
#define ARENA_TYPES_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Various types used with Arena's binary data files. Struct sizes are hardcoded to show
// intent and to avoid issues with padding since they map directly to Arena's data.

namespace ArenaTypes
{
	// For .MIF and .RMD FLOR/MAP1/MAP2 voxels.
	using VoxelID = uint16_t;

	// Flat entries can be in .MIF and .RMD FLOR/MAP1 voxels.
	using FlatIndex = uint16_t;

	// *ITEM indices are used for entities; some values are reserved for creatures, humans, etc..
	using ItemIndex = uint16_t;

	// Maps one or more *MENU IDs to a type of transition voxel. Cities and the wilderness interpret
	// IDs differently.
	enum class MenuType
	{
		None,
		CityGates, // Transition between city and wilderness.
		Crypt, // WCRYPT
		Dungeon, // DUNGEON
		Equipment, // EQUIP
		House, // BS
		MagesGuild, // MAGE
		Noble, // NOBLE
		Palace, // PALACE
		Tavern, // TAVERN
		Temple, // TEMPLE
		Tower // TOWER
	};

	// Helps determine entity definitions during level generation. Separate from MenuType since it's only
	// for interiors.
	// - @todo: add an InteriorDefinition at some point to further de-hardcode things. It would contain
	//   rulerIsMale, etc.. Might also have a variable for loot piles when sneaking in at night.
	enum class InteriorType
	{
		Crypt, // WCRYPT
		Dungeon, // DUNGEON
		Equipment, // EQUIP
		House, // BS
		MagesGuild, // MAGE
		Noble, // NOBLE
		Palace, // PALACE
		Tavern, // TAVERN
		Temple, // TEMPLE
		Tower // TOWER
	};

	// Types of city locations in a world map province.
	enum class CityType
	{
		CityState,
		Town,
		Village
	};

	// Each type of voxel definition. These are mostly used with rendering, but also for determining how to
	// interpret the voxel data itself. If the type is None, then the voxel is empty and there is nothing
	// to render.
	enum class VoxelType
	{
		None,
		Wall,
		Floor,
		Ceiling,
		Raised,
		Diagonal,
		TransparentWall,
		Edge,
		Chasm,
		Door
	};

	enum class ChasmType
	{
		Dry,
		Wet,
		Lava
	};

	// Each type of door. Most doors swing open, while others raise up or slide to the side.
	// Splitting doors do not appear in the original game but are supposedly supported.
	enum class DoorType
	{
		Swinging,
		Sliding,
		Raising,
		Splitting
	};

	struct Light
	{
		static constexpr size_t SIZE = 6;

		// X and Y are in Arena units.
		uint16_t x, y, radius;

		void init(const uint8_t *data);
	};

	struct MIFHeader
	{
		uint8_t unknown1, entryCount;
		std::array<uint16_t, 4> startX, startY;
		uint8_t startingLevelIndex, levelCount, unknown2;
		uint16_t mapWidth, mapHeight;
		std::array<uint8_t, 34> unknown3;

		void init(const uint8_t *data);
	};

	struct MIFLock
	{
		static constexpr size_t SIZE = 3;

		uint8_t x, y, lockLevel;

		void init(const uint8_t *data);
	};

	struct MIFTarget
	{
		static constexpr size_t SIZE = 2;

		uint8_t x, y;

		void init(const uint8_t *data);
	};

	struct MIFTrigger
	{
		static constexpr size_t SIZE = 4;

		uint8_t x, y;

		// Some text and sound indices are negative (which means they're unused), 
		// so they need to be signed.
		int8_t textIndex, soundIndex;

		void init(const uint8_t *data);
	};

	struct DynamicTrigger
	{
		static constexpr size_t SIZE = 6;

		// @todo. Similar to MIFTrigger but can be disabled?
		std::array<uint8_t, 6> unknown;

		void init(const uint8_t *data);
	};

	struct GameState
	{
		static constexpr size_t SIZE = 3559;

		uint8_t junk1;
		uint8_t weatherFlags; // 0x80 precipitation, 1 rain, 2 snow.
		uint8_t playerFloor;
		uint8_t oldFloor;
		std::array<uint8_t, 388> junk2;
		uint16_t lightsCount;
		std::array<Light, 256> lights; // Auto-generated lights.
		uint32_t junk3;
		std::array<char, 33> levelName;
		std::array<char, 13> infName;
		std::array<uint8_t, 128> junk4;
		MIFHeader levelHeader;
		std::array<char, 13> mifName;
		uint32_t junk5;
		uint16_t walkSpeed;
		uint16_t turnSpeed;
		uint32_t junk6;
		uint16_t flags5;
		uint16_t flags4;
		std::array<uint8_t, 48> junk7;
		uint16_t levelEntryX, levelEntryY;
		uint16_t playerX, playerZ, playerY;
		uint32_t unknown1;
		uint16_t playerAngle;
		std::array<uint8_t, 6> junk8;
		uint16_t lightRadius;
		std::array<uint8_t, 576> junk9;
		std::array<MIFLock, 64> locks;
		uint8_t lockCount;
		std::array<MIFTarget, 64> targets;
		uint8_t targetCount;
		std::array<MIFTrigger, 64> triggers;
		uint8_t triggerCount;
		std::array<uint8_t, 136> junk10;

		void init(const uint8_t *data);
	};

	// For SAVEGAME.0x.
	struct SaveGame
	{
		static constexpr size_t SIZE = 166631;

		std::array<uint8_t, 320 * 200> screenBuffer;
		std::array<uint8_t, 256 * 3> palette;
		GameState gameState;
		std::array<uint16_t, 128 * 128 * 3> gameLevel;

		void init(const uint8_t *data);
	};

	struct InventoryItem
	{
		uint8_t slotID;
		uint16_t weight;
		uint8_t hands, param1, param2;
		uint16_t health, maxHealth;
		uint32_t price;
		uint8_t flags, x, material, y, attribute;

		void init(const uint8_t *data);
	};

	// For SAVEENGN.0x.
	struct SaveEngine
	{
		static constexpr size_t SIZE = 17983;

		struct CreatureData
		{
			std::array<uint8_t, 32> unknown;

			void init(const uint8_t *data);
		};

		struct LootItem
		{
			static constexpr size_t SIZE = 28;

			uint16_t unknown1, containerPosition;
			uint8_t floor;
			uint16_t goldValue, unknown2;
			InventoryItem inventoryItem;

			void init(const uint8_t *data);
		};

		struct NPCData
		{
			static constexpr size_t SIZE = 1054;

			uint32_t randomSeed;
			uint8_t raceID, classID, level, isFemale, homeCityID;

			union
			{
				std::array<char, 32> name;
				CreatureData creatureData;
			};

			std::array<uint8_t, 8> currentAttributes, baseAttributes;
			std::array<uint16_t, 16> actorValues;
			uint16_t hp, maxHP, stamina;
			std::array<uint8_t, 7> misc;
			uint16_t spellPoints, maxSpellPoints;
			uint8_t misc2;
			uint8_t face; // Face index in appropriate CIF file.
			std::array<uint8_t, 2> misc3;
			std::array<InventoryItem, 40> inventory;
			uint8_t knownSpellCount;
			std::array<uint8_t, 160> knownSpellIDs;
			uint16_t gold;
			uint32_t experience; // NPC only.
			uint16_t statusFlags;
			std::array<uint8_t, 10> statusCounters;
			uint8_t encumbranceMod;
			uint16_t activeEffects, shieldValue;

			void init(const uint8_t *data);
		};

		struct CityGenData
		{
			static constexpr size_t SIZE = 56;

			std::array<char, 20> cityName;
			std::array<char, 13> mifName;
			uint8_t citySize; // 4, 5, 6.
			uint16_t blockOffset;
			uint8_t provinceID, cityType;
			uint16_t localX, localY;
			uint8_t cityID, unknown;
			uint16_t absLatitude;
			uint8_t terrainType, quarter;
			uint32_t rulerSeed, citySeed;

			void init(const uint8_t *data);
		};

		struct Buff
		{
			std::array<uint8_t, 60> unknown;

			void init(const uint8_t *data);
		};

		struct NPCSprite
		{
			static constexpr size_t SIZE = 28;

			uint16_t x, z, y;
			uint16_t pDynamicLight; // void* in original.
			uint16_t speed, angle;
			uint8_t flat, frame, param1;
			uint16_t flags;
			uint8_t param2;
			uint16_t data, param3, unknown1, unknown2, param4;

			void init(const uint8_t *data);
		};

		struct BaseQuest
		{
			static constexpr size_t SIZE = 41;

			uint32_t questSeed;
			uint16_t location1, item1;
			uint32_t startDate, dueDate, tavernData, destinationData, reward;
			uint16_t portrait;
			uint32_t questGiverSeed;
			uint16_t opponentFaction;
			uint8_t destinationCityID, questCityID, unknown, relationship, escorteeIsFemale;

			void init(const uint8_t *data);
		};

		struct ExtQuest
		{
			static constexpr size_t SIZE = 51;

			BaseQuest baseQuest;
			std::array<uint8_t, 5> unknown;
			uint8_t faction, npcRace, monsterRace, locationName, locNameTemplate;

			void init(const uint8_t *data);
		};

		struct MainQuestData
		{
			static constexpr size_t SIZE = 10;

			uint8_t canHaveVision, nextStep, dungeonLocationKnown, acceptedKeyQuest,
				hadVision, talkedToRuler, stepDone, hasKeyItem, hasStaffPiece, showKey;

			void init(const uint8_t *data);
		};

		struct ArtifactQuestData
		{
			static constexpr size_t SIZE = 19;

			uint8_t currentArtifact;
			uint32_t tavernLocation;
			uint8_t mapDungeonID, mapProvinceID, artifactDungeonID, artifactProvinceID,
				artifactQuestFlags;
			uint16_t artifactBitMask;
			uint32_t artifactQuestStarted;
			uint8_t artifactDays;
			uint16_t artifactPriceOrOffset;

			void init(const uint8_t *data);
		};

		struct PlayerData
		{
			static constexpr size_t SIZE = 2609;

			uint32_t gold;
			uint16_t blessing, flags2, gameOptions;
			uint32_t gameTime;
			std::array<uint16_t, 5> dateTime; // YMDHm.
			uint32_t junk1;
			uint16_t movementAngle, flags6;
			uint8_t junk2, jumpPhase;
			uint8_t flags7; // 1 if in the starting dungeon.
			std::array<uint8_t, 23> junk3;
			uint32_t randomDungeonSeed;
			uint16_t wildX, wildY;
			std::array<uint8_t, 4> wildBlocks; // 0-70 presumably.
			uint16_t wildBlockX, wildBlockY;
			std::array<uint16_t, 4> unknownCoord;
			uint16_t angle;
			uint32_t wildSeed;
			std::array<uint8_t, 8> junk4;
			std::array<DynamicTrigger, 8> dynamicTriggers;
			std::array<uint8_t, 14> keyRing;
			std::array<Buff, 16> buffs;
			std::array<uint8_t, 2> junk5;
			CityGenData currentCity;
			std::array<uint8_t, 36> weather;
			std::array<uint8_t, 53> junk6;
			std::array<NPCSprite, 15> npcs;
			std::array<NPCSprite, 8> enemies;
			std::array<uint8_t, 147> junk7;
			std::array<BaseQuest, 4> cityQuests;
			std::array<ExtQuest, 4> palaceQuests;
			ExtQuest rumorsBuffer;
			uint32_t unknown;
			MainQuestData mainQuest;
			std::array<uint8_t, 8> junk8;
			std::array<uint16_t, 32> pickedLocks;
			std::array<uint8_t, 6> junk9;
			uint16_t worldX, worldY, junk10;
			std::array<uint8_t, 9> portrait;
			ArtifactQuestData artifactQuest;
			std::array<uint8_t, 2> junk11;
			uint8_t lootRecordCount;

			void init(const uint8_t *data);
		};

		struct InternalState
		{
			std::array<uint8_t, 288> unknown;

			void init(const uint8_t *data);
		};

		// First two members are scrambled.
		NPCData player;
		PlayerData playerData;
		std::array<NPCData, 8> enemies;
		std::array<LootItem, 200> loot;
		InternalState gameState2;

		void init(const uint8_t *data);
	};

	// For STATES.0x (main quest lock and trigger states).
	struct MQLevelState
	{
		static constexpr size_t SIZE = 28800;

		struct HashTable
		{
			static constexpr size_t SIZE = 450;

			uint8_t triggerCount;
			std::array<MIFTrigger, 64> triggers;
			uint8_t lockCount;
			std::array<MIFLock, 64> locks;

			void init(const uint8_t *data);
		};

		// Index is (((provinceID * 2) + localDungeonID) * 4) + level.
		std::array<HashTable, 64> hashTables;

		void init(const uint8_t *data);
	};

	// For each spell in SPELLS.0x and SPELLSG.0x.
	struct SpellData
	{
		static constexpr size_t SIZE = 85;

		std::array<std::array<uint16_t, 3>, 6> params;
		uint8_t targetType, unknown, element;
		uint16_t flags;

		// Effects (i.e., "Fortify"; 0xFF = nothing), sub-effects (i.e., "Attribute"),
		// and affected attributes (i.e., "Strength").
		std::array<uint8_t, 3> effects, subEffects, affectedAttributes;

		uint16_t cost;
		std::array<char, 33> name;

		void init(const uint8_t *data);

		template <size_t T>
		static void initArray(std::array<SpellData, T> &arr, const uint8_t *data)
		{
			for (size_t i = 0; i < arr.size(); i++)
			{
				arr[i].init(data + (SpellData::SIZE * i));
			}
		}
	};

	// For SPELLS.0x (custom spells).
	using Spells = std::array<SpellData, 32>;

	// For SPELLSG.0x (general spells).
	using Spellsg = std::array<SpellData, 128>;

	// For IN#.0x.
	struct Tavern
	{
		static constexpr size_t SIZE = 6;

		uint16_t remainingHours;
		uint32_t timeLimit;

		void init(const uint8_t *data);
	};

	// For RE#.0x (EQ#.0x can be ignored).
	struct Repair
	{
		static constexpr size_t SIZE = 120;

		struct Job
		{
			static constexpr size_t SIZE = 24;

			uint8_t valid;
			uint32_t dueTo;
			InventoryItem item;

			void init(const uint8_t *data);
		};

		std::array<Job, 5> jobs;

		void init(const uint8_t *data);
	};

	// For AUTOMAP.0x.
	struct Automap
	{
		static constexpr size_t SIZE = 131136;

		struct FogOfWarCache
		{
			static constexpr size_t SIZE = 8196;

			struct Note
			{
				static constexpr size_t SIZE = 64;

				uint16_t x, y;
				std::array<char, 60> text;

				void init(const uint8_t *data);
			};

			uint32_t levelHash;
			std::array<Note, 64> notes;
			std::array<uint8_t, 4096> bitmap; // 2 bits per block.

			void init(const uint8_t *data);
		};

		std::array<FogOfWarCache, 16> caches;

		void init(const uint8_t *data);
	};

	// For LOG.0x.
	struct Log
	{
		struct Entry
		{
			std::string title, body;

			void init(const std::string &data);
		};

		std::vector<Entry> entries;

		void init(const std::string &data);
	};

	// For NAMES.DAT.
	struct Names
	{
		static constexpr size_t SIZE = 480;

		struct Entry
		{
			static constexpr size_t SIZE = 48;

			std::array<char, 48> name;

			void init(const uint8_t *data);
		};

		std::array<Entry, 10> entries;

		void init(const uint8_t *data);
	};
}

#endif
