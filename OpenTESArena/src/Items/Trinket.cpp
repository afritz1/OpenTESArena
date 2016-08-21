#include <cassert>
#include <map>

#include "Trinket.h"

#include "ItemType.h"
#include "TrinketType.h"

const std::map<TrinketType, std::string> TrinketDisplayNames =
{
	{ TrinketType::Crystal, "Crystal" },
	{ TrinketType::Mark, "Mark" }
};

// These values are made up and should be revised sometime.
const std::map<TrinketType, double> TrinketWeights =
{
	{ TrinketType::Crystal, 0.25 },
	{ TrinketType::Mark, 0.20 }
};

// These values are made up and should be revised sometime.
const std::map<TrinketType, int> TrinketGoldValues =
{
	{ TrinketType::Crystal, 100 },
	{ TrinketType::Mark, 80 }
};

const std::map<TrinketType, int> TrinketMaxEquipCounts =
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
	double weight = TrinketWeights.at(this->getTrinketType());
	assert(weight >= 0.0);
	return weight;
}

int32_t Trinket::getGoldValue() const
{
	int32_t baseValue = TrinketGoldValues.at(this->getTrinketType());
	return baseValue;
}

std::string Trinket::getDisplayName() const
{
	auto displayName = TrinketDisplayNames.at(this->getTrinketType());
	return displayName;
}

TrinketType Trinket::getTrinketType() const
{
	return this->trinketType;
}

int32_t Trinket::getMaxEquipCount() const
{
	int32_t maxCount = TrinketMaxEquipCounts.at(this->getTrinketType());
	return maxCount;
}
