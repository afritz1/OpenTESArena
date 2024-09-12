#include <unordered_map>

#include "Accessory.h"
#include "AccessoryArtifactData.h"
#include "AccessoryType.h"
#include "ItemType.h"
#include "Metal.h"
#include "MetalType.h"

#include "components/debug/Debug.h"

const std::unordered_map<AccessoryType, std::string> AccessoryDisplayNames =
{
	{ AccessoryType::Amulet, "Amulet" },
	{ AccessoryType::Belt, "Belt" },
	{ AccessoryType::Bracelet, "Bracelet" },
	{ AccessoryType::Bracers, "Bracers" },
	{ AccessoryType::Ring, "Ring" },
	{ AccessoryType::Torc, "Torc" }
};

// These values are made up. I don't know if accessories have a weight. For a moment,
// I was confused as to why bracers are an accessory. I guess they aren't an armor
// because they don't have a particular body part (arms?). Just let it be "anonymous".
const std::unordered_map<AccessoryType, double> AccessoryWeights =
{
	{ AccessoryType::Amulet, 0.20 },
	{ AccessoryType::Belt, 0.50 },
	{ AccessoryType::Bracelet, 0.15 },
	{ AccessoryType::Bracers, 1.5 },
	{ AccessoryType::Ring, 0.05 },
	{ AccessoryType::Torc, 0.25 }
};

// These values are made up, and are based on iron.
const std::unordered_map<AccessoryType, int> AccessoryGoldValues =
{
	{ AccessoryType::Amulet, 150 },
	{ AccessoryType::Belt, 30 },
	{ AccessoryType::Bracelet, 80 },
	{ AccessoryType::Bracers, 100 },
	{ AccessoryType::Ring, 125 },
	{ AccessoryType::Torc, 145 }
};

// Max allowed number of accessories per slot. The original game says only one
// bracelet and one ring, but I think it would make sense to have a bracelet
// "on each arm", and at least two rings realistically.
const std::unordered_map<AccessoryType, int> AccessoryMaxEquipCounts =
{
	{ AccessoryType::Amulet, 1 },
	{ AccessoryType::Belt, 1 },
	{ AccessoryType::Bracelet, 2 },
	{ AccessoryType::Bracers, 1 },
	{ AccessoryType::Ring, 2 },
	{ AccessoryType::Torc, 1 }
};

Accessory::Accessory(AccessoryType accessoryType, MetalType metalType,
	const AccessoryArtifactData *artifactData)
	: Item(artifactData), Metallic(metalType)
{
	this->accessoryType = accessoryType;
}

Accessory::Accessory(AccessoryType accessoryType, MetalType metalType)
	: Accessory(accessoryType, metalType, nullptr) { }

Accessory::Accessory(const AccessoryArtifactData *artifactData)
	: Accessory(artifactData->getAccessoryType(), artifactData->getMetalType(),
		artifactData) { }

std::unique_ptr<Item> Accessory::clone() const
{
	return std::make_unique<Accessory>(
		this->getAccessoryType(), this->getMetal().getMetalType(),
		static_cast<const AccessoryArtifactData*>(this->getArtifactData())); // @todo: unsafe cast, can't use dynamic_cast anymore
}

ItemType Accessory::getItemType() const
{
	return ItemType::Accessory;
}

double Accessory::getWeight() const
{
	auto weight = AccessoryWeights.at(this->getAccessoryType());
	DebugAssert(weight >= 0.0);
	return weight;
}

int Accessory::getGoldValue() const
{
	int baseValue = AccessoryGoldValues.at(this->getAccessoryType());
	return baseValue;
}

std::string Accessory::getDisplayName() const
{
	auto displayName = (this->getArtifactData() != nullptr) ?
		this->getArtifactData()->getDisplayName() :
		AccessoryDisplayNames.at(this->getAccessoryType());
	return displayName;
}

AccessoryType Accessory::getAccessoryType() const
{
	return this->accessoryType;
}

int Accessory::getMaxEquipCount() const
{
	int maxCount = AccessoryMaxEquipCounts.at(this->getAccessoryType());
	return maxCount;
}
