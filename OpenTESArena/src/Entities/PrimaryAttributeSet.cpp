#include "PrimaryAttribute.h"
#include "PrimaryAttributeName.h"
#include "PrimaryAttributeSet.h"

PrimaryAttributeSet::PrimaryAttributeSet(int raceID, bool male, Random &random)
{
	for (const PrimaryAttributeName attributeName : PRIMARY_ATTRIBUTE_NAMES)
	{
		const PrimaryAttribute attribute = PrimaryAttribute(attributeName, raceID, male, random);
		this->attributeMap.insert({ attributeName, attribute });
	}
}

const PrimaryAttribute PrimaryAttributeSet::get(PrimaryAttributeName attributeName) const
{
	return this->attributeMap.at(attributeName);
}

const std::vector<PrimaryAttribute> PrimaryAttributeSet::getAll() const
{
	std::vector<PrimaryAttribute> attributes;

	for (const PrimaryAttributeName name : PRIMARY_ATTRIBUTE_NAMES)
	{
		const PrimaryAttribute attribute = this->get(name);
		attributes.push_back(attribute);
	}

	return attributes;
}

const int PrimaryAttributeSet::getValue(PrimaryAttributeName attributeName) const
{
	const PrimaryAttribute attribute = this->get(attributeName);
	return attribute.get();
}

const PrimaryAttribute PrimaryAttributeSet::getStrength() const
{
	return this->get(PrimaryAttributeName::Strength);
}

const int PrimaryAttributeSet::getStrengthValue() const
{
	return this->getValue(PrimaryAttributeName::Strength);
}

const PrimaryAttribute PrimaryAttributeSet::getIntelligence() const
{
	return this->get(PrimaryAttributeName::Intelligence);
}

const int PrimaryAttributeSet::getIntelligenceValue() const
{
	return this->getValue(PrimaryAttributeName::Intelligence);
}

const PrimaryAttribute PrimaryAttributeSet::getWillpower() const
{
	return this->get(PrimaryAttributeName::Willpower);
}

const int PrimaryAttributeSet::getWillpowerValue() const
{
	return this->getValue(PrimaryAttributeName::Willpower);
}

const PrimaryAttribute PrimaryAttributeSet::getAgility() const
{
	return this->get(PrimaryAttributeName::Agility);
}

const int PrimaryAttributeSet::getAgilityValue() const
{
	return this->getValue(PrimaryAttributeName::Agility);
}

const PrimaryAttribute PrimaryAttributeSet::getSpeed() const
{
	return this->get(PrimaryAttributeName::Speed);
}

const int PrimaryAttributeSet::getSpeedValue() const
{
	return this->getValue(PrimaryAttributeName::Speed);
}

const PrimaryAttribute PrimaryAttributeSet::getEndurance() const
{
	return this->get(PrimaryAttributeName::Endurance);
}

const int PrimaryAttributeSet::getEnduranceValue() const
{
	return this->getValue(PrimaryAttributeName::Endurance);
}

const PrimaryAttribute PrimaryAttributeSet::getPersonality() const
{
	return this->get(PrimaryAttributeName::Personality);
}

const int PrimaryAttributeSet::getPersonalityValue() const
{
	return this->getValue(PrimaryAttributeName::Personality);
}

const PrimaryAttribute PrimaryAttributeSet::getLuck() const
{
	return this->get(PrimaryAttributeName::Luck);
}

const int PrimaryAttributeSet::getLuckValue() const
{
	return this->getValue(PrimaryAttributeName::Luck);
}
