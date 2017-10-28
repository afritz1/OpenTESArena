#include <cassert>
#include <map>

#include "AttributeModifierName.h"
#include "PrimaryAttribute.h"
#include "PrimaryAttributeName.h"

const std::map<PrimaryAttributeName, std::string> PrimaryAttributeDisplayNames =
{
	{ PrimaryAttributeName::Strength, "Strength" },
	{ PrimaryAttributeName::Intelligence, "Intelligence" },
	{ PrimaryAttributeName::Willpower, "Willpower" },
	{ PrimaryAttributeName::Agility, "Agility" },
	{ PrimaryAttributeName::Speed, "Speed" },
	{ PrimaryAttributeName::Endurance, "Endurance" },
	{ PrimaryAttributeName::Personality, "Personality" },
	{ PrimaryAttributeName::Luck, "Luck" }
};

const std::map<PrimaryAttributeName, std::vector<AttributeModifierName>> PrimaryAttributeModifierNames =
{
	{ PrimaryAttributeName::Strength, { AttributeModifierName::MeleeDamage } },
	{ PrimaryAttributeName::Intelligence, { } },
	{ PrimaryAttributeName::Willpower, { AttributeModifierName::MagicDefense } },
	{ PrimaryAttributeName::Agility, { AttributeModifierName::ToHit, AttributeModifierName::ToDefense } },
	{ PrimaryAttributeName::Speed, { } },
	{ PrimaryAttributeName::Endurance, { AttributeModifierName::HealthPerLevel, 
	AttributeModifierName::HealModifier } },
	{ PrimaryAttributeName::Personality, { AttributeModifierName::Charisma } },
	{ PrimaryAttributeName::Luck, { } }
};

const int PrimaryAttribute::MIN_VALUE = 0;
const int PrimaryAttribute::MAX_VALUE = 100;

PrimaryAttribute::PrimaryAttribute(PrimaryAttributeName attributeName, int baseValue)
{
	assert(baseValue >= PrimaryAttribute::MIN_VALUE);
	assert(baseValue <= PrimaryAttribute::MAX_VALUE);

	this->attributeName = attributeName;
	this->baseValue = baseValue;
}

PrimaryAttribute::~PrimaryAttribute()
{

}

int PrimaryAttribute::get() const
{
	assert(this->baseValue >= PrimaryAttribute::MIN_VALUE);
	assert(this->baseValue <= PrimaryAttribute::MAX_VALUE);

	return this->baseValue;
}

PrimaryAttributeName PrimaryAttribute::getAttributeName() const
{
	return this->attributeName;
}

std::vector<AttributeModifierName> PrimaryAttribute::getModifierNames() const
{
	auto modifierNames = PrimaryAttributeModifierNames.at(this->getAttributeName());
	return modifierNames;
}

int PrimaryAttribute::getModifier() const
{
	assert(this->baseValue >= PrimaryAttribute::MIN_VALUE);
	assert(this->baseValue <= PrimaryAttribute::MAX_VALUE);

	// This value is exactly right. Try some more experiments.
	int modifierValue = (this->baseValue - 50) / 10;
	return modifierValue;
}

std::string PrimaryAttribute::toString() const
{
	auto displayName = PrimaryAttributeDisplayNames.at(this->getAttributeName());
	return displayName;
}

void PrimaryAttribute::set(int value)
{
	// The caller shouldn't try to set the value to a bad value. If only this was Ada!
	// Ada has automatic bounds checking on numeric types, like "percent is 0.0...100.0"
	// or something, so no assertions would be necessary.
	assert(value >= PrimaryAttribute::MIN_VALUE);
	assert(value <= PrimaryAttribute::MAX_VALUE);

	this->baseValue = value;
}
