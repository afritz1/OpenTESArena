#include <algorithm>
#include <cassert>
#include <map>

#include "ItemCondition.h"
#include "ItemConditionName.h"
#include "ArmorMaterial.h"
#include "../Entities/BodyPartName.h"
#include "Metal.h"
#include "ShieldType.h"
#include "WeaponType.h"
#include "../Math/Random.h"

// It doesn't look like values for item conditions are visible anywhere, like in
// the manual for instance, so I'm remaking them anew with reasonable values.

const auto ItemConditionDisplayNames = std::map<ItemConditionName, std::string>
{
	{ ItemConditionName::New, "New" },
	{ ItemConditionName::AlmostNew, "Almost New" },
	{ ItemConditionName::SlightlyUsed, "Slightly Used" },
	{ ItemConditionName::Used, "Used" },
	{ ItemConditionName::Worn, "Worn" },
	{ ItemConditionName::Battered, "Battered" },
	{ ItemConditionName::Broken, "Broken" }
};

// Theoretically, a very poor piece of armor will break in only a couple hundred
// hits, while a very nice piece of armor will last thousands of hits.
const auto ItemConditionArmorMaxConditions = std::map<BodyPartName, int>
{
	{ BodyPartName::Head, 300 },
	{ BodyPartName::RightShoulder, 320 },
	{ BodyPartName::LeftShoulder, 320 },
	{ BodyPartName::Chest, 550 },
	{ BodyPartName::Hands, 300 },
	{ BodyPartName::Legs, 350 },
	{ BodyPartName::Feet, 320 }
};

const auto ItemConditionShieldMaxConditions = std::map<ShieldType, int>
{
	{ ShieldType::Buckler, 400 },
	{ ShieldType::Round, 600 },
	{ ShieldType::Kite, 700 },
	{ ShieldType::Tower, 800 }
};

// The relative number of strikes a weapon can make before breaking, before
// any material multipliers.
const auto ItemConditionWeaponMaxConditions = std::map<WeaponType, int>
{
	{ WeaponType::BattleAxe, 600 },
	{ WeaponType::Broadsword, 500 },
	{ WeaponType::Claymore, 550 },
	{ WeaponType::Dagger, 350 },
	{ WeaponType::DaiKatana, 550 },
	{ WeaponType::Fists, 1 },
	{ WeaponType::Flail, 500 },
	{ WeaponType::Katana, 500 },
	{ WeaponType::LongBow, 500 },
	{ WeaponType::Longsword, 500 },
	{ WeaponType::Mace, 525 },
	{ WeaponType::Saber, 500 },
	{ WeaponType::ShortBow, 450 },
	{ WeaponType::Shortsword, 450 },
	{ WeaponType::Staff, 450 },
	{ WeaponType::Tanto, 350 },
	{ WeaponType::Wakizashi, 400 },
	{ WeaponType::WarAxe, 500 },
	{ WeaponType::Warhammer, 600 }
};

// Rates for how quick each item receives degradation per "degrade". The majority
// of these rates will be 1, since that's how the max conditions are designed,
// though fists cannot degrade at all, so their rate is 0.
const auto ItemConditionArmorDegradeRates = std::map<BodyPartName, int>
{
	{ BodyPartName::Head, 1 },
	{ BodyPartName::RightShoulder, 1 },
	{ BodyPartName::LeftShoulder, 1 },
	{ BodyPartName::Chest, 1 },
	{ BodyPartName::Hands, 1 },
	{ BodyPartName::Legs, 1 },
	{ BodyPartName::Feet, 1 }
};

const auto ItemConditionShieldDegradeRates = std::map<ShieldType, int>
{
	{ ShieldType::Buckler, 1 },
	{ ShieldType::Round, 1 },
	{ ShieldType::Kite, 1 },
	{ ShieldType::Tower, 1 }
};

const auto ItemConditionWeaponDegradeRates = std::map<WeaponType, int>
{
	{ WeaponType::BattleAxe, 1 },
	{ WeaponType::Broadsword, 1 },
	{ WeaponType::Claymore, 1 },
	{ WeaponType::Dagger, 1 },
	{ WeaponType::DaiKatana, 1 },
	{ WeaponType::Fists, 0 }, // Fists have zero degrade.
	{ WeaponType::Flail, 1 },
	{ WeaponType::Katana, 1 },
	{ WeaponType::LongBow, 1 },
	{ WeaponType::Longsword, 1 },
	{ WeaponType::Mace, 1 },
	{ WeaponType::Saber, 1 },
	{ WeaponType::ShortBow, 1 },
	{ WeaponType::Shortsword, 1 },
	{ WeaponType::Staff, 1 },
	{ WeaponType::Tanto, 1 },
	{ WeaponType::Wakizashi, 1 },
	{ WeaponType::WarAxe, 1 },
	{ WeaponType::Warhammer, 1 }
};

