#include <cassert>
#include <map>

#include "Metal.h"

const auto MetalTypeDisplayNames = std::map<MetalType, std::string>
{
	{ MetalType::Iron, "Iron" },
	{ MetalType::Steel, "Steel" },
	{ MetalType::Silver, "Silver" },
	{ MetalType::Elven, "Elven" },
	{ MetalType::Dwarven, "Dwarven" },
	{ MetalType::Mithril, "Mithril" },
	{ MetalType::Adamantium, "Adamantium" },
	{ MetalType::Ebony, "Ebony" }
};

// Negate the modifiers for armor rating if the 2nd edition D&D rules are being used.
const auto MetalRatingModifiers = std::map<MetalType, int>
{
	{ MetalType::Iron, -1 },
	{ MetalType::Steel, 0 },
	{ MetalType::Silver, 0 },
	{ MetalType::Elven, 1 },
	{ MetalType::Dwarven, 2 },
	{ MetalType::Mithril, 3 },
	{ MetalType::Adamantium, 4 },
	{ MetalType::Ebony, 5 }
};

// Multiplier for extra metal strength. From this table, an ebony item lasts
// three times longer than its equivalent iron, steel, or silver item.
const auto MetalConditionMultipliers = std::map<MetalType, int>
{
	{ MetalType::Iron, 1 },
	{ MetalType::Steel, 1 },
	{ MetalType::Silver, 1 },
	{ MetalType::Elven, 2 },
	{ MetalType::Dwarven, 2 },
	{ MetalType::Mithril, 2 },
	{ MetalType::Adamantium, 3 },
	{ MetalType::Ebony, 3 }
};

// Multipliers for changing weight based on the type of metal. These values are
// made up and are subject to change.
const auto MetalWeightMultipliers = std::map<MetalType, double>
{
	{ MetalType::Iron, 1.15 },
	{ MetalType::Steel, 1.0 },
	{ MetalType::Silver, 0.90 },
	{ MetalType::Elven, 0.75 },
	{ MetalType::Dwarven, 1.50 },
	{ MetalType::Mithril, 0.80 },
	{ MetalType::Adamantium, 1.50 },
	{ MetalType::Ebony, 2.0 }
};

Metal::Metal(MetalType metalType)
{
	this->metalType = metalType;
}

Metal::~Metal()
{

}

const MetalType &Metal::getMetalType() const
{
	return this->metalType;
}

std::string Metal::toString() const
{
	auto displayName = MetalTypeDisplayNames.at(this->getMetalType());
	assert(displayName.size() > 0);
	return displayName;
}

int Metal::getRatingModifier() const
{
	int modifier = MetalRatingModifiers.at(this->getMetalType());
	return modifier;
}

int Metal::getConditionMultiplier() const
{
	int multiplier = MetalConditionMultipliers.at(this->getMetalType());
	return multiplier;
}

double Metal::getWeightMultiplier() const
{
	auto multiplier = MetalWeightMultipliers.at(this->getMetalType());
	return multiplier;
}
