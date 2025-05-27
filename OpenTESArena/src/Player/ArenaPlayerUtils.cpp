﻿#include <algorithm>
#include <cmath>

#include "ArenaPlayerUtils.h"
#include "../Math/Random.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Stats/PrimaryAttribute.h"

#include "components/utilities/StringView.h"

int ArenaPlayerUtils::getBaseSpeed(int speedAttribute, int encumbranceMod)
{
	return ((((speedAttribute * 20) / 256) * (256 - encumbranceMod)) / 256) + 20;
}

int ArenaPlayerUtils::getMoveSpeed(int baseSpeed)
{
	return baseSpeed;
}

int ArenaPlayerUtils::getTurnSpeed(int baseSpeed)
{
	return (baseSpeed / 2) + 13;
}

int ArenaPlayerUtils::getChasmFallSpeed(int frame)
{
	return static_cast<int>(std::pow(2, 2 + (frame / 2)));
}

int ArenaPlayerUtils::getJumpUnitsPerFrame(int frame)
{
	return 10 - (2 * frame);
}

int ArenaPlayerUtils::rollHealthDice(int healthDie, Random &random)
{
	return 1 + random.next(healthDie);
}

int ArenaPlayerUtils::calculateMaxHealthPoints(int charClassDefID, Random &random)
{
	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	constexpr int baseHealthPoints = 25;
	const int classHitDieRoll = ArenaPlayerUtils::rollHealthDice(charClassDef.healthDie, random);
	const int totalHealthPoints = baseHealthPoints + classHitDieRoll;
	return totalHealthPoints;
}

int ArenaPlayerUtils::calculateMaxStamina(int strength, int endurance)
{
	return strength + endurance;
}

int ArenaPlayerUtils::calculateMaxSpellPoints(int charClassDefID, int intelligence)
{
	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const int maxSpellPoints = static_cast<int>(static_cast<double>(intelligence) * charClassDef.spellPointsMultiplier);
	return maxSpellPoints;
}

int ArenaPlayerUtils::calculateDamageBonus(int strength)
{
	if (strength <= 43)
	{
		return 0;
	}

	return (strength - 48) / 5;
}

int ArenaPlayerUtils::calculateMaxWeight(int strength)
{
	return strength * 2;
}

int ArenaPlayerUtils::calculateMagicDefenseBonus(int willpower)
{
	if (willpower <= 38)
	{
		return -2;
	}
	else if (willpower <= 41)
	{
		return -1;
	}
	else if (willpower <= 46)
	{
		return 0;
	}

	return (willpower - 46) / 9;
}

int ArenaPlayerUtils::calculateBonusToHit(int agility)
{
	if (agility <= 45)
	{
		return -1;
	}
	else if (agility <= 46)
	{
		return 0;
	}

	return (agility - 50) / 5;
}

int ArenaPlayerUtils::calculateBonusToHealth(int endurance)
{
	if (endurance <= 34)
	{
		return -1;
	}
	else if (endurance <= 54)
	{
		return 0;
	}

	const int bonus = ((endurance - 55) / 10) + 1;
	return std::min(bonus, 5);
}

int ArenaPlayerUtils::calculateStartingGold(Random &random)
{
	return 50 + random.next(150);
}

DerivedAttributes ArenaPlayerUtils::calculateStrengthDerivedBonuses(int strength)
{
	DerivedAttributes values;
	values.bonusDamage = ArenaPlayerUtils::calculateDamageBonus(strength);
	values.maxKilos = ArenaPlayerUtils::calculateMaxWeight(strength);
	return values;
}

DerivedAttributes ArenaPlayerUtils::calculateWillpowerDerivedBonuses(int willpower)
{
	DerivedAttributes values;
	values.magicDef = ArenaPlayerUtils::calculateMagicDefenseBonus(willpower);
	return values;
}

DerivedAttributes ArenaPlayerUtils::calculateAgilityDerivedBonuses(int agility)
{
	DerivedAttributes values;
	values.bonusToHit = ArenaPlayerUtils::calculateBonusToHit(agility);
	values.bonusToDefend = values.bonusToHit;
	return values;
}

DerivedAttributes ArenaPlayerUtils::calculateEnduranceDerivedBonuses(int endurance)
{
	DerivedAttributes values;
	values.bonusToHealth = ArenaPlayerUtils::calculateBonusToHealth(endurance);
	values.healMod = values.bonusToHealth;
	return values;
}

DerivedAttributes ArenaPlayerUtils::calculatePersonalityDerivedBonuses(int personality)
{
	// Personality and agility have the same bonus progression.
	DerivedAttributes values;
	values.bonusToCharisma = ArenaPlayerUtils::calculateBonusToHit(personality);
	return values;
}

DerivedAttributes ArenaPlayerUtils::calculateTotalDerivedBonuses(const PrimaryAttributes &attributes)
{
	DerivedAttributes totalDerivedAttributes;

	auto addToTotalDerivedAttributes = [&totalDerivedAttributes](const DerivedAttributes &derived)
	{
		totalDerivedAttributes.bonusToHit += derived.bonusToHit;
		totalDerivedAttributes.bonusToDefend += derived.bonusToDefend;
		totalDerivedAttributes.bonusToCharisma += derived.bonusToCharisma;
		totalDerivedAttributes.bonusToHealth += derived.bonusToHealth;
		totalDerivedAttributes.healMod += derived.healMod;
		totalDerivedAttributes.bonusDamage += derived.bonusDamage;
		totalDerivedAttributes.maxKilos += derived.maxKilos;
		totalDerivedAttributes.magicDef += derived.magicDef;
	};

	addToTotalDerivedAttributes(ArenaPlayerUtils::calculateStrengthDerivedBonuses(attributes.strength.maxValue));
	addToTotalDerivedAttributes(ArenaPlayerUtils::calculateAgilityDerivedBonuses(attributes.agility.maxValue));
	addToTotalDerivedAttributes(ArenaPlayerUtils::calculateWillpowerDerivedBonuses(attributes.willpower.maxValue));
	addToTotalDerivedAttributes(ArenaPlayerUtils::calculateEnduranceDerivedBonuses(attributes.endurance.maxValue));
	addToTotalDerivedAttributes(ArenaPlayerUtils::calculatePersonalityDerivedBonuses(attributes.personality.maxValue));

	return totalDerivedAttributes;
}