ItemCondition::ItemCondition(BodyPartName partName, const ArmorMaterial &material)
{
	int maxArmorCondition = ItemConditionArmorMaxConditions.at(partName);

	// I rolled the material multiplier (leather, chain, plate) in with the metal 
	// multiplier (iron, steel, etc.), so no special type data is needed.
	int materialMultiplier = material.getConditionMultiplier();

	this->maxCondition = maxArmorCondition * materialMultiplier;
	this->currentCondition = this->maxCondition;
	this->degradeRate = ItemConditionArmorDegradeRates.at(partName);

	assert(this->maxCondition > 0);
	assert(this->currentCondition == this->maxCondition);
	assert(this->degradeRate >= 0);
}

ItemCondition::ItemCondition(ShieldType shieldType, const Metal &metal)
{
	int maxShieldCondition = ItemConditionShieldMaxConditions.at(shieldType);
	int metalMultiplier = metal.getConditionMultiplier();

	this->maxCondition = maxShieldCondition * metalMultiplier;
	this->currentCondition = this->maxCondition;
	this->degradeRate = ItemConditionShieldDegradeRates.at(shieldType);

	assert(this->maxCondition > 0);
	assert(this->currentCondition == this->maxCondition);
	assert(this->degradeRate >= 0);
}

ItemCondition::ItemCondition(WeaponType weaponType, const Metal &metal)
{
	int maxWeaponCondition = ItemConditionWeaponMaxConditions.at(weaponType);
	int metalMultiplier = metal.getConditionMultiplier();

	this->maxCondition = maxWeaponCondition * metalMultiplier;
	this->currentCondition = this->maxCondition;
	this->degradeRate = ItemConditionWeaponDegradeRates.at(weaponType);

	assert(this->maxCondition > 0);
	assert(this->currentCondition == this->maxCondition);
	assert(this->degradeRate >= 0);
}

ItemCondition::ItemCondition()
{
	auto weaponType = WeaponType::Fists;
	this->maxCondition = ItemConditionWeaponMaxConditions.at(weaponType);
	this->currentCondition = this->maxCondition;
	this->degradeRate = ItemConditionWeaponDegradeRates.at(weaponType);

	assert(this->maxCondition > 0);
	assert(this->currentCondition == this->maxCondition);
	assert(this->degradeRate >= 0);
}

ItemCondition::~ItemCondition()
{

}

ItemConditionName ItemCondition::getCurrentConditionName() const
{
	assert(this->maxCondition > 0);

	double percent = static_cast<double>(this->currentCondition) /
		static_cast<double>(this->maxCondition);

	// The condition ranges don't need to be uniformly distributed. We can give
	// the player a bit more leeway when the item condition is critical.
	if (percent > 0.90)
	{
		return ItemConditionName::New;
	}
	else if (percent > 0.75)
	{
		return ItemConditionName::AlmostNew;
	}
	else if (percent > 0.60)
	{
		return ItemConditionName::SlightlyUsed;
	}
	else if (percent > 0.50)
	{
		return ItemConditionName::Used;
	}
	else if (percent > 0.35)
	{
		return ItemConditionName::Worn;
	}
	else if (percent > 0.0)
	{
		return ItemConditionName::Battered;
	}
	else
	{
		return ItemConditionName::Broken;
	}
}

bool ItemCondition::isBroken() const
{
	return this->getCurrentConditionName() == ItemConditionName::Broken;
}

void ItemCondition::repairFully()
{
	this->currentCondition = this->maxCondition;
}

void ItemCondition::repairSlightly()
{
	// Randomly repair a little bit.
	auto random = Random();
	this->currentCondition += random.next(this->degradeRate + 1);

	// Make sure the incremented condition is not greater than the max.
	this->currentCondition = std::min(this->currentCondition, this->maxCondition);

	assert(this->currentCondition <= this->maxCondition);
}

void ItemCondition::degrade()
{
	this->currentCondition -= this->degradeRate;
}
