#include <algorithm>
#include <cmath>

#include "ArenaPlayerUtils.h"
#include "../Math/Random.h"
#include "../Stats/CharacterClassLibrary.h"

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

int ArenaPlayerUtils::calculateBonusToHit(int Agility)
{
	if (Agility <= 45) return -1;
	if (Agility <= 46) return 0;
	return (Agility - 50) / 5;
}


int ArenaPlayerUtils::calculateBonusToCharisma(int charisma)
{
	if (charisma <= 45) return -1;
	if (charisma <= 46) return 0;
	return (charisma - 50) / 5;
}

int ArenaPlayerUtils::calculateBonusToHealth(int endurance)
{
	if (endurance <= 34) return -1;
	if (endurance <= 54) return 0;
	int bonus = (endurance - 55) / 10 + 1;
	return std::min(bonus, 5);
}

int ArenaPlayerUtils::calculateDamageBonus(int strength)
{
	if (strength <= 43) return 0;
	return (strength - 48) / 5;
}
int ArenaPlayerUtils::calculateMagicDefenseBonus(int Will) {
	if (Will <= 38) return -2;
	if (Will <= 41) return -1;
	if (Will <= 46) return 0;
	return (Will - 46) / 9; 
}

ArenaPlayerUtils::AttributeBonusValues ArenaPlayerUtils::calculateAttributeBonus(const char* attributeName, int attributeValue) {
	AttributeBonusValues values = { 0, 0, 0, 0, 0, 0, 0, 0 };

	if (strcmp(attributeName, "Agility") == 0) {
		values.bonusToHit = calculateBonusToHit(attributeValue);
		values.bonusToDefend = values.bonusToHit;
	}
	else if (strcmp(attributeName, "Personality") == 0) {
		//Personality and agility have the same bonus progression.
		values.bonusToCharisma = calculateBonusToHit(attributeValue);
	}
	else if (strcmp(attributeName, "Endurance") == 0) {
		values.bonusToHealth = calculateBonusToHealth(attributeValue);
		values.healMod = values.bonusToHealth;
	}
	else if (strcmp(attributeName, "Strength") == 0) {
		values.bonusDamage = calculateDamageBonus(attributeValue);
		values.maxKilos = attributeValue * 2;
	}
	else if (strcmp(attributeName, "Willpower") == 0) {
		values.magicDef = calculateMagicDefenseBonus(attributeValue);
	}
	return values;
}