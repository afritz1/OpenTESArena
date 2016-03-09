#include <cassert>
#include <iostream>
#include <map>

#include "BodyArmor.h"
#include "ArmorType.h"
#include "ArmorMaterial.h"
#include "BodyArmorArtifactData.h"
#include "../Entities/BodyPart.h"
#include "../Entities/BodyPartName.h"

// This uses a body part name as the mapping instead of an armor type because the
// "Shield" entry would never be used, since it's overridden by the Shield class.
const auto BodyArmorDisplayNames = std::map<BodyPartName, std::string>
{
	{ BodyPartName::Head, "Helm" },
	{ BodyPartName::LeftShoulder, "Left Pauldron" },
	{ BodyPartName::RightShoulder, "Right Pauldron" },
	{ BodyPartName::Chest, "Cuirass" },
	{ BodyPartName::Hands, "Gauntlets" },
	{ BodyPartName::Legs, "Greaves" },
	{ BodyPartName::Feet, "Boots" }
};

// These weights are based on iron. They are made up and will need to be revised 
// to fit the game better.
const auto BodyArmorWeights = std::map<BodyPartName, double>
{
	{ BodyPartName::Head, 5.0 },
	{ BodyPartName::LeftShoulder, 6.0 },
	{ BodyPartName::RightShoulder, 6.0 },
	{ BodyPartName::Chest, 18.0 },
	{ BodyPartName::Hands, 4.0 },
	{ BodyPartName::Legs, 12.0 },
	{ BodyPartName::Feet, 8.0 }
};

// These values are based on iron. They are made up and will need to be revised 
// to fit the game better.
const auto BodyArmorGoldValues = std::map<BodyPartName, int>
{
	{ BodyPartName::Head, 25 },
	{ BodyPartName::LeftShoulder, 20 },
	{ BodyPartName::RightShoulder, 20 },
	{ BodyPartName::Chest, 50 },
	{ BodyPartName::Hands, 20 },
	{ BodyPartName::Legs, 30 },
	{ BodyPartName::Feet, 20 }
};

// Since it's BodyPartName -> ArmorType, no Shield dummy value needs to exist.
const auto BodyArmorPartProtections = std::map<BodyPartName, ArmorType>
{
	{ BodyPartName::Head, ArmorType::Helm },
	{ BodyPartName::LeftShoulder, ArmorType::LeftPauldron },
	{ BodyPartName::RightShoulder, ArmorType::RightPauldron },
	{ BodyPartName::Chest, ArmorType::Cuirass },
	{ BodyPartName::Hands, ArmorType::Gauntlets },
	{ BodyPartName::Legs, ArmorType::Greaves },
	{ BodyPartName::Feet, ArmorType::Boots }
};

BodyArmor::BodyArmor(BodyPartName partName, std::unique_ptr<ArmorMaterial> armorMaterial)
{
	this->artifactData = nullptr;
	this->armorMaterial = std::move(armorMaterial);
	this->part = std::unique_ptr<BodyPart>(new BodyPart(partName));

	assert(this->artifactData.get() == nullptr);
	assert(this->armorMaterial.get() != nullptr);
	assert(this->part.get() != nullptr);
}

BodyArmor::BodyArmor(BodyArmorArtifactName artifactName)
{
	this->artifactData = std::unique_ptr<BodyArmorArtifactData>(
		new BodyArmorArtifactData(artifactName));
	this->armorMaterial = this->artifactData->getArmorMaterial();
	this->part = std::unique_ptr<BodyPart>(new BodyPart(
		this->artifactData->getBodyPartName()));

	assert(this->artifactData.get() != nullptr);
	assert(this->armorMaterial.get() != nullptr);
	assert(this->part.get() != nullptr);
}

BodyArmor::BodyArmor(const BodyArmor &bodyArmor)
{
	this->artifactData = (bodyArmor.artifactData == nullptr) ? nullptr :
		std::unique_ptr<BodyArmorArtifactData>(
			new BodyArmorArtifactData(*bodyArmor.artifactData));
	this->armorMaterial = bodyArmor.armorMaterial->clone();
	this->part = std::unique_ptr<BodyPart>(new BodyPart(bodyArmor.part->getPartName()));

	// This assert makes sure they're logically equivalent.
	assert((!(this->artifactData != nullptr)) ^ (bodyArmor.artifactData != nullptr));

	assert(this->armorMaterial.get() != nullptr);
	assert(this->part.get() != nullptr);
}

BodyArmor::~BodyArmor()
{

}

double BodyArmor::getWeight() const
{
	auto baseWeight = BodyArmorWeights.at(this->getBodyPart().getPartName());
	auto materialMultiplier = this->getArmorMaterial()->getWeightMultiplier();
	auto weight = baseWeight * materialMultiplier;
	assert(weight >= 0.0);
	return weight;
}

int BodyArmor::getGoldValue() const
{
	// Refine this method.
	int baseValue = BodyArmorGoldValues.at(this->getBodyPart().getPartName());
	int ratingModifier = this->getArmorRating();
	auto metalMultiplier = this->getArmorMaterial()->getWeightMultiplier();
	int value = static_cast<int>(static_cast<double>(baseValue + ratingModifier) *
		metalMultiplier);
	return value;
}

std::string BodyArmor::getDisplayName() const
{
	auto displayName = (this->getArtifactData() != nullptr) ? 
		this->getArtifactData()->getDisplayName() :
		this->getArmorMaterial()->toString() + " " + this->typeToString();
	assert(displayName.size() > 0);
	return displayName;
}

const BodyArmorArtifactData *BodyArmor::getArtifactData() const
{
	return this->artifactData.get();
}

const BodyPart &BodyArmor::getBodyPart() const
{
	return *this->part;
}

std::string BodyArmor::typeToString() const
{
	auto displayName = BodyArmorDisplayNames.at(this->getBodyPart().getPartName());
	assert(displayName.size() > 0);
	return displayName;
}

ArmorType BodyArmor::getArmorType() const
{
	auto armorType = BodyArmorPartProtections.at(this->getBodyPart().getPartName());
	return armorType;
}

const ArmorMaterial *BodyArmor::getArmorMaterial() const
{
	assert(this->armorMaterial.get() != nullptr);

	return this->armorMaterial.get();
}

std::vector<BodyPartName> BodyArmor::getProtectedBodyParts() const
{
	auto partNames = std::vector<BodyPartName>();
	partNames.push_back(this->getBodyPart().getPartName());

	// Body armors only protect one body part, unlike shields.
	assert(partNames.size() == 1);

	return partNames;
}

int BodyArmor::getArmorRating() const
{
	// I think body armor ratings are actually just bound to the material in the original.
	// That makes okay sense, because with the (supposedly) weighted hit model with the
	// chest being hit the most, having uniformly distributed armor ratings for each kind
	// of piece seems okay. 

	// Otherwise, if all body parts had equal likelihood of being hit, then each piece of
	// armor should have different ratings, with the chest having the highest of something
	// like 6 or 7 while gauntlets only get like, say, 2.

	// The armor rating model is bound on a by-body-part basis, not a total pool.

	// I won't make a mapping, then. It'll just depend on the material.
	return this->getArmorMaterial()->getArmorRating();
}
