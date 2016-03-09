#include <cassert>
#include <map>

#include "Trinket.h"
#include "ItemType.h"

const auto TrinketDisplayNames = std::map<TrinketName, std::string>
{
	{ TrinketName::Crystal, "Crystal" },
	{ TrinketName::Mark, "Mark" }
};

// These values are made up and should be revised sometime.
const auto TrinketWeights = std::map<TrinketName, double>
{
	{ TrinketName::Crystal, 0.25 },
	{ TrinketName::Mark, 0.20 }
};

// These values are made up and should be revised sometime.
const auto TrinketGoldValues = std::map<TrinketName, int>
{
	{ TrinketName::Crystal, 100 },
	{ TrinketName::Mark, 80 }
};

const auto TrinketMaxEquipCounts = std::map<TrinketName, int>
{
	{ TrinketName::Crystal, 1 },
	{ TrinketName::Mark, 1 }
};

Trinket::Trinket(TrinketName trinketName)
{
	this->trinketName = trinketName;
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
	auto weight = TrinketWeights.at(this->getTrinketName());
	assert(weight >= 0.0);
	return weight;
}

int Trinket::getGoldValue() const
{
	int baseValue = TrinketGoldValues.at(this->getTrinketName());
	return baseValue;
}

std::string Trinket::getDisplayName() const
{
	auto displayName = TrinketDisplayNames.at(this->getTrinketName());
	assert(displayName.size() > 0);
	return displayName;
}

const TrinketName &Trinket::getTrinketName() const
{
	return this->trinketName;
}

int Trinket::getMaxEquipCount() const
{
	int maxCount = TrinketMaxEquipCounts.at(this->getTrinketName());
	return maxCount;
}
