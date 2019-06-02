#include <unordered_map>

#include "Metal.h"
#include "MetalType.h"

const std::unordered_map<MetalType, std::string> MetalTypeDisplayNames =
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
const std::unordered_map<MetalType, int> MetalRatingModifiers =
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
const std::unordered_map<MetalType, int> MetalConditionMultipliers =
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
const std::unordered_map<MetalType, double> MetalWeightMultipliers =
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

MetalType Metal::getMetalType() const
{
	return this->metalType;
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
	double multiplier = MetalWeightMultipliers.at(this->getMetalType());
	return multiplier;
}

std::string Metal::toString() const
{
	auto displayName = MetalTypeDisplayNames.at(this->getMetalType());
	return displayName;
}
