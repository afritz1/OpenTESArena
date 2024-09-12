#include <unordered_map>

#include "ArmorMaterial.h"
#include "ArmorType.h"
#include "Metal.h"
#include "Shield.h"
#include "ShieldArtifactData.h"
#include "ShieldType.h"
#include "../Entities/BodyPart.h"
#include "../Entities/BodyPartName.h"

#include "components/debug/Debug.h"

const std::unordered_map<ShieldType, std::string> ShieldTypeDisplayNames =
{
	{ ShieldType::Buckler, "Buckler" },
	{ ShieldType::Round, "Round Shield" },
	{ ShieldType::Kite, "Kite Shield" },
	{ ShieldType::Tower, "Tower Shield" }
};

// Using positive armor ratings here. Negate them for 2nd edition rules.
const std::unordered_map<ShieldType, int> ShieldRatings =
{
	{ ShieldType::Buckler, 1 },
	{ ShieldType::Round, 2 },
	{ ShieldType::Kite, 3 },
	{ ShieldType::Tower, 4 }
};

// These numbers are based on iron. They are made up and will probably be revised 
// at some point.
const std::unordered_map<ShieldType, double> ShieldWeights =
{
	{ ShieldType::Buckler, 5.0 },
	{ ShieldType::Round, 6.0 },
	{ ShieldType::Kite, 8.0 },
	{ ShieldType::Tower, 12.0 }
};

// These numbers are based on iron. They are made up and will probably be revised 
// at some point.
const std::unordered_map<ShieldType, int> ShieldGoldValues =
{
	{ ShieldType::Buckler, 20 },
	{ ShieldType::Round, 30 },
	{ ShieldType::Kite, 45 },
	{ ShieldType::Tower, 60 }
};

// Shields protect multiple body parts, unlike regular body armor pieces.
const std::unordered_map<ShieldType, std::vector<BodyPartName>> ShieldProtectedBodyParts =
{
	{ ShieldType::Buckler, { BodyPartName::Hands, BodyPartName::LeftShoulder } },
	{ ShieldType::Round, { BodyPartName::Hands, BodyPartName::LeftShoulder } },
	{ ShieldType::Kite, { BodyPartName::Hands, BodyPartName::LeftShoulder, BodyPartName::Legs } },
	{ ShieldType::Tower, { BodyPartName::Chest, BodyPartName::Hands, BodyPartName::Head,
	BodyPartName::LeftShoulder, BodyPartName::Legs } }
};

Shield::Shield(ShieldType shieldType, MetalType metalType,
	const ShieldArtifactData *artifactData)
	: Armor(artifactData)
{
	this->armorMaterial = std::make_unique<HeavyArmorMaterial>(metalType);
	this->shieldType = shieldType;
}

Shield::Shield(ShieldType shieldType, MetalType metalType)
	: Shield(shieldType, metalType, nullptr) { }

Shield::Shield(const ShieldArtifactData *artifactData)
	: Shield(artifactData->getShieldType(), artifactData->getMetalType(),
		artifactData) { }

std::unique_ptr<Item> Shield::clone() const
{
	return std::make_unique<Shield>(this->getShieldType(),
		this->armorMaterial->getMetal().getMetalType(),
		static_cast<const ShieldArtifactData*>(this->getArtifactData())); // @todo: unsafe cast, can't use dynamic_cast anymore
}

double Shield::getWeight() const
{
	double baseWeight = ShieldWeights.at(this->getShieldType());
	double metalMultiplier = this->getArmorMaterial()->getWeightMultiplier();
	double weight = baseWeight * metalMultiplier;
	DebugAssert(weight >= 0.0);
	return weight;
}

int Shield::getGoldValue() const
{
	// Refine this method sometime.
	int baseValue = ShieldGoldValues.at(this->getShieldType());
	int ratingModifier = this->getArmorRating();
	double metalMultiplier = this->getArmorMaterial()->getWeightMultiplier();
	int value = static_cast<int>(static_cast<double>(baseValue + ratingModifier) * 
		metalMultiplier);
	return value;
}

std::string Shield::getDisplayName() const
{
	return this->getArmorMaterial()->toString() + " " + this->typeToString();
}

ShieldType Shield::getShieldType() const
{
	return this->shieldType;
}

std::string Shield::typeToString() const
{
	auto displayName = ShieldTypeDisplayNames.at(this->getShieldType());
	return displayName;
}

ArmorType Shield::getArmorType() const
{
	return ArmorType::Shield;
}

const ArmorMaterial *Shield::getArmorMaterial() const
{
	return static_cast<ArmorMaterial*>(this->armorMaterial.get());
}

std::vector<BodyPartName> Shield::getProtectedBodyParts() const
{
	auto partNames = ShieldProtectedBodyParts.at(this->getShieldType());
	return partNames;
}

int Shield::getArmorRating() const
{
	int rating = ShieldRatings.at(this->getShieldType());
	return rating;
}
