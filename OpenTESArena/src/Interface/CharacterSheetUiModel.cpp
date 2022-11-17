#include "CharacterSheetUiModel.h"
#include "../Game/Game.h"

#include "components/debug/Debug.h"

std::string CharacterSheetUiModel::getPlayerName(Game &game)
{
	const Player &player = game.getGameState().getPlayer();
	return player.getDisplayName();
}

std::string CharacterSheetUiModel::getPlayerRaceName(Game &game)
{
	const Player &player = game.getGameState().getPlayer();
	const ExeData &exeData = game.getBinaryAssetLibrary().getExeData();

	const auto &singularRaceNames = exeData.races.singularNames;
	const int raceNameIndex = player.getRaceID();
	DebugAssertIndex(singularRaceNames, raceNameIndex);
	return singularRaceNames[raceNameIndex];
}

std::string CharacterSheetUiModel::getPlayerClassName(Game& game)
{
	const CharacterClassLibrary& charClassLibrary = game.getCharacterClassLibrary();
	const Player& player = game.getGameState().getPlayer();
	const int defID = player.getCharacterClassDefID();
	const CharacterClassDefinition& charClassDef = charClassLibrary.getDefinition(defID);
	return charClassDef.getName();
}

std::string CharacterSheetUiModel::getPlayerStrengthText(Game& game)
{
	const Player& player = game.getGameState().getPlayer();
	const int value = player.getStrength().get();
	return std::to_string(value);
}

std::string CharacterSheetUiModel::getPlayerIntelligenceText(Game& game)
{
	const Player& player = game.getGameState().getPlayer();
	const int value = player.getIntelligence().get();
	return std::to_string(value);
}

std::string CharacterSheetUiModel::getPlayerWillpowerText(Game& game)
{
	const Player& player = game.getGameState().getPlayer();
	const int value = player.getWillpower().get();
	return std::to_string(value);
}

std::string CharacterSheetUiModel::getPlayerAgilityText(Game& game)
{
	const Player& player = game.getGameState().getPlayer();
	const int value = player.getAgility().get();
	return std::to_string(value);
}

std::string CharacterSheetUiModel::getPlayerSpeedText(Game& game)
{
	const Player& player = game.getGameState().getPlayer();
	const int value = player.getSpeed().get();
	return std::to_string(value);
}

std::string CharacterSheetUiModel::getPlayerEnduranceText(Game& game)
{
	const Player& player = game.getGameState().getPlayer();
	const int value = player.getEndurance().get();
	return std::to_string(value);
}

std::string CharacterSheetUiModel::getPlayerPersonalityText(Game& game)
{
	const Player& player = game.getGameState().getPlayer();
	const int value = player.getPersonality().get();
	return std::to_string(value);
}

std::string CharacterSheetUiModel::getPlayerLuckText(Game& game)
{
	const Player& player = game.getGameState().getPlayer();
	const int value = player.getLuck().get();
	return std::to_string(value);
}
