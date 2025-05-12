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
	if (!charClassDef.castsMagic)
	{
		return 0;
	}

	// @todo: exeData lookup for SpellPointModifiers in Player stats wiki. sorceror is special case
	constexpr std::pair<const char*, double> MagicClassIntelligenceMultipliers[] =
	{
		{ "Mage", 2.0 },
		{ "Spellsword", 1.5 },
		{ "Battlemage", 1.75 },
		{ "Sorceror", 3.0 },
		{ "Healer", 2.0 },
		{ "Nightblade", 1.5 },
		{ "Bard", 1.0 }
	};

	const auto iter = std::find_if(std::begin(MagicClassIntelligenceMultipliers), std::end(MagicClassIntelligenceMultipliers),
		[&charClassDef](const std::pair<const char*, double> &pair)
	{
		return StringView::equals(pair.first, charClassDef.name);
	});

	if (iter == std::end(MagicClassIntelligenceMultipliers))
	{
		return intelligence;
	}

	const double multiplier = iter->second;
	const int maxSpellPoints = static_cast<int>(std::floor(static_cast<double>(intelligence) * multiplier));
	return maxSpellPoints;
}
