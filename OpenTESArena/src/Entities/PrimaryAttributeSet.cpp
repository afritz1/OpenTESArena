#include "PrimaryAttribute.h"
#include "PrimaryAttributeName.h"
#include "PrimaryAttributeSet.h"

PrimaryAttributeSet::PrimaryAttributeSet() { }

void PrimaryAttributeSet::init(int raceID, bool male, Random &random)
{
	for (const PrimaryAttributeName attributeName : PRIMARY_ATTRIBUTE_NAMES)
	{
		PrimaryAttribute attribute(attributeName, raceID, male, random);
		this->attributeMap.emplace(attributeName, std::move(attribute));
	}
}

const PrimaryAttribute &PrimaryAttributeSet::get(PrimaryAttributeName attributeName) const
{
	return this->attributeMap.at(attributeName);
}

std::vector<PrimaryAttribute> PrimaryAttributeSet::getAll() const
{
	std::vector<PrimaryAttribute> attributes;

	for (const PrimaryAttributeName name : PRIMARY_ATTRIBUTE_NAMES)
	{
		const PrimaryAttribute &attribute = this->get(name);
		attributes.emplace_back(attribute);
	}

	return attributes;
}

int PrimaryAttributeSet::getValue(PrimaryAttributeName attributeName) const
{
	const PrimaryAttribute &attribute = this->get(attributeName);
	return attribute.get();
}

const PrimaryAttribute &PrimaryAttributeSet::getStrength() const
{
	return this->get(PrimaryAttributeName::Strength);
}

const PrimaryAttribute &PrimaryAttributeSet::getIntelligence() const
{
	return this->get(PrimaryAttributeName::Intelligence);
}

const PrimaryAttribute &PrimaryAttributeSet::getWillpower() const
{
	return this->get(PrimaryAttributeName::Willpower);
}

const PrimaryAttribute &PrimaryAttributeSet::getAgility() const
{
	return this->get(PrimaryAttributeName::Agility);
}

const PrimaryAttribute &PrimaryAttributeSet::getSpeed() const
{
	return this->get(PrimaryAttributeName::Speed);
}

const PrimaryAttribute &PrimaryAttributeSet::getEndurance() const
{
	return this->get(PrimaryAttributeName::Endurance);
}

const PrimaryAttribute &PrimaryAttributeSet::getPersonality() const
{
	return this->get(PrimaryAttributeName::Personality);
}

const PrimaryAttribute &PrimaryAttributeSet::getLuck() const
{
	return this->get(PrimaryAttributeName::Luck);
}

void PrimaryAttributeSet::clear()
{
	this->attributeMap.clear();
}
