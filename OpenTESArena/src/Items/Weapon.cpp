#include <cassert>
#include <map>

#include "Weapon.h"
#include "ItemType.h"
#include "Metal.h"
#include "WeaponArtifactData.h"
#include "WeaponArtifactName.h"
#include "WeaponHandCount.h"
#include "WeaponRangeName.h"

// The manual mentions chain and plate gauntlets, but I don't think those are really
// in the game. It says they have weights too, but that doesn't make sense because
// stats like that depend on the armor pieces themselves. I'll just roll both of them
// into "fists". Maybe I'll add a damage modifier to gauntlets, but don't count on it.
const auto WeaponTypeDisplayNames = std::map<WeaponType, std::string>
{
	{ WeaponType::BattleAxe, "Battle Axe" },
	{ WeaponType::Broadsword, "Broadsword" },
	{ WeaponType::Claymore, "Claymore" },
	{ WeaponType::Dagger, "Dagger" },
	{ WeaponType::DaiKatana, "Dai-Katana" },
	{ WeaponType::Fists, "Fists" },
	{ WeaponType::Flail, "Flail" },
	{ WeaponType::Katana, "Katana" },
	{ WeaponType::LongBow, "Long Bow" },
	{ WeaponType::Longsword, "Longsword" },
	{ WeaponType::Mace, "Mace" },
	{ WeaponType::Saber, "Saber" },
	{ WeaponType::ShortBow, "Short Bow" },
	{ WeaponType::Shortsword, "Shortsword" },
	{ WeaponType::Staff, "Staff" },
	{ WeaponType::Tanto, "Tanto" },
	{ WeaponType::Wakizashi, "Wakizashi" },
	{ WeaponType::WarAxe, "War Axe" },
	{ WeaponType::Warhammer, "Warhammer" }
};

const auto WeaponTypeHandCounts = std::map<WeaponType, WeaponHandCount>
{
	{ WeaponType::BattleAxe, WeaponHandCount::Two },
	{ WeaponType::Broadsword, WeaponHandCount::One },
	{ WeaponType::Claymore, WeaponHandCount::Two },
	{ WeaponType::Dagger, WeaponHandCount::One },
	{ WeaponType::DaiKatana, WeaponHandCount::Two },
	{ WeaponType::Fists, WeaponHandCount::One },
	{ WeaponType::Flail, WeaponHandCount::Two },
	{ WeaponType::Katana, WeaponHandCount::One },
	{ WeaponType::LongBow, WeaponHandCount::Two },
	{ WeaponType::Longsword, WeaponHandCount::One },
	{ WeaponType::Mace, WeaponHandCount::One },
	{ WeaponType::Saber, WeaponHandCount::One },
	{ WeaponType::ShortBow, WeaponHandCount::Two },
	{ WeaponType::Shortsword, WeaponHandCount::One },
	{ WeaponType::Staff, WeaponHandCount::Two },
	{ WeaponType::Tanto, WeaponHandCount::One },
	{ WeaponType::Wakizashi, WeaponHandCount::One },
	{ WeaponType::WarAxe, WeaponHandCount::Two },
	{ WeaponType::Warhammer, WeaponHandCount::Two }
};

