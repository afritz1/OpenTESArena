#ifndef ITEM_DEFINITION_H
#define ITEM_DEFINITION_H

#include "components/utilities/BufferView.h"

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
	ArmorMaterialType materialType;
	ItemMaterialDefinitionID plateMaterialDefID;

	void initLeather(const char *name);
	void initChain(const char *name);
	void initPlate(const char *name, ItemMaterialDefinitionID materialDefID);
};

struct ConsumableItemDefinition
{
	char name[64]; // Potion, etc.

	void init(const char *name);
};

struct MiscItemDefinition
{
	char name[64]; // Book, key, staff piece, etc.

	void init(const char *name);
};

struct ShieldItemDefinition
{
	char name[64]; // Buckler, kite, etc.

	void init(const char *name);
};

struct TrinketItemDefinition
{
	char name[64]; // Crystal, mark, etc.

	void init(const char *name);
};

struct WeaponItemDefinition
{
	char name[64]; // Dagger, longsword, etc.
	int handCount;
	bool isRanged;
	ItemMaterialDefinitionID materialDefID;

	void initMelee(const char *name, int handCount, ItemMaterialDefinitionID materialDefID);
	void initRanged(const char *name, ItemMaterialDefinitionID materialDefID);
};

struct ArtifactItemDefinition
{
	char flavorText[1024];	
	int provinceIDs[8];
	int provinceCount;

	ArtifactItemDefinition();

	void init(const char *flavorText, BufferView<const int> provinceIDs);
};

enum class ItemType
{
	Accessory,
	Armor,
	Consumable,
	Misc,
	Shield,
	Trinket,
	Weapon
};

using ItemDefinitionID = int;

struct ItemDefinition
{
	ItemType type;

	union
	{
		AccessoryItemDefinition accessory;
		ArmorItemDefinition armor;
		ConsumableItemDefinition consumable;
		MiscItemDefinition misc;
		ShieldItemDefinition shield;
		TrinketItemDefinition trinket;
		WeaponItemDefinition weapon;
	};

	bool isArtifact;
	ArtifactItemDefinition artifact;

	ItemDefinition();

	void init(ItemType type);
};

#endif
