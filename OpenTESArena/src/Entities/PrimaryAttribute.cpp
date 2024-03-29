#include <unordered_map>

#include "AttributeModifierName.h"
#include "PrimaryAttribute.h"
#include "PrimaryAttributeName.h"
#include "../Math/Random.h"

#include "components/debug/Debug.h"

const std::unordered_map<PrimaryAttributeName, std::string> PrimaryAttributeDisplayNames =
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

const std::unordered_map<PrimaryAttributeName, std::vector<AttributeModifierName>> PrimaryAttributeModifierNames =
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
	DebugAssert(baseValue >= PrimaryAttribute::MIN_VALUE);
	DebugAssert(baseValue <= PrimaryAttribute::MAX_VALUE);

	this->attributeName = attributeName;
	this->baseValue = baseValue;
}

PrimaryAttribute::PrimaryAttribute(PrimaryAttributeName attributeName, int raceID, bool male, Random &random)
{
	DebugAssert(raceID >= 0);
	DebugAssert(raceID <= 7);

	this->attributeName = attributeName;

	// @todo: read from ExeData::entities::raceAttributes
	this->baseValue = 0;
}

int PrimaryAttribute::get() const
{
	DebugAssert(this->baseValue >= PrimaryAttribute::MIN_VALUE);
	DebugAssert(this->baseValue <= PrimaryAttribute::MAX_VALUE);

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
	DebugAssert(this->baseValue >= PrimaryAttribute::MIN_VALUE);
	DebugAssert(this->baseValue <= PrimaryAttribute::MAX_VALUE);

	// This value is exactly right. Try some more experiments.
	int modifierValue = (this->baseValue - 50) / 10;
	return modifierValue;
}

std::string PrimaryAttribute::toString() const
{
	std::string displayName = PrimaryAttributeDisplayNames.at(this->getAttributeName());
	return displayName;
}

void PrimaryAttribute::set(int value)
{
	// The caller shouldn't try to set the value to a bad value. If only this was Ada!
	// Ada has automatic bounds checking on numeric types, like "percent is 0.0...100.0"
	// or something, so no assertions would be necessary.
	DebugAssert(value >= PrimaryAttribute::MIN_VALUE);
	DebugAssert(value <= PrimaryAttribute::MAX_VALUE);

	this->baseValue = value;
}
