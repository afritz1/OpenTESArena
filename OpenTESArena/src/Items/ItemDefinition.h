#ifndef ITEM_DEFINITION_H
#define ITEM_DEFINITION_H

#include <cstdint>

#include "components/utilities/Enum.h"
#include "components/utilities/Span.h"

using ItemConditionDefinitionID = int;

struct ItemConditionDefinition
{
	char name[64]; // New, lightly used, etc.
	int current, max, degradeRate;

	ItemConditionDefinition();

	void init(const char *name, int current, int max, int degradeRate);
};

using ItemMaterialDefinitionID = int;

struct ItemMaterialDefinition
{
	char name[64]; // Iron, steel, etc.
	int ratingMultiplier;
	int conditionMultiplier;
	int weightMultiplier;

	ItemMaterialDefinition();

	void init(const char *name, int ratingMultiplier, int conditionMultiplier, int weightMultiplier);
};

struct AccessoryItemDefinition
{
	char name[64]; // Amulet, belt, etc..
	ItemMaterialDefinitionID materialDefID;

	void init(const char *name, ItemMaterialDefinitionID materialDefID);
};

enum class ArmorMaterialType
{
	Leather,
	Chain,
	Plate // Requires item material.
};

struct ArmorItemDefinition
{
	char name[64]; // Helmet, left pauldron, etc..
	double weight;
	ArmorMaterialType materialType;
	ItemMaterialDefinitionID plateMaterialDefID;

	void initLeather(const char *name, double weight);
	void initChain(const char *name, double weight);
	void initPlate(const char *name, double weight, ItemMaterialDefinitionID materialDefID);
};

struct ConsumableItemDefinition
{
	char name[64]; // "Potion of <effect>", etc.
	char unidentifiedName[64];
	// @todo: effect def ID?

	void init(const char *name, const char *unidentifiedName);
};

struct GoldItemDefinition
{
	char nameSingular[64]; // ... gold piece (used with loot containers).
	char namePlural[64]; // Bag of ... gold (used with loot containers).

	void init(const char *nameSingular, const char *namePlural);
};

struct MiscItemDefinition
{
	char name[64]; // Book, key, staff piece, etc.

	void init(const char *name);
};

struct ShieldItemDefinition
{
	char name[64]; // Buckler, kite, etc.
	double weight;

	void init(const char *name, double weight);
};

struct TrinketItemDefinition
{
	char name[64]; // Crystal, mark, etc.

	void init(const char *name);
};

struct WeaponItemDefinition
{
	char name[64]; // Dagger, longsword, etc.
	double weight;
	int basePrice;
	int damageMin;
	int damageMax;
	int handCount;
	bool isRanged;
	ItemMaterialDefinitionID materialDefID;

	void initMelee(const char *name, double weight, int basePrice, int damageMin, int damageMax, int handCount, ItemMaterialDefinitionID materialDefID);
	void initRanged(const char *name, double weight, int basePrice, int damageMin, int damageMax, ItemMaterialDefinitionID materialDefID);
};

struct ArtifactItemDefinition
{
	char flavorText[1024];	
	int provinceIDs[8];
	int provinceCount;

	ArtifactItemDefinition();

	void init(const char *flavorText, Span<const int> provinceIDs);
};

enum class ItemType : uint32_t
{
	Accessory = 1 << 0,
	Armor = 1 << 1,
	Consumable = 1 << 2,
	Gold = 1 << 3,
	Misc = 1 << 4,
	Shield = 1 << 5,
	Trinket = 1 << 6,
	Weapon = 1 << 7
};

AllowEnumFlags(ItemType);
using ItemTypeFlags = EnumFlags<ItemType>; // For random item picking from library.

using ItemDefinitionID = int;

struct ItemDefinition
{
	ItemType type;

	union
	{
		AccessoryItemDefinition accessory;
		ArmorItemDefinition armor;
		ConsumableItemDefinition consumable;
		GoldItemDefinition gold;
		MiscItemDefinition misc;
		ShieldItemDefinition shield;
		TrinketItemDefinition trinket;
		WeaponItemDefinition weapon;
	};

	int originalItemID; // For the weapon/armor ID lookup the original game does.
	bool isArtifact;
	ArtifactItemDefinition artifact;

	ItemDefinition();

	void init(ItemType type, int originalItemID);

	std::string getDisplayName(int stackAmount) const;
	double getWeight() const;
};

#endif
