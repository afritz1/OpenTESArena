#ifndef BODY_PART_H
#define BODY_PART_H

#include <string>

#include "BodyPartName.h"

class BodyPart
{
private:
	BodyPartName partName;
public:
	BodyPart(BodyPartName partName);
	~BodyPart();

	const BodyPartName &getPartName() const;
	std::string toString() const;
};

#endif