#include <cassert>
#include <map>

#include "BodyPart.h"

#include "../Items/ArmorType.h"

const auto BodyPartDisplayNames = std::map<BodyPartName, std::string>
{
	{ BodyPartName::Head, "Head" },
	{ BodyPartName::LeftShoulder, "Left Shoulder" },
	{ BodyPartName::RightShoulder, "Right Shoulder" },
	{ BodyPartName::Chest, "Chest" },
	{ BodyPartName::Hands, "Hands" },
	{ BodyPartName::Legs, "Legs" },
	{ BodyPartName::Feet, "Feet" }
};

const auto BodyPartArmors = std::map<BodyPartName, ArmorType>
{
	{ BodyPartName::Head, ArmorType::Helm },
	{ BodyPartName::LeftShoulder, ArmorType::LeftPauldron },
	{ BodyPartName::RightShoulder, ArmorType::RightPauldron },
	{ BodyPartName::Chest, ArmorType::Cuirass },
	{ BodyPartName::Hands, ArmorType::Gauntlets },
	{ BodyPartName::Legs, ArmorType::Greaves },
	{ BodyPartName::Feet, ArmorType::Boots }
};

BodyPart::BodyPart(BodyPartName partName)
{
	this->partName = partName;
}

BodyPart::~BodyPart()
{

}

const BodyPartName &BodyPart::getPartName() const
{
	return this->partName;
}

ArmorType BodyPart::getArmorType() const
{
	auto armorType = BodyPartArmors.at(this->getPartName());
	return armorType;
}

std::string BodyPart::toString() const
{
	auto displayName = BodyPartDisplayNames.at(this->getPartName());
	assert(displayName.size() > 0);
	return displayName;
}
