#include <algorithm>
#include <unordered_map>

#include "ArmorMaterial.h"
#include "ItemCondition.h"
#include "ItemConditionName.h"
#include "Metal.h"
#include "ShieldType.h"
#include "../Entities/BodyPartName.h"
#include "../Math/Random.h"

#include "components/debug/Debug.h"

// It doesn't look like item condition values are visible anywhere, like in
// the manual for instance, so I'm remaking them anew with reasonable values.
// They are of course placeholders.

const std::unordered_map<ItemConditionName, std::string> ItemConditionDisplayNames =
{
	{ ItemConditionName::New, "New" },
	{ ItemConditionName::AlmostNew, "Almost New" },
	{ ItemConditionName::SlightlyUsed, "Slightly Used" },
	{ ItemConditionName::Used, "Used" },
	{ ItemConditionName::Worn, "Worn" },
	{ ItemConditionName::Battered, "Battered" },
	{ ItemConditionName::Broken, "Broken" }
};

ItemCondition ItemCondition::makeArmorCondition(BodyPartName partName, const ArmorMaterial &material)
{
	// @todo: use values from original game.
	ItemCondition itemCondition;

	const int maxArmorCondition = 1;

	// I rolled the material multiplier (leather, chain, plate) in with the metal 
	// multiplier (iron, steel, etc.), so no special type data is needed.
	const int materialMultiplier = material.getConditionMultiplier();

	itemCondition.maxCondition = maxArmorCondition * materialMultiplier;
	itemCondition.currentCondition = itemCondition.maxCondition;
	itemCondition.degradeRate = 1;
	return itemCondition;
}

ItemCondition ItemCondition::makeShieldCondition(ShieldType shieldType, const Metal &metal)
{
	// @todo: use values from original game.
	ItemCondition itemCondition;

	const int maxShieldCondition = 1;
	const int metalMultiplier = metal.getConditionMultiplier();

	itemCondition.maxCondition = maxShieldCondition * metalMultiplier;
	itemCondition.currentCondition = itemCondition.maxCondition;
	itemCondition.degradeRate = 1;
	return itemCondition;
}

ItemCondition ItemCondition::makeWeaponCondition(int weaponID, const Metal &metal)
{
	// @todo: use values from original game.
	ItemCondition itemCondition;

	const int maxWeaponCondition = 1;
	const int metalMultiplier = metal.getConditionMultiplier();

	itemCondition.maxCondition = maxWeaponCondition * metalMultiplier;
	itemCondition.currentCondition = itemCondition.maxCondition;
	itemCondition.degradeRate = 1;
	return itemCondition;
}

ItemCondition ItemCondition::makeFistsCondition()
{
	ItemCondition itemCondition;
	itemCondition.maxCondition = 1;
	itemCondition.currentCondition = itemCondition.maxCondition;
	itemCondition.degradeRate = 0;
	return itemCondition;
}

ItemConditionName ItemCondition::getCurrentConditionName() const
{
	DebugAssert(this->maxCondition > 0);

	const double percent = static_cast<double>(this->currentCondition) /
		static_cast<double>(this->maxCondition);

	// Placeholder condition ranges.
	// - @todo: get actual condition ranges.
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

void ItemCondition::repairSlightly(Random &random)
{
	// The calling function needs to make sure there's some delay between auto-
	// repairs, so that higher frame rates don't cause faster repairs!

	// Randomly repair a little bit.
	this->currentCondition += random.next(this->degradeRate + 1);

	// Make sure the incremented condition is not greater than the max.
	this->currentCondition = std::min(this->currentCondition, this->maxCondition);
}

void ItemCondition::degrade()
{
	this->currentCondition -= this->degradeRate;
}
