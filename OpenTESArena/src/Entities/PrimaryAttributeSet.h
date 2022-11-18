#ifndef PRIMARY_ATTRIBUTE_SET_H
#define PRIMARY_ATTRIBUTE_SET_H

#include <unordered_map>

#include "PrimaryAttribute.h"
#include "PrimaryAttributeName.h"

class PrimaryAttributeSet
{
private:
	std::unordered_map<PrimaryAttributeName, PrimaryAttribute> attributeMap;
public:
	PrimaryAttributeSet(int raceID, bool male, Random &random); // Rolls new values based on race & gender.

	const PrimaryAttribute &get(PrimaryAttributeName name) const;
	std::vector<PrimaryAttribute> getAll() const;
	int getValue(PrimaryAttributeName name) const;
	const PrimaryAttribute &getStrength() const;
	const PrimaryAttribute &getIntelligence() const;
	const PrimaryAttribute &getWillpower() const;
	const PrimaryAttribute &getAgility() const;
	const PrimaryAttribute &getSpeed() const;
	const PrimaryAttribute &getEndurance() const;
	const PrimaryAttribute &getPersonality() const;
	const PrimaryAttribute &getLuck() const;
};

#endif
