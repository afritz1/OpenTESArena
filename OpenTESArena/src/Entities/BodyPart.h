#ifndef BODY_PART_H
#define BODY_PART_H

#include <string>

enum class ArmorType;
enum class BodyPartName;

// Namespace for obtaining the associated armor type and display name for a particular body part.
namespace BodyPart
{
	ArmorType getArmorType(BodyPartName partName);
	const std::string &toString(BodyPartName partName);
}

#endif
