#include <algorithm>
#include <cassert>
#include <map>

#include "DerivedAttribute.h"

const auto DerivedAttributeDisplayNames = std::map<DerivedAttributeName, std::string>
{
	{ DerivedAttributeName::Health, "Health" },
	{ DerivedAttributeName::SpellPoints, "Spell Points" },
	{ DerivedAttributeName::Stamina, "Stamina" }
};

DerivedAttribute::DerivedAttribute(DerivedAttributeName attributeName, int baseMaximum)
{
	this->attributeName = attributeName;
	this->maximum = baseMaximum;
	this->current = this->maximum;

	assert(this->maximum > 0);
	assert(this->current == this->maximum);
}

DerivedAttribute::~DerivedAttribute()
{

}

const int &DerivedAttribute::getCurrent() const
{
	return std::min(this->current, this->getMaximum());
}

const int &DerivedAttribute::getMaximum() const
{
	assert(this->maximum > 0);

	return this->maximum;
}

const DerivedAttributeName &DerivedAttribute::getAttributeName() const
{
	return this->attributeName;
}

std::string DerivedAttribute::toString() const
{
	auto displayName = DerivedAttributeDisplayNames.at(this->getAttributeName());
	assert(displayName.size() > 0);
	return displayName;
}

void DerivedAttribute::setCurrent(int value)
{
	this->current = value;
}

void DerivedAttribute::setMaximum(int value)
{
	// The maximum for a derived attribute shouldn't be set to zero. Set
	// the current value instead.
	assert(value > 0);

	this->maximum = value;
}
