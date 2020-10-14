#include <array>

#include "ItemType.h"
#include "Metal.h"
#include "Weapon.h"
#include "WeaponArtifactData.h"
#include "WeaponHandCount.h"
#include "WeaponRangeType.h"
#include "../Assets/ExeData.h"

#include "components/debug/Debug.h"

// @todo: most of this class could be in an asset library as plain old data, and any
// instance-related things (condition, artifact state, etc.) would be in this class.

namespace
{
	// @todo: obtain from executable data.
	const std::array<WeaponHandCount, 18> WeaponHandCounts =
	{
		WeaponHandCount::Two, // Staff
		WeaponHandCount::One, // Dagger
		WeaponHandCount::One, // Shortsword
		WeaponHandCount::One, // Broadsword
		WeaponHandCount::One, // Saber
		WeaponHandCount::One, // Longsword
		WeaponHandCount::Two, // Claymore
		WeaponHandCount::One, // Tanto
		WeaponHandCount::One, // Wakizashi
		WeaponHandCount::One, // Katana
		WeaponHandCount::Two, // Dai-katana
		WeaponHandCount::One, // Mace
		WeaponHandCount::Two, // Flail
		WeaponHandCount::Two, // War hammer
		WeaponHandCount::Two, // War axe
		WeaponHandCount::Two, // Battle axe
		WeaponHandCount::Two, // Short bow
		WeaponHandCount::Two // Long bow
	};

	// Base damage values from the manual. Fists are {1, 2}.
	// - @todo: obtain from executable data.
	const std::array<std::pair<int, int>, 18> WeaponBaseDamages =
	{
		std::make_pair(1, 8), // Staff
		std::make_pair(1, 6), // Dagger
		std::make_pair(1, 8), // Shortsword
		std::make_pair(1, 12), // Broadsword
		std::make_pair(3, 12), // Saber
		std::make_pair(2, 16), // Longsword
		std::make_pair(2, 18), // Claymore
		std::make_pair(1, 8), // Tanto
		std::make_pair(1, 10), // Wakizashi
		std::make_pair(3, 16), // Katana
		std::make_pair(3, 21), // Dai-katana
		std::make_pair(1, 12), // Mace
		std::make_pair(2, 14), // Flail
		std::make_pair(3, 18), // War hammer
		std::make_pair(2, 12), // War axe
		std::make_pair(2, 16), // Battle axe
		std::make_pair(2, 8), // Short bow
		std::make_pair(2, 12) // Long bow
	};

	// Weights in kilograms. For some odd reason, the manual lists fists as having
	// a weight of 1 kilogram (ignored here).
	// - @todo: obtain from executable data.
	const std::array<double, 18> WeaponWeights =
	{
		3.0, // Staff
		0.50, // Dagger
		3.0, // Shortsword
		6.0, // Broadsword
		7.0, // Saber
		8.0, // Longsword
		14.0, // Claymore
		0.50, // Tanto
		2.0, // Wakizashi
		6.0, // Katana
		8.0, // Dai-katana
		8.0, // Mace
		10.0, // Flail
		16.0, // War hammer
		8.0, // War axe
		12.0, // Battle axe
		0.50, // Short bow
		1.0 // Long bow
	};

	// Gold values.
	// - @todo: obtain from executable data.
	const std::array<int, 18> WeaponGoldValues =
	{
		1, // Staff
		1, // Dagger
		1, // Shortsword
		1, // Broadsword
		1, // Saber
		1, // Longsword
		1, // Claymore
		1, // Tanto
		1, // Wakizashi
		1, // Katana
		1, // Dai-katana
		1, // Mace
		1, // Flail
		1, // War hammer
		1, // War axe
		1, // Battle axe
		1, // Short bow
		1 // Long bow
	};

	const std::array<WeaponRangeType, 18> WeaponRangeTypes =
	{
		WeaponRangeType::Melee, // Staff
		WeaponRangeType::Melee, // Dagger
		WeaponRangeType::Melee, // Shortsword
		WeaponRangeType::Melee, // Broadsword
		WeaponRangeType::Melee, // Saber
		WeaponRangeType::Melee, // Longsword
		WeaponRangeType::Melee, // Claymore
		WeaponRangeType::Melee, // Tanto
		WeaponRangeType::Melee, // Wakizashi
		WeaponRangeType::Melee, // Katana
		WeaponRangeType::Melee, // Dai-katana
		WeaponRangeType::Melee, // Mace
		WeaponRangeType::Melee, // Flail
		WeaponRangeType::Melee, // War hammer
		WeaponRangeType::Melee, // War axe
		WeaponRangeType::Melee, // Battle axe
		WeaponRangeType::Ranged, // Short bow
		WeaponRangeType::Ranged // Long bow
	};
}

Weapon::Weapon(int weaponID, const std::string &weaponName, MetalType metalType,
	const WeaponArtifactData *artifactData)
	: Item(artifactData), Metallic(metalType), weaponName(weaponName)
{
	this->weaponID = weaponID;
}

Weapon::Weapon(int weaponID, MetalType metalType,
	const WeaponArtifactData *artifactData, const ExeData &exeData)
	: Item(artifactData), Metallic(metalType)
{
	// Fists are not allowed.
	DebugAssert(weaponID != -1);

	this->weaponID = weaponID;
	this->weaponName = exeData.equipment.weaponNames.at(weaponID);
}

Weapon::Weapon(int weaponID, MetalType metalType, const ExeData &exeData)
	: Weapon(weaponID, metalType, nullptr, exeData) { }

Weapon::Weapon(const WeaponArtifactData *artifactData, const ExeData &exeData)
	: Weapon(artifactData->getWeaponID(), artifactData->getMetalType(),
		artifactData, exeData) { }

std::unique_ptr<Item> Weapon::clone() const
{
	return std::make_unique<Weapon>(this->weaponID, this->weaponName,
		this->getMetal().getMetalType(), this->artifactData.get());
}

ItemType Weapon::getItemType() const
{
	return ItemType::Weapon;
}

double Weapon::getWeight() const
{
	const double baseWeight = WeaponWeights.at(this->weaponID);
	const double metalMultiplier = this->getMetal().getWeightMultiplier();
	const double weight = baseWeight * metalMultiplier;
	return weight;
}

int Weapon::getGoldValue() const
{
	// @todo: use values from original game.
	const int baseValue = WeaponGoldValues.at(this->weaponID);
	const int ratingMultiplier = this->getMetal().getRatingModifier();
	const double weightMultiplier = this->getMetal().getWeightMultiplier();
	const int goldValue = static_cast<int>(
		static_cast<double>(baseValue + ratingMultiplier) * weightMultiplier);
	return goldValue;
}

std::string Weapon::getDisplayName() const
{
	return this->getMetal().toString() + " " + this->weaponName;
}

int Weapon::getWeaponID() const
{
	return this->weaponID;
}

const std::string &Weapon::getWeaponName() const
{
	return this->weaponName;
}

WeaponHandCount Weapon::getHandCount() const
{
	const WeaponHandCount handCount = WeaponHandCounts.at(this->weaponID);
	return handCount;
}

WeaponRangeType Weapon::getWeaponRangeType() const
{
	const WeaponRangeType rangeType = WeaponRangeTypes.at(this->weaponID);
	return rangeType;
}

int Weapon::getBaseMinDamage() const
{
	const int minDamage = WeaponBaseDamages.at(this->weaponID).first;
	return minDamage;
}

int Weapon::getBaseMaxDamage() const
{
	const int maxDamage = WeaponBaseDamages.at(this->weaponID).second;
	return maxDamage;
}
