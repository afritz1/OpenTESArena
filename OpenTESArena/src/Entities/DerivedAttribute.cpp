#include <algorithm>
#include <unordered_map>

#include "DerivedAttribute.h"
#include "DerivedAttributeName.h"
#include "../Utilities/Debug.h"

const std::unordered_map<DerivedAttributeName, std::string> DerivedAttributeDisplayNames =
{
	{ DerivedAttributeName::Health, "Health" },
	{ DerivedAttributeName::SpellPoints, "Spell Points" },
	{ DerivedAttributeName::Stamina, "Stamina" }
};

DerivedAttribute::DerivedAttribute(DerivedAttributeName attributeName, int baseMaximum)
{
	DebugAssert(baseMaximum > 0);

	this->attributeName = attributeName;
	this->maximum = baseMaximum;
	this->current = this->maximum;
}

int DerivedAttribute::getCurrent() const
{
	return std::min(this->current, this->getMaximum());
}

int DerivedAttribute::getMaximum() const
{
	DebugAssert(this->maximum > 0);

	return this->maximum;
}

DerivedAttributeName DerivedAttribute::getAttributeName() const
{
	return this->attributeName;
}

std::string DerivedAttribute::toString() const
{
	auto displayName = DerivedAttributeDisplayNames.at(this->getAttributeName());
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
	DebugAssert(value > 0);

	this->maximum = value;
}
