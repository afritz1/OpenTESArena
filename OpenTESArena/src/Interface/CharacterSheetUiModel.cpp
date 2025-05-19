#include <cmath>
#include <cstring>
#include <vector>

#include "CharacterSheetUiModel.h"
#include "../Game/Game.h"
#include "../Player/ArenaPlayerUtils.h"
#include "../Player/Player.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Stats/CharacterRaceLibrary.h"
#include "../Stats/PrimaryAttribute.h"

#include "components/debug/Debug.h"

namespace
{
	std::string GetPlayerCurrentMaxStatusString(double currentValue, double maxValue)
	{
		const int currentInt = static_cast<int>(std::round(currentValue));
		const int maxInt = static_cast<int>(std::round(maxValue));

		char buffer[64];
		std::snprintf(buffer, sizeof(buffer), "%d/%d", currentInt, maxInt);
		return std::string(buffer);
	}
}

std::string CharacterSheetUiModel::getDerivedAttributeDisplayString(int value)
{
	const char *signString = "";
	if (value >= 0)
	{
		signString = "+";
	}

	char buffer[64];
	std::snprintf(buffer, sizeof(buffer), "%s%d", signString, value);
	return buffer;
}

std::string CharacterSheetUiModel::getPlayerName(Game &game)
{
	const Player &player = game.player;
	return player.displayName;
}

std::string CharacterSheetUiModel::getPlayerRaceName(Game &game)
{
	const Player &player = game.player;
	const CharacterRaceLibrary &characterRaceLibrary = CharacterRaceLibrary::getInstance();
	const CharacterRaceDefinition &characterRaceDefinition = characterRaceLibrary.getDefinition(player.raceID);
	return characterRaceDefinition.singularName;
}

std::string CharacterSheetUiModel::getPlayerClassName(Game &game)
{
	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const Player &player = game.player;
	const int defID = player.charClassDefID;
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(defID);
	return charClassDef.name;
}

const PrimaryAttributes &CharacterSheetUiModel::getPlayerAttributes(Game &game)
{
	return game.player.primaryAttributes;
}

DerivedAttributes CharacterSheetUiModel::getPlayerDerivedAttributes(Game &game)
{
	return ArenaPlayerUtils::calculateTotalDerivedBonuses(game.player.primaryAttributes);
}

std::string CharacterSheetUiModel::getPlayerExperience(Game &game)
{
	const Player &player = game.player;
	return std::to_string(player.experience);
}

std::string CharacterSheetUiModel::getPlayerLevel(Game &game)
{
	const Player &player = game.player;
	return std::to_string(player.level);
}

std::string CharacterSheetUiModel::getPlayerHealth(Game &game)
{
	const Player &player = game.player;
	return GetPlayerCurrentMaxStatusString(player.currentHealth, player.maxHealth);
}

std::string CharacterSheetUiModel::getPlayerStamina(Game &game)
{
	const Player &player = game.player;
	return GetPlayerCurrentMaxStatusString(player.currentStamina, player.maxStamina);
}

std::string CharacterSheetUiModel::getPlayerSpellPoints(Game &game)
{
	const Player &player = game.player;
	return GetPlayerCurrentMaxStatusString(player.currentSpellPoints, player.maxSpellPoints);
}

std::string CharacterSheetUiModel::getPlayerGold(Game &game)
{
	const Player &player = game.player;
	return std::to_string(player.gold);
}
