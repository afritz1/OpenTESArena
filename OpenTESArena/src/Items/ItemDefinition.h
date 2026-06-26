#pragma once

#include <cstdint>

#include "../Assets/ArenaTypes.h"
#include "../Player/WeaponAnimation.h"
#include "../Spells/SpellDefinition.h"
#include "../Stats/PrimaryAttribute.h"

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
	char name[64]; // "<Material> amulet, belt, etc." or "Amulet, belt, etc. of <attribute>"
	ArenaAccessoryTypeID typeID;
	char unidentifiedName[64]; // Amulet, belt, etc..
	ItemMaterialDefinitionID materialDefID;
	int basePrice;
	PrimaryAttributeID attributeID;

	void init(const char *name, ArenaAccessoryTypeID typeID, const char *unidentifiedName, ItemMaterialDefinitionID materialDefID, PrimaryAttributeID attributeID, int basePrice);
};

static constexpr int ARMOR_MATERIAL_TYPE_COUNT = static_cast<int>(ArenaArmorMaterialType::Leather) + 1;

struct ArmorItemDefinition
{
	char name[64]; // Helmet, left pauldron, etc..
	ArenaArmorTypeID typeID;
	double weight;
	ArenaArmorMaterialType materialType;
	ItemMaterialDefinitionID plateMaterialDefID;

	void initLeather(const char *name, ArenaArmorTypeID typeID, double weight);
	void initChain(const char *name, ArenaArmorTypeID typeID, double weight);
	void initPlate(const char *name, ArenaArmorTypeID typeID, double weight, ItemMaterialDefinitionID materialDefID);
};

struct ConsumableItemDefinition
{
	char name[64]; // "Potion of <effect>", etc.
	ArenaConsumableTypeID typeID;
	char unidentifiedName[64];
	// @todo: effect def ID?

	void init(const char *name, ArenaConsumableTypeID typeID, const char *unidentifiedName);
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
	ArenaMiscTypeID typeID;

	void init(const char *name, ArenaMiscTypeID typeID);
};

struct ShieldItemDefinition
{
	char name[64]; // Buckler, kite, etc.
	ArenaArmorTypeID armorTypeID; // Shield type ID + 7. The original game treats shields as armor.
	double weight;

	void init(const char *name, ArenaArmorTypeID armorTypeID, double weight);
};

struct TrinketItemDefinition
{
	char name[64]; // "Crystal, mark, etc. of <spell>"
	ArenaTrinketTypeID typeID;
	char unidentifiedName[64]; // Crystal, mark, etc.
	SpellID spellID;

	void init(const char *name, ArenaTrinketTypeID typeID, const char *unidentifiedName, SpellID spellID);
};

struct WeaponItemDefinition
{
	char name[64]; // Dagger, longsword, etc.
	ArenaWeaponTypeID typeID;
	double weight;
	int basePrice;
	int damageMin;
	int damageMax;
	int handCount;
	bool isRanged;
	ItemMaterialDefinitionID materialDefID;
	WeaponAnimationDefinitionID weaponAnimDefID;

	void initMelee(const char *name, ArenaWeaponTypeID typeID, double weight, int basePrice, int damageMin, int damageMax, int handCount, ItemMaterialDefinitionID materialDefID, WeaponAnimationDefinitionID weaponAnimDefID);
	void initRanged(const char *name, ArenaWeaponTypeID typeID, double weight, int basePrice, int damageMin, int damageMax, ItemMaterialDefinitionID materialDefID, WeaponAnimationDefinitionID weaponAnimDefID);
};

struct ArtifactItemDefinition
{
	char flavorText[1024];	
	int provinceIDs[8];
	int provinceCount;

	ArtifactItemDefinition();

	void init(const char *flavorText, Span<const int> provinceIDs);
};

enum class ItemType
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

	bool isStackable;
	bool isArtifact;
	ArtifactItemDefinition artifact;

	ItemDefinition();

	void init(ItemType type);

	std::string getDisplayName(int stackAmount) const;
	double getWeight() const;
};
