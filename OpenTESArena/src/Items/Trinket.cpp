#include <cassert>
#include <map>

#include "Trinket.h"

#include "ItemType.h"

const auto TrinketDisplayNames = std::map<TrinketType, std::string>
{
	{ TrinketType::Crystal, "Crystal" },
	{ TrinketType::Mark, "Mark" }
};

// These values are made up and should be revised sometime.
const auto TrinketWeights = std::map<TrinketType, double>
{
	{ TrinketType::Crystal, 0.25 },
	{ TrinketType::Mark, 0.20 }
};

// These values are made up and should be revised sometime.
const auto TrinketGoldValues = std::map<TrinketType, int>
{
	{ TrinketType::Crystal, 100 },
	{ TrinketType::Mark, 80 }
};

const auto TrinketMaxEquipCounts = std::map<TrinketType, int>
{
	{ TrinketType::Crystal, 1 },
	{ TrinketType::Mark, 1 }
};

Trinket::Trinket(TrinketType trinketType)
	: Item(nullptr)
{
	this->trinketType = trinketType;
}

Trinket::~Trinket()
{

}

ItemType Trinket::getItemType() const
{
	return ItemType::Trinket;
}

double Trinket::getWeight() const
{
	auto weight = TrinketWeights.at(this->getTrinketType());
	assert(weight >= 0.0);
	return weight;
}

int Trinket::getGoldValue() const
{
	int baseValue = TrinketGoldValues.at(this->getTrinketType());
	return baseValue;
}

std::string Trinket::getDisplayName() const
{
	auto displayName = TrinketDisplayNames.at(this->getTrinketType());
	assert(displayName.size() > 0);
	return displayName;
}

const TrinketType &Trinket::getTrinketType() const
{
	return this->trinketType;
}

int Trinket::getMaxEquipCount() const
{
	int maxCount = TrinketMaxEquipCounts.at(this->getTrinketType());
	return maxCount;
}