const auto WeaponTypeBaseDamages = std::map<WeaponType, std::pair<int, int>>
{
	{ WeaponType::BattleAxe, std::pair<int, int>(2, 16) },
	{ WeaponType::Broadsword, std::pair<int, int>(1, 12) },
	{ WeaponType::Claymore, std::pair<int, int>(2, 18) },
	{ WeaponType::Dagger, std::pair<int, int>(1, 6) },
	{ WeaponType::DaiKatana, std::pair<int, int>(3, 21) },
	{ WeaponType::Fists, std::pair<int, int>(1, 2) },
	{ WeaponType::Flail, std::pair<int, int>(2, 14) },
	{ WeaponType::Katana, std::pair<int, int>(3, 16) },
	{ WeaponType::LongBow, std::pair<int, int>(2, 12) },
	{ WeaponType::Longsword, std::pair<int, int>(2, 16) },
	{ WeaponType::Mace, std::pair<int, int>(1, 12) },
	{ WeaponType::Saber, std::pair<int, int>(3, 12) },
	{ WeaponType::ShortBow, std::pair<int, int>(2, 8) },
	{ WeaponType::Shortsword, std::pair<int, int>(1, 8) },
	{ WeaponType::Staff, std::pair<int, int>(1, 8) },
	{ WeaponType::Tanto, std::pair<int, int>(1, 8) },
	{ WeaponType::Wakizashi, std::pair<int, int>(1, 10) },
	{ WeaponType::WarAxe, std::pair<int, int>(2, 12) },
	{ WeaponType::Warhammer, std::pair<int, int>(3, 18) }
};

// Weights in kilograms. For some odd reason, the manual lists "fists" as having
// a weight of 1 kilogram. Are they implying the weight of flesh...? Are fists a 
// hidden inventory item...? Anyway, I decided to make its weight zero since that 
// makes more sense.
const auto WeaponTypeWeights = std::map<WeaponType, double>
{
	{ WeaponType::BattleAxe, 12.0 },
	{ WeaponType::Broadsword, 6.0 },
	{ WeaponType::Claymore, 14.0 },
	{ WeaponType::Dagger, 0.5 },
	{ WeaponType::DaiKatana, 8.0 },
	{ WeaponType::Fists, 0.0 },
	{ WeaponType::Flail, 10.0 },
	{ WeaponType::Katana, 6.0 },
	{ WeaponType::LongBow, 1.0 },
	{ WeaponType::Longsword, 8.0 },
	{ WeaponType::Mace, 8.0 },
	{ WeaponType::Saber, 7.0 },
	{ WeaponType::ShortBow, 0.5 },
	{ WeaponType::Shortsword, 3.0 },
	{ WeaponType::Staff, 3.0 },
	{ WeaponType::Tanto, 0.5 },
	{ WeaponType::Wakizashi, 2.0 },
	{ WeaponType::WarAxe, 8.0 },
	{ WeaponType::Warhammer, 16.0 }
};

// These values are based on iron. They are made up, and should be revised at
// some point.
const auto WeaponTypeGoldValues = std::map<WeaponType, int>
{
	{ WeaponType::BattleAxe, 35 },
	{ WeaponType::Broadsword, 20 },
	{ WeaponType::Claymore, 30 },
	{ WeaponType::Dagger, 10 },
	{ WeaponType::DaiKatana, 30 },
	{ WeaponType::Fists, 0 },
	{ WeaponType::Flail, 20 },
	{ WeaponType::Katana, 25 },
	{ WeaponType::LongBow, 20 },
	{ WeaponType::Longsword, 20 },
	{ WeaponType::Mace, 20 },
	{ WeaponType::Saber, 20 },
	{ WeaponType::ShortBow, 15 },
	{ WeaponType::Shortsword, 15 },
	{ WeaponType::Staff, 15 },
	{ WeaponType::Tanto, 10 },
	{ WeaponType::Wakizashi, 15 },
	{ WeaponType::WarAxe, 20 },
	{ WeaponType::Warhammer, 35 }
};

