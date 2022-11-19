#include <vector>

#include "CharacterSheetUiModel.h"
#include "../Entities/PrimaryAttribute.h"
#include "../Game/Game.h"

#include "components/debug/Debug.h"

std::string CharacterSheetUiModel::getPlayerName(Game &game)
{
	const Player &player = game.getPlayer();
	return player.getDisplayName();
}

std::string CharacterSheetUiModel::getPlayerRaceName(Game &game)
{
	const Player &player = game.getPlayer();
	const ExeData &exeData = game.getBinaryAssetLibrary().getExeData();

	const auto &singularRaceNames = exeData.races.singularNames;
	const int raceNameIndex = player.getRaceID();
	DebugAssertIndex(singularRaceNames, raceNameIndex);
	return singularRaceNames[raceNameIndex];
}

std::string CharacterSheetUiModel::getPlayerClassName(Game &game)
{
	const CharacterClassLibrary &charClassLibrary = game.getCharacterClassLibrary();
	const Player &player = game.getPlayer();
	const int defID = player.getCharacterClassDefID();
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(defID);
	return charClassDef.getName();
}

std::vector<PrimaryAttribute> CharacterSheetUiModel::getPlayerAttributes(Game &game)
{
	const Player& player = game.getGameState().getPlayer();
	return player.getAttributes().getAll();
}
