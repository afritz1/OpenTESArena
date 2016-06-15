#include <cassert>
#include <map>

#include "BodyPart.h"

#include "BodyPartName.h"
#include "../Items/ArmorType.h"

const std::map<BodyPartName, std::string> BodyPartDisplayNames =
{
	{ BodyPartName::Head, "Head" },
	{ BodyPartName::LeftShoulder, "Left Shoulder" },
	{ BodyPartName::RightShoulder, "Right Shoulder" },
	{ BodyPartName::Chest, "Chest" },
	{ BodyPartName::Hands, "Hands" },
	{ BodyPartName::Legs, "Legs" },
	{ BodyPartName::Feet, "Feet" }
};

const std::map<BodyPartName, ArmorType> BodyPartArmors =
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

BodyPartName BodyPart::getPartName() const
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
	return displayName;
}
