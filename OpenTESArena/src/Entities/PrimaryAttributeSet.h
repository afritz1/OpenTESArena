#ifndef PRIMARY_ATTRIBUTE_SET_H
#define PRIMARY_ATTRIBUTE_SET_H

#include <map>

#include "PrimaryAttribute.h"
#include "PrimaryAttributeName.h"

class PrimaryAttributeSet
{
private:
	std::map<PrimaryAttributeName, PrimaryAttribute> attributeMap;
public:
	PrimaryAttributeSet(int raceID, bool male, Random &random); // Rolls new values based on race & gender.

	const PrimaryAttribute get(PrimaryAttributeName name) const;
	const std::vector<PrimaryAttribute> getAll() const;
	const int getValue(PrimaryAttributeName name) const;
	const PrimaryAttribute getStrength() const;
	const int getStrengthValue() const;
	const PrimaryAttribute getIntelligence() const;
	const int getIntelligenceValue() const;
	const PrimaryAttribute getWillpower() const;
	const int getWillpowerValue() const;
	const PrimaryAttribute getAgility() const;
	const int getAgilityValue() const;
	const PrimaryAttribute getSpeed() const;
	const int getSpeedValue() const;
	const PrimaryAttribute getEndurance() const;
	const int getEnduranceValue() const;
	const PrimaryAttribute getPersonality() const;
	const int getPersonalityValue() const;
	const PrimaryAttribute getLuck() const;
	const int getLuckValue() const;
};

#endif
