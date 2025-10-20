#include <algorithm>
#include <cmath>

#include "ArenaPlayerUtils.h"
#include "../Assets/ExeData.h"
#include "../Math/Random.h"
#include "../Player/Player.h"
#include "../Stats/ArenaStatUtils.h"
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
	const int endurance256Base = ArenaStatUtils::scale100To256(endurance);
	const int result256Base = (endurance256Base - 128 + 12) / 25;
	return ArenaStatUtils::scale256To100(result256Base);
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

int ArenaPlayerUtils::getThievingChance(int difficultyLevel, int thievingDivisor, int playerLevel, const PrimaryAttributes &attributes)
{
	DebugAssert(thievingDivisor > 0);
	DebugAssert(difficultyLevel > 0);

	const int attributesModifier = attributes.intelligence.maxValue + attributes.agility.maxValue;
	const int ability = ((((attributesModifier / thievingDivisor) * (playerLevel + 1)) * 100) / (difficultyLevel * 100));
	const int clampedAbility = std::clamp(ability, 0, 100);
	return clampedAbility;
}

bool ArenaPlayerUtils::attemptThieving(int difficultyLevel, int thievingDivisor, int playerLevel, const PrimaryAttributes &attributes, Random &random)
{
	const int thievingChance = ArenaPlayerUtils::getThievingChance(difficultyLevel, thievingDivisor, playerLevel, attributes);
	const int roll = random.next(100);
	return thievingChance >= roll;
}

int ArenaPlayerUtils::getLockDifficultyMessageIndex(int difficultyLevel, int thievingDivisor, int playerLevel, const PrimaryAttributes &attributes, const ExeData &exeData)
{
	int index;
	if (difficultyLevel >= 20)
	{
		// Magically-locked door. Use the last message.
		index = static_cast<int>(std::size(exeData.status.lockDifficultyMessages)) - 1;
	}
	else
	{
		const int thievingChance = ArenaPlayerUtils::getThievingChance(difficultyLevel, thievingDivisor, playerLevel, attributes);
		index = (thievingChance / 5) - 6;
		index = std::clamp(index, 0, static_cast<int>(std::size(exeData.status.lockDifficultyMessages) - 2));
	}

	return index;
}

int ArenaPlayerUtils::getSelfDamageFromDoorBashWithFists(Random &random)
{
	const int roll = random.next(100);
	const bool shouldDamagePlayer = roll >= 20;
	if (!shouldDamagePlayer)
	{
		return 0;
	}

	return 1;
}

bool ArenaPlayerUtils::isDoorBashSuccessful(int damage, int lockLevel, const PrimaryAttributes &attributes, Random &random)
{
	constexpr int minDamageRequired = ArenaPlayerUtils::DoorBashMinDamageRequired;
	if (damage < minDamageRequired)
	{
		return false;
	}

	const int difficultyLevel = lockLevel * 5;
	const int threshold = (ArenaStatUtils::scale100To256(attributes.strength.maxValue) * 100 >> 8) - difficultyLevel;
	const int roll = random.next(100);
	return threshold >= roll;
}

void ArenaPlayerUtils::applyRestHealing(Player &player, int restFactor, int tavernRoomType, const ExeData &exeData)
{
	// @todo: Attribute recovery

	// Health recovery
	const int bonusHealing = ArenaPlayerUtils::calculateEnduranceDerivedBonuses(player.primaryAttributes.endurance.maxValue).healMod;
	const int healerBonus = (player.charClassDefID == 4) ? 20 : 0;
	const int multiplier = (bonusHealing * 5) + 60 + healerBonus;
	int healAmount = (static_cast<int>(player.maxHealth) * restFactor * multiplier) / 1000;

	// The original game checks whether the player is the Barbarian class here and ANDs the healMod against
	// itself. If the healMod is <= 0 it zeroes out an already-zero value to which roomModifier is added, having
	// no effect. Possibly the AND was supposed to be an ADD, so that the Barbarian would get 2x the healMod,
	// or 0 if the healMod was negative, added to roomModifier before multiplying it by restFactor.

	const auto &tavernRoomHealModifiers = exeData.services.tavernRoomHealModifiers;
	DebugAssertIndex(tavernRoomHealModifiers, tavernRoomType);
	const int roomModifier = tavernRoomHealModifiers[tavernRoomType];
	const int roomRestHealAmount = roomModifier * restFactor;
	healAmount += roomRestHealAmount;

	if (healAmount <= 0)
	{
		healAmount = 1;
	}

	player.currentHealth = std::min(player.currentHealth + healAmount, player.maxHealth);

	// Stamina recovery
	const int staminaCap = ArenaPlayerUtils::calculateMaxStamina(player.primaryAttributes.strength.maxValue, player.primaryAttributes.endurance.maxValue);
	const int staminaGainMultiplier = (bonusHealing * 5) + 70;
	const int staminaGainAmount256 = ((ArenaStatUtils::scale100To256(staminaCap) << 6) * restFactor) / 1000;
	const int staminaGainAmount = ArenaStatUtils::scale256To100((staminaGainAmount256 * staminaGainMultiplier) >> 6);
	player.currentStamina = std::min(player.currentStamina + staminaGainAmount256, static_cast<double>(staminaCap));

	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(player.charClassDefID);

	// Spell points recovery
	if (charClassDef.castsMagic && (player.charClassDefID != 3) && (player.currentSpellPoints < player.maxSpellPoints))
	{
		const int spellPointsGainAmount = (static_cast<int>(player.maxSpellPoints) * restFactor) >> 3;
		player.currentSpellPoints = std::min(player.currentSpellPoints + spellPointsGainAmount, player.maxSpellPoints);
	}
}
