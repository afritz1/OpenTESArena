#ifndef BODY_PART_H
#define BODY_PART_H

#include <string>

// Namespace for obtaining the associated armor type and display name for 
// a particular body part.

enum class ArmorType;
enum class BodyPartName;

namespace BodyPart
{
	ArmorType getArmorType(BodyPartName partName);
	const std::string &toString(BodyPartName partName);
}

#endif
