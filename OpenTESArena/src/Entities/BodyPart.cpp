#include <cassert>
#include <map>

#include "BodyPart.h"

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

std::string BodyPart::toString() const
{
	auto displayName = BodyPartDisplayNames.at(this->getPartName());
	assert(displayName.size() > 0);
	return displayName;
}
