#include <cassert>
#include <map>

#include "PrimaryAttribute.h"

#include "AttributeModifierName.h"

const auto PrimaryAttributeDisplayNames = std::map<PrimaryAttributeName, std::string>
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

const auto PrimaryAttributeModifierNames = std::map<PrimaryAttributeName, std::vector<AttributeModifierName>>
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
	this->attributeName = attributeName;
	this->baseValue = baseValue;

	assert(this->baseValue >= PrimaryAttribute::MIN_VALUE);
	assert(this->baseValue <= PrimaryAttribute::MAX_VALUE);
}

PrimaryAttribute::~PrimaryAttribute()
{

}

const int &PrimaryAttribute::get() const
{
	assert(this->baseValue >= PrimaryAttribute::MIN_VALUE);
	assert(this->baseValue <= PrimaryAttribute::MAX_VALUE);

	return this->baseValue;
}

const PrimaryAttributeName &PrimaryAttribute::getAttributeName() const
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

	// I don't think this value is exactly right. Try some more experiments.
	int modifierValue = (this->baseValue - 50) / 10;
	return modifierValue;
}

std::string PrimaryAttribute::toString() const
{
	auto displayName = PrimaryAttributeDisplayNames.at(this->getAttributeName());
	assert(displayName.size() > 0);
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