const auto WeaponTypeRangeNames = std::map<WeaponType, WeaponRangeName>
{
	{ WeaponType::BattleAxe, WeaponRangeName::Melee },
	{ WeaponType::Broadsword, WeaponRangeName::Melee },
	{ WeaponType::Claymore, WeaponRangeName::Melee },
	{ WeaponType::Dagger, WeaponRangeName::Melee },
	{ WeaponType::DaiKatana, WeaponRangeName::Melee },
	{ WeaponType::Fists, WeaponRangeName::Melee },
	{ WeaponType::Flail, WeaponRangeName::Melee },
	{ WeaponType::Katana, WeaponRangeName::Melee },
	{ WeaponType::LongBow, WeaponRangeName::Ranged },
	{ WeaponType::Longsword, WeaponRangeName::Melee },
	{ WeaponType::Mace, WeaponRangeName::Melee },
	{ WeaponType::Saber, WeaponRangeName::Melee },
	{ WeaponType::ShortBow, WeaponRangeName::Ranged },
	{ WeaponType::Shortsword, WeaponRangeName::Melee },
	{ WeaponType::Staff, WeaponRangeName::Melee },
	{ WeaponType::Tanto, WeaponRangeName::Melee },
	{ WeaponType::Wakizashi, WeaponRangeName::Melee },
	{ WeaponType::WarAxe, WeaponRangeName::Melee },
	{ WeaponType::Warhammer, WeaponRangeName::Melee }
};

Weapon::Weapon(WeaponType weaponType, MetalType metalType)
	: Metallic(metalType)
{
	this->artifactData = nullptr;
	this->weaponType = weaponType;

	assert(this->artifactData.get() == nullptr);
}

Weapon::Weapon(WeaponArtifactName artifactName)
	: Metallic(WeaponArtifactData(artifactName).getMetalType())
{
	this->artifactData = std::unique_ptr<WeaponArtifactData>(
		new WeaponArtifactData(artifactName));
	this->weaponType = this->artifactData->getWeaponType();

	assert(this->artifactData.get() != nullptr);
}

Weapon::Weapon(const Weapon &weapon)
	: Metallic(weapon.getMetal().getMetalType())
{
	this->artifactData = (weapon.getArtifactData() == nullptr) ? nullptr :
		std::unique_ptr<WeaponArtifactData>(new WeaponArtifactData(
			weapon.getArtifactData()->getArtifactName()));
	this->weaponType = weapon.getWeaponType();

	// This assert makes sure they're logically equivalent.
	assert((!(this->artifactData != nullptr)) ^ (weapon.artifactData != nullptr));
}

Weapon::~Weapon()
{

}

ItemType Weapon::getItemType() const
{
	return ItemType::Weapon;
}

double Weapon::getWeight() const
{
	auto baseWeight = WeaponTypeWeights.at(this->getWeaponType());
	auto metalMultiplier = this->getMetal().getWeightMultiplier();
	auto weight = baseWeight * metalMultiplier;
	assert(weight >= 0.0);
	return weight;
}

int Weapon::getGoldValue() const
{
	// Refine this method.
	int baseValue = WeaponTypeGoldValues.at(this->getWeaponType());
	int ratingMultiplier = this->getMetal().getRatingModifier();
	auto weightMultiplier = this->getMetal().getWeightMultiplier();
	auto goldValue = static_cast<int>(static_cast<double>(baseValue + ratingMultiplier) * 
		weightMultiplier);
	return goldValue;
}

std::string Weapon::getDisplayName() const
{
	return this->getMetal().toString() + " " + this->typeToString();
}

const WeaponArtifactData *Weapon::getArtifactData() const
{
	return this->artifactData.get();
}

const WeaponType &Weapon::getWeaponType() const
{
	return this->weaponType;
}

WeaponHandCount Weapon::getHandCount() const
{
	auto handCount = WeaponTypeHandCounts.at(this->getWeaponType());
	return handCount;
}

int Weapon::getBaseMinDamage() const
{
	int minDamage = WeaponTypeBaseDamages.at(this->getWeaponType()).first;
	return minDamage;
}

int Weapon::getBaseMaxDamage() const
{
	int maxDamage = WeaponTypeBaseDamages.at(this->getWeaponType()).second;
	return maxDamage;
}

WeaponRangeName Weapon::getWeaponRangeName() const
{
	auto rangeName = WeaponTypeRangeNames.at(this->getWeaponType());
	return rangeName;
}

std::string Weapon::typeToString() const
{
	auto displayName = WeaponTypeDisplayNames.at(this->getWeaponType());
	assert(displayName.size() > 0);
	return displayName;
}
