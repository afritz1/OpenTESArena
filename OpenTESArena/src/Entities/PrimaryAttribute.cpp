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

	// Lower limit (non-inclusive) of base attribute value, set by race and gender below.
	// 20-sided-die roll will be added.
	int baseValueBase;

	// Source: https://en.uesp.net/wiki/Arena:Character_Creation#Character_Stats
	if (raceID == 0) // Breton
	{
		if (attributeName == PrimaryAttributeName::Strength) baseValueBase = 30;
		else if (attributeName == PrimaryAttributeName::Intelligence) baseValueBase = 50;
		else if (attributeName == PrimaryAttributeName::Willpower) baseValueBase = 50;
		else if (attributeName == PrimaryAttributeName::Agility) baseValueBase = 40;
		else if (attributeName == PrimaryAttributeName::Speed) baseValueBase = 40;
		else if (attributeName == PrimaryAttributeName::Endurance) baseValueBase = 30;
		else if (attributeName == PrimaryAttributeName::Personality) baseValueBase = 40;
		else baseValueBase = 40;
	}
	else if (raceID == 1) // Redguard
	{
		if (attributeName == PrimaryAttributeName::Strength) baseValueBase = male ? 40 : 30;
		else if (attributeName == PrimaryAttributeName::Intelligence) baseValueBase = 30;
		else if (attributeName == PrimaryAttributeName::Willpower) baseValueBase = 30;
		else if (attributeName == PrimaryAttributeName::Agility) baseValueBase = male ? 40 : 50;
		else if (attributeName == PrimaryAttributeName::Speed) baseValueBase = 50;
		else if (attributeName == PrimaryAttributeName::Endurance) baseValueBase = 50;
		else if (attributeName == PrimaryAttributeName::Personality) baseValueBase = 40;
		else baseValueBase = 40;
	}
	else if (raceID == 2) // Nord
	{
		if (attributeName == PrimaryAttributeName::Strength) baseValueBase = male ? 50 : 40;
		else if (attributeName == PrimaryAttributeName::Intelligence) baseValueBase = 30;
		else if (attributeName == PrimaryAttributeName::Willpower) baseValueBase = male ? 30 : 40;
		else if (attributeName == PrimaryAttributeName::Agility) baseValueBase = male ? 30 : 40;
		else if (attributeName == PrimaryAttributeName::Speed) baseValueBase = 40;
		else if (attributeName == PrimaryAttributeName::Endurance) baseValueBase = male ? 50 : 40;
		else if (attributeName == PrimaryAttributeName::Personality) baseValueBase = 40;
		else baseValueBase = male ? 40 : 50;
	}
	else if (raceID == 3) // Dark Elf
	{
		if (attributeName == PrimaryAttributeName::Strength) baseValueBase = male ? 50 : 40;
		else if (attributeName == PrimaryAttributeName::Intelligence) baseValueBase = 50;
		else if (attributeName == PrimaryAttributeName::Willpower) baseValueBase = 30;
		else if (attributeName == PrimaryAttributeName::Agility) baseValueBase = male ? 50 : 40;
		else if (attributeName == PrimaryAttributeName::Speed) baseValueBase = male ? 50 : 40;
		else if (attributeName == PrimaryAttributeName::Endurance) baseValueBase = 40;
		else if (attributeName == PrimaryAttributeName::Personality) baseValueBase = 40;
		else baseValueBase = 40;
	}
	else if (raceID == 4) // High Elf
	{
		if (attributeName == PrimaryAttributeName::Strength) baseValueBase = 30;
		else if (attributeName == PrimaryAttributeName::Intelligence) baseValueBase = 50;
		else if (attributeName == PrimaryAttributeName::Willpower) baseValueBase = male ? 50 : 40;
		else if (attributeName == PrimaryAttributeName::Agility) baseValueBase = 40;
		else if (attributeName == PrimaryAttributeName::Speed) baseValueBase = 40;
		else if (attributeName == PrimaryAttributeName::Endurance) baseValueBase = 30;
		else if (attributeName == PrimaryAttributeName::Personality) baseValueBase = male ? 40 : 50;
		else baseValueBase = 40;
	}
	else if (raceID == 5) // Wood Elf
	{
		if (attributeName == PrimaryAttributeName::Strength) baseValueBase = 40;
		else if (attributeName == PrimaryAttributeName::Intelligence) baseValueBase = 40;
		else if (attributeName == PrimaryAttributeName::Willpower) baseValueBase = 40;
		else if (attributeName == PrimaryAttributeName::Agility) baseValueBase = male ? 50 : 40;
		else if (attributeName == PrimaryAttributeName::Speed) baseValueBase = male ? 50 : 40;
		else if (attributeName == PrimaryAttributeName::Endurance) baseValueBase = male ? 30 : 40;
		else if (attributeName == PrimaryAttributeName::Personality) baseValueBase = 40;
		else baseValueBase = male ? 30 : 40;
	}
	else if (raceID == 6) // Khajiit
	{
		if (attributeName == PrimaryAttributeName::Strength) baseValueBase = 40;
		else if (attributeName == PrimaryAttributeName::Intelligence) baseValueBase = 40;
		else if (attributeName == PrimaryAttributeName::Willpower) baseValueBase = 30;
		else if (attributeName == PrimaryAttributeName::Agility) baseValueBase = male ? 50 : 40;
		else if (attributeName == PrimaryAttributeName::Speed) baseValueBase = male ? 40 : 50;
		else if (attributeName == PrimaryAttributeName::Endurance) baseValueBase = 30;
		else if (attributeName == PrimaryAttributeName::Personality) baseValueBase = 40;
		else baseValueBase = 50;
	}
	else // Argonian
	{
		if (attributeName == PrimaryAttributeName::Strength) baseValueBase = 40;
		else if (attributeName == PrimaryAttributeName::Intelligence) baseValueBase = 40;
		else if (attributeName == PrimaryAttributeName::Willpower) baseValueBase = 40;
		else if (attributeName == PrimaryAttributeName::Agility) baseValueBase = male ? 50 : 40;
		else if (attributeName == PrimaryAttributeName::Speed) baseValueBase = male ? 50 : 40;
		else if (attributeName == PrimaryAttributeName::Endurance) baseValueBase = male ? 30 : 40;
		else if (attributeName == PrimaryAttributeName::Personality) baseValueBase = 40;
		else baseValueBase = male ? 30 : 40;
	}

	this->baseValue = baseValueBase + random.next(20) + 1;
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
	auto displayName = PrimaryAttributeDisplayNames.at(this->getAttributeName());
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
