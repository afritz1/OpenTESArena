#include <cassert>
#include <map>

#include "Accessory.h"
#include "AccessoryArtifactData.h"
#include "ItemType.h"
#include "Metal.h"
#include "MetalType.h"

const auto AccessoryDisplayNames = std::map<AccessoryType, std::string>
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
const auto AccessoryWeights = std::map<AccessoryType, double>
{
	{ AccessoryType::Amulet, 0.20 },
	{ AccessoryType::Belt, 0.50 },
	{ AccessoryType::Bracelet, 0.15 },
	{ AccessoryType::Bracers, 1.5 },
	{ AccessoryType::Ring, 0.05 },
	{ AccessoryType::Torc, 0.25 }
};

// These values are made up, and are based on iron.
const auto AccessoryGoldValues = std::map<AccessoryType, int>
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
const auto AccessoryMaxEquipCounts = std::map<AccessoryType, int>
{
	{ AccessoryType::Amulet, 1 },
	{ AccessoryType::Belt, 1 },
	{ AccessoryType::Bracelet, 2 },
	{ AccessoryType::Bracers, 1 },
	{ AccessoryType::Ring, 2 },
	{ AccessoryType::Torc, 1 }
};

Accessory::Accessory(AccessoryType accessoryType, MetalType metalType)
	: Metallic(metalType)
{
	this->artifactData = nullptr;
	this->accessoryType = accessoryType;

	assert(this->artifactData.get() == nullptr);
}

Accessory::Accessory(AccessoryArtifactName artifactName)
	: Metallic(AccessoryArtifactData(artifactName).getMetalType())
{
	this->artifactData = std::unique_ptr<AccessoryArtifactData>(
		new AccessoryArtifactData(artifactName));
	this->accessoryType = this->artifactData->getAccessoryType();

	assert(this->artifactData.get() != nullptr);
}

Accessory::Accessory(const Accessory &accessory)
	: Metallic(accessory.getMetal().getMetalType())
{
	this->artifactData = (accessory.artifactData == nullptr) ? nullptr :
		std::unique_ptr<AccessoryArtifactData>(
			new AccessoryArtifactData(*accessory.artifactData));
	this->accessoryType = accessory.accessoryType;

	// This assert makes sure they're logically equivalent.
	assert((!(this->artifactData != nullptr)) ^ (accessory.artifactData != nullptr));
}

Accessory::~Accessory()
{

}

ItemType Accessory::getItemType() const
{
	return ItemType::Accessory;
}

double Accessory::getWeight() const
{
	auto weight = AccessoryWeights.at(this->getAccessoryType());
	assert(weight >= 0.0);
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
	assert(displayName.size() > 0);
	return displayName;
}

const AccessoryArtifactData *Accessory::getArtifactData() const
{
	return this->artifactData.get();
}

const AccessoryType &Accessory::getAccessoryType() const
{
	return this->accessoryType;
}

int Accessory::getMaxEquipCount() const
{
	int maxCount = AccessoryMaxEquipCounts.at(this->getAccessoryType());
	return maxCount;
}
