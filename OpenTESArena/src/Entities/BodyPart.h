#ifndef BODY_PART_H
#define BODY_PART_H

#include <string>

#include "BodyPartName.h"

enum class ArmorType;

class BodyPart
{
private:
	BodyPartName partName;
public:
	BodyPart(BodyPartName partName);
	~BodyPart();

	BodyPartName getPartName() const;
	ArmorType getArmorType() const;
	std::string toString() const;
};

#endif
