#ifndef ARENA_TYPES_H
#define ARENA_TYPES_H

#include <array>
#include <cstddef>
#include <cstdint>

// Various composite types used with Arena's binary data files.

class ArenaTypes
{
private:
	ArenaTypes() = delete;
	~ArenaTypes() = delete;
public:
	struct Light
	{
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
		static const size_t SIZE;

		uint8_t x, y, lockLevel;

		void init(const uint8_t *data);
	};

	struct MIFTarget
	{
		uint8_t x, y;

		void init(const uint8_t *data);
	};

	struct MIFTrigger
	{
		static const size_t SIZE;

		uint8_t x, y;

		// Some text and sound indices are negative (which means they're unused), 
		// so they need to be signed.
		int8_t textIndex, soundIndex;

		void init(const uint8_t *data);
	};

	struct GameState
	{
		uint8_t junk1;
		uint8_t weatherFlags; // 0x80 precipitation, 1 rain, 2 snow.
		uint8_t playerFloor;
		uint8_t oldFloor;
		std::array<uint8_t, 182> junk2;
		uint16_t lightsCount;
		std::array<Light, 256> lights; // Auto-generated lights.
		uint32_t junk3;
		std::array<char, 33> levelName;
		std::array<char, 13> infName;
		std::array<uint8_t, 80> junk4;
		MIFHeader levelHeader; // 61 bytes; same as in .MIF files.
		std::array<char, 13> mifName;
		uint32_t junk5;
		uint16_t walkSpeed;
		uint16_t turnSpeed;
		uint32_t junk6;
		uint16_t flags5;
		uint16_t flags4;
		std::array<uint8_t, 30> junk7;
		uint16_t levelEntryX, levelEntryY;
		uint16_t playerX, playerZ, playerY;
		uint32_t unknown1;
		uint16_t playerAngle;
		std::array<uint8_t, 6> junk8;
		uint16_t lightRadius;
		std::array<uint8_t, 240> junk9;
		std::array<MIFLock, 64> locks;
		uint16_t lockCount;
		std::array<MIFTarget, 64> targets;
		uint16_t targetCount;
		std::array<MIFTrigger, 64> triggers;
		uint16_t triggerCount;
		std::array<uint8_t, 89> junk10;

		void init(const uint8_t *data);
	};

	// For SAVEGAME.0x.
	struct SaveGame
	{
		std::array<uint8_t, 320 * 200> screenBuffer;
		std::array<uint8_t, 256 * 3> palette;
		GameState gameState;
		std::array<uint16_t, 128 * 128 * 3> gameLevel;

		void init(const uint8_t *data);
	};

	// For SAVEENGN.0x.
	struct SaveEngine
	{
		struct CreatureData
		{
			std::array<uint8_t, 32> unknown;

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

		struct LootItem
		{
			static const size_t SIZE;

			uint16_t unknown1, containerPosition;
			uint8_t floor;
			uint16_t goldValue, unknown2;
			InventoryItem inventoryItem;

			void init(const uint8_t *data);
		};

		struct NPCData
		{
			static const size_t SIZE;

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
			std::array<uint8_t, 4> misc2;
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
			// To do.
			void init(const uint8_t *data);
		};

		struct BaseQuest
		{
			static const size_t SIZE;

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
			static const size_t SIZE;

			BaseQuest baseQuest;
			std::array<uint8_t, 5> unknown;
			uint8_t faction, npcRace, monsterRace, locationName, locNameTemplate;

			void init(const uint8_t *data);
		};

		struct MainQuestData
		{
			static const size_t SIZE;

			uint8_t canHaveVision, nextStep, dungeonLocationKnown, acceptedKeyQuest,
				hadVision, talkedToRuler, stepDone, hasKeyItem, hasStaffPiece, showKey;

			void init(const uint8_t *data);
		};

		struct ArtifactQuestData
		{
			static const size_t SIZE;

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
			static const size_t SIZE;

			uint32_t gold, experience;
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
			std::array<MIFTrigger, 6> dynamicTriggers;
			std::array<uint8_t, 14> keyRing;
			std::array<Buff, 16> buffs;
			std::array<uint8_t, 2> junk5;
			CityGenData currentCity;
			std::array<uint8_t, 36> weather;
			std::array<uint8_t, 53> junk6;
			std::array<NPCSprite, 15> npcs;
			std::array<NPCSprite, 8> enemies;
			std::array<uint8_t, 93> junk7;
			std::array<BaseQuest, 4> cityQuests;
			std::array<ExtQuest, 4> palaceQuests;
			ExtQuest rumorsBuffer;
			uint32_t unknown;
			MainQuestData mainQuest; // 11 bytes.
			std::array<uint8_t, 8> junk8;
			std::array<uint16_t, 32> pickedLocks;
			std::array<uint8_t, 6> junk9;
			uint16_t worldX, worldY, junk10;
			std::array<uint8_t, 9> portrait;
			ArtifactQuestData artifactQuest;
			std::array<uint8_t, 5> junk11;

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
		struct HashTable
		{
			static const size_t SIZE;

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

	// For SPELLS.0x and SPELLSG.0x.
	struct Spells
	{
		// To do: get from existing SpellData in MiscAssets.
		void init(const uint8_t *data);
	};

	// For INN.0x.
	struct Tavern
	{
		uint16_t remainingHours;
		uint32_t timeLimit;

		void init(const uint8_t *data);
	};

	// For AUTOMAP.0x.
	struct Automap
	{
		struct FogOfWarCache
		{
			static const size_t SIZE;

			struct Note
			{
				static const size_t SIZE;

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
		// To do.
		void init(const uint8_t *data);
	};
};

#endif
