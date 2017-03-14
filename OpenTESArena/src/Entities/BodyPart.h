#ifndef BODY_PART_H
#define BODY_PART_H

#include <string>

// Static class for obtaining the associated armor type and display name for 
// a particular body part.

enum class ArmorType;
enum class BodyPartName;

class BodyPart
{
private:
	BodyPart() = delete;
	~BodyPart() = delete;
public:
	static ArmorType getArmorType(BodyPartName partName);
	static const std::string &toString(BodyPartName partName);
};

#endif
