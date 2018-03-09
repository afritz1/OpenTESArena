#include <cassert>
#include <unordered_map>

#include "ArmorType.h"
#include "ArmorMaterial.h"
#include "BodyArmor.h"
#include "BodyArmorArtifactData.h"
#include "../Entities/BodyPart.h"
#include "../Entities/BodyPartName.h"

namespace std
{
	// Hash specialization, required until GCC 6.1.
	template <>
	struct hash<BodyPartName>
	{
		size_t operator()(const BodyPartName &x) const
		{
			return static_cast<size_t>(x);
		}
	};
}

// This uses a body part name as the mapping instead of an armor type because the
// "Shield" entry would never be used, since it's overridden by the Shield class.
const std::unordered_map<BodyPartName, std::string> BodyArmorDisplayNames =
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
const std::unordered_map<BodyPartName, double> BodyArmorWeights =
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
const std::unordered_map<BodyPartName, int> BodyArmorGoldValues =
{
	{ BodyPartName::Head, 25 },
	{ BodyPartName::LeftShoulder, 20 },
	{ BodyPartName::RightShoulder, 20 },
	{ BodyPartName::Chest, 50 },
	{ BodyPartName::Hands, 20 },
	{ BodyPartName::Legs, 30 },
	{ BodyPartName::Feet, 20 }
};

BodyArmor::BodyArmor(BodyPartName partName, const ArmorMaterial *armorMaterial,
	const BodyArmorArtifactData *artifactData)
	: Armor(artifactData)
{
	this->armorMaterial = armorMaterial->clone();
	this->partName = partName;
}

BodyArmor::BodyArmor(BodyPartName partName, const ArmorMaterial *armorMaterial)
	: BodyArmor(partName, armorMaterial, nullptr) { }

BodyArmor::BodyArmor(const BodyArmorArtifactData *artifactData)
	: BodyArmor(artifactData->getBodyPartName(), artifactData->getArmorMaterial(),
		artifactData) { }

BodyArmor::~BodyArmor()
{

}

std::unique_ptr<Item> BodyArmor::clone() const
{
	return std::make_unique<BodyArmor>(
		this->getPartName(), this->getArmorMaterial(),
		dynamic_cast<const BodyArmorArtifactData*>(this->getArtifactData()));
}

double BodyArmor::getWeight() const
{
	double baseWeight = BodyArmorWeights.at(this->getPartName());
	double materialMultiplier = this->getArmorMaterial()->getWeightMultiplier();
	double weight = baseWeight * materialMultiplier;
	assert(weight >= 0.0);
	return weight;
}

int BodyArmor::getGoldValue() const
{
	// Refine this method.
	int baseValue = BodyArmorGoldValues.at(this->getPartName());
	int ratingModifier = this->getArmorRating();
	double metalMultiplier = this->getArmorMaterial()->getWeightMultiplier();
	int value = static_cast<int>(static_cast<double>(baseValue + ratingModifier) *
		metalMultiplier);
	return value;
}

std::string BodyArmor::getDisplayName() const
{
	auto displayName = (this->getArtifactData() != nullptr) ? 
		this->getArtifactData()->getDisplayName() :
		this->getArmorMaterial()->toString() + " " + this->typeToString();
	return displayName;
}

BodyPartName BodyArmor::getPartName() const
{
	return this->partName;
}

std::string BodyArmor::typeToString() const
{
	auto displayName = BodyArmorDisplayNames.at(this->getPartName());
	return displayName;
}

ArmorType BodyArmor::getArmorType() const
{
	ArmorType armorType = BodyPart::getArmorType(this->getPartName());
	return armorType;
}

const ArmorMaterial *BodyArmor::getArmorMaterial() const
{
	assert(this->armorMaterial.get() != nullptr);

	return this->armorMaterial.get();
}

std::vector<BodyPartName> BodyArmor::getProtectedBodyParts() const
{
	// Body armors only protect one body part, unlike shields. 
	// This returns a vector to retain the same interface with armors.
	std::vector<BodyPartName> partNames;
	partNames.push_back(this->getPartName());

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
