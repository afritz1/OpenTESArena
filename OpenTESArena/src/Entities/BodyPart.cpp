#include <unordered_map>

#include "BodyPart.h"
#include "BodyPartName.h"
#include "../Items/ArmorType.h"

const std::unordered_map<BodyPartName, std::string> BodyPartDisplayNames =
{
	{ BodyPartName::Head, "Head" },
	{ BodyPartName::LeftShoulder, "Left Shoulder" },
	{ BodyPartName::RightShoulder, "Right Shoulder" },
	{ BodyPartName::Chest, "Chest" },
	{ BodyPartName::Hands, "Hands" },
	{ BodyPartName::Legs, "Legs" },
	{ BodyPartName::Feet, "Feet" }
};

const std::unordered_map<BodyPartName, ArmorType> BodyPartArmors =
{
	{ BodyPartName::Head, ArmorType::Helm },
	{ BodyPartName::LeftShoulder, ArmorType::LeftPauldron },
	{ BodyPartName::RightShoulder, ArmorType::RightPauldron },
	{ BodyPartName::Chest, ArmorType::Cuirass },
	{ BodyPartName::Hands, ArmorType::Gauntlets },
	{ BodyPartName::Legs, ArmorType::Greaves },
	{ BodyPartName::Feet, ArmorType::Boots }
};

ArmorType BodyPart::getArmorType(BodyPartName partName)
{
	ArmorType armorType = BodyPartArmors.at(partName);
	return armorType;
}

const std::string &BodyPart::toString(BodyPartName partName)
{
	const std::string &displayName = BodyPartDisplayNames.at(partName);
	return displayName;
}
