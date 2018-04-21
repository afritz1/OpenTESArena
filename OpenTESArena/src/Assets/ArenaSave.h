#ifndef ARENA_SAVE_H
#define ARENA_SAVE_H

#include <array>
#include <cstdint>

class ArenaSave
{
private:
	ArenaSave() = delete;
	~ArenaSave() = delete;
public:
	// For SAVEGAME.0x.
	struct SaveGame
	{
		struct GameState
		{
			std::array<uint8_t, 3559> unknown;
		};

		std::array<uint8_t, 320 * 200> screenBuffer;
		std::array<uint8_t, 256 * 3> palette;
		GameState gameState;
		std::array<uint16_t, 128 * 128 * 3> gameLevel;
	};

	struct Trigger
	{
		// To do. Same as .MIF trigger?
	};

	struct Lock
	{
		// To do. Same as .MIF lock?
	};

	// For SAVEENGN.0x.
	struct SaveEngine
	{
		struct CreatureData
		{
			std::array<uint8_t, 32> unknown;
		};

		struct InventoryItem
		{
			uint8_t slotID;
			uint16_t weight;
			uint8_t hands, param1, param2;
			uint16_t health, maxHealth;
			uint32_t price;
			uint8_t flags, x, material, y, attribute;
		};

		struct LootItem
		{
			uint16_t unknown1, containerPosition;
			uint8_t floor;
			uint16_t goldValue, unknown2;
			InventoryItem inventoryItem;
		};

		struct NpcData
		{
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
			uint16_t activeEffects, shieldValue;
		};

		struct CityGenData
		{
			std::array<char, 20> cityName;
			std::array<char, 13> mifName;
			uint8_t citySize; // 4, 5, 6.
			uint16_t blockOffset;
			uint8_t province, cityType;
			uint16_t localX, localY;
			uint8_t cityID, unknown;
			uint16_t absLatitude;
			uint8_t terrainType, quarter;
			uint32_t rulerSeed, citySeed;
		};

		struct Buff
		{
			std::array<uint8_t, 60> unknown;
		};

		struct NpcSprite
		{
			// To do.
		};

		struct BaseQuest
		{
			uint32_t questSeed;
			uint16_t location1, item1;
			uint32_t startDate, dueDate, tavernData, destinationData, reward;
			uint16_t portrait;
			uint32_t questGiverSeed;
			uint16_t opponentFaction;
			uint8_t destinationCityID, questCityID, unknown, relationship, escorteeIsFemale;
		};

		struct ExtQuest
		{
			BaseQuest baseQuest;
			// To do.
		};

		struct MainQuestData
		{
			// To do.
		};

		struct ArtifactQuestData
		{
			// To do.
		};

		struct PlayerData
		{
			uint32_t gold, experience;
			uint16_t blessing, flags2, gameOptions;
			uint32_t gameTime;
			std::array<uint16_t, 5> dateTime; // YMDHm.
			std::array<uint8_t, 34> junk;
			uint32_t randomDungeonSeed;
			uint16_t wildX, wildY;
			std::array<uint8_t, 4> wildBlocks; // 0-70 presumably.
			uint16_t wildBlockX, wildBlockY;
			std::array<uint16_t, 4> unknownCoord;
			uint16_t angle;
			uint32_t wildSeed;
			std::array<uint8_t, 8> junk2;
			std::array<Trigger, 6> dynamicTriggers;
			std::array<uint8_t, 14> keyRing;
			std::array<Buff, 16> buffs;
			std::array<uint8_t, 2> junk3;
			CityGenData currentCity;
			std::array<uint8_t, 36> weather;
			std::array<uint8_t, 53> junk4;
			std::array<NpcSprite, 15> npcs;
			std::array<NpcSprite, 8> enemies;
			std::array<uint8_t, 93> junk5;
			std::array<BaseQuest, 4> cityQuests;
			std::array<ExtQuest, 4> palaceQuests;
			ExtQuest rumorsBuffer;
			uint32_t unknown;
			MainQuestData mainQuest;
			std::array<uint8_t, 7> junk6;
			std::array<uint16_t, 32> pickedLocks;
			std::array<uint8_t, 6> junk7;
			uint16_t worldX, worldY, junk8;
			std::array<uint8_t, 9> portrait;
			ArtifactQuestData artifactQuest;
			std::array<uint8_t, 5> junk9;
		};

		struct InternalState
		{
			std::array<uint8_t, 288> unknown;
		};

		// First two members are scrambled.
		NpcData player;
		PlayerData playerData;
		std::array<NpcData, 8> enemies;
		std::array<LootItem, 200> loot;
		InternalState gameState2;

		void unscramble();
	};

	// For STATES.0x (main quest lock and trigger states).
	struct MQLevelState
	{
		struct LevelHashTable
		{
			uint8_t triggerCount;
			std::array<Trigger, 64> triggers;
			uint8_t lockCount;
			std::array<Lock, 64> locks;
		};

		// Index is (((provinceID * 2) + localDungeonID) * 4) + level.
		std::array<LevelHashTable, 64> hashTables;
	};

	// For SPELLS.0x and SPELLSG.0x.
	struct SpellData
	{
		// To do: get from existing SpellData in MiscAssets.
	};

	// For INN.0x.
	struct Tavern
	{
		uint16_t remainingHours;
		uint32_t timeLimit;
	};

	// For AUTOMAP.0x.
	struct Automap
	{
		// Fog of war caches.
		struct FogOfWarCache
		{
			struct Note
			{
				uint16_t x, y;
				std::array<char, 60> text;
			};

			uint32_t levelHash;
			std::array<Note, 64> notes;
			std::array<uint8_t, 4096> bitmap; // 2 bits per block.
		};

		std::array<FogOfWarCache, 16> caches;
	};

	// For LOG.0x.
	struct Log
	{
		// To do.
	};

	static Automap loadAutomap(int index);
	static Log loadLog(int index);
	static SaveEngine loadSaveEngn(int index);
	static SaveGame loadSaveGame(int index);
	static SpellData loadSpells(int index);
	static SpellData loadSpellsg(int index);
	static MQLevelState loadState(int index);
	// To do: load city data.
	// To do: load INN.0x.
	// To do: load wild 001, 002, 003, 004.
};

#endif
