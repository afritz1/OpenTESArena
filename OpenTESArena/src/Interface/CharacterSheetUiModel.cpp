#include <vector>

#include "CharacterSheetUiModel.h"
#include "../Game/Game.h"
#include "../Player/Player.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Stats/CharacterRaceLibrary.h"
#include "../Stats/PrimaryAttribute.h"

#include "components/debug/Debug.h"

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
	return charClassDef.getName();
}

const PrimaryAttributes &CharacterSheetUiModel::getPlayerAttributes(Game &game)
{
	return game.player.primaryAttributes;
}
