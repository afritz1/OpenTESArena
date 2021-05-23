#include "CharacterCreationUiModel.h"
#include "../Game/Game.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

std::string CharacterCreationUiModel::getPlayerName(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	return std::string(charCreationState.getName());
}

std::string CharacterCreationUiModel::getPlayerRaceName(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const ExeData &exeData = game.getBinaryAssetLibrary().getExeData();

	const auto &singularRaceNames = exeData.races.singularNames;
	const int raceNameIndex = charCreationState.getRaceIndex();
	DebugAssertIndex(singularRaceNames, raceNameIndex);
	return singularRaceNames[raceNameIndex];
}

std::string CharacterCreationUiModel::getPlayerClassName(Game &game)
{
	const CharacterClassLibrary &charClassLibrary = game.getCharacterClassLibrary();
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const int defID = charCreationState.getClassDefID();
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(defID);
	return charClassDef.getName();
}

std::string CharacterCreationUiModel::getChooseClassCreationTitleText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	std::string text = exeData.charCreation.chooseClassCreation;
	text = String::replace(text, '\r', '\n');
	return text;
}

std::string CharacterCreationUiModel::getGenerateClassButtonText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	return exeData.charCreation.chooseClassCreationGenerate;
}

std::string CharacterCreationUiModel::getGenerateClassButtonTooltipText()
{
	return "Answer questions\n(not implemented)";
}

std::string CharacterCreationUiModel::getSelectClassButtonText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	return exeData.charCreation.chooseClassCreationSelect;
}

std::string CharacterCreationUiModel::getSelectClassButtonTooltipText()
{
	return "Choose from a list";
}

std::string CharacterCreationUiModel::getChooseAttributesText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	std::string text = exeData.charCreation.distributeClassPoints;
	text = String::replace(text, '\r', '\n');
	return text;
}

std::string CharacterCreationUiModel::getAttributesMessageBoxTitleText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	return exeData.charCreation.chooseAttributes;
}

std::string CharacterCreationUiModel::getAttributesMessageBoxSaveText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	std::string text = exeData.charCreation.chooseAttributesSave;

	// @todo: use the formatting characters in the string for color.
	// - For now, just delete them.
	text.erase(1, 2);

	return text;
}

std::string CharacterCreationUiModel::getAttributesMessageBoxRerollText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	std::string text = exeData.charCreation.chooseAttributesReroll;

	// @todo: use the formatting characters in the string for color.
	// - For now, just delete them.
	text.erase(1, 2);

	return text;
}

std::string CharacterCreationUiModel::getAppearanceMessageBoxText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	std::string text = exeData.charCreation.chooseAppearance;
	text = String::replace(text, '\r', '\n');
	return text;
}
