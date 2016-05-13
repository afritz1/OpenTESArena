#include <cassert>
#include <vector>

#include "PrimaryAttributeSet.h"

#include "PrimaryAttribute.h"

PrimaryAttributeSet::PrimaryAttributeSet()
{
	// Initialize to empty.
	this->primaryAttributes = std::map<PrimaryAttributeName, PrimaryAttribute>();

	// List of attributes to generate.
	const auto attributeNames = std::vector<PrimaryAttributeName>
	{
		PrimaryAttributeName::Strength,
		PrimaryAttributeName::Intelligence,
		PrimaryAttributeName::Willpower,
		PrimaryAttributeName::Agility,
		PrimaryAttributeName::Speed,
		PrimaryAttributeName::Endurance,
		PrimaryAttributeName::Personality,
		PrimaryAttributeName::Luck
	};

	// Initialize each primary attribute to zero.
	for (const auto &attributeName : attributeNames)
	{
		auto attribute = PrimaryAttribute(attributeName, 0);
		this->primaryAttributes.insert(std::pair<PrimaryAttributeName, PrimaryAttribute>(
			attributeName, attribute));
	}

	assert(this->primaryAttributes.size() == attributeNames.size());
}

PrimaryAttributeSet::~PrimaryAttributeSet()
{

}

const PrimaryAttribute &PrimaryAttributeSet::get(PrimaryAttributeName attributeName) const
{
	const auto &attribute = this->primaryAttributes.at(attributeName);
	return attribute;
}

void PrimaryAttributeSet::set(PrimaryAttributeName attributeName, int value)
{
	auto &attribute = this->primaryAttributes.at(attributeName);

	// This method checks that the value is valid before setting it.
	attribute.set(value);

	assert(attribute.get() == value);
}
