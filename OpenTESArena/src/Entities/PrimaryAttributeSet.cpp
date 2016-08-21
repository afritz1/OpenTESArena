#include <cassert>
#include <vector>

#include "PrimaryAttributeSet.h"

#include "PrimaryAttribute.h"
#include "PrimaryAttributeName.h"

PrimaryAttributeSet::PrimaryAttributeSet()
{
	// Initialize to empty. This isn't immediately constructed here because the
	// primary attributes each need the primary attribute name to go with them,
	// and the loop below does that cleanly (perhaps this is a poor design).
	this->primaryAttributes = std::map<PrimaryAttributeName, PrimaryAttribute>();

	// List of attributes to generate.
	const std::vector<PrimaryAttributeName> attributeNames =
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
	for (const auto attributeName : attributeNames)
	{
		PrimaryAttribute attribute(attributeName, 0);
		this->primaryAttributes.insert(std::make_pair(attributeName, attribute));
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
}
