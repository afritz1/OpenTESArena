#include "CharacterSheetUiMVC.h"
#include "LevelUpUiMVC.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Game/Game.h"

std::string LevelUpUiModel::getPlayerHealth(Game &game)
{
	const Player &player = game.player;
	const CharacterLevelUpState &charLevelUpState = *game.charLevelUpState;
	return CharacterSheetUiModel::getStatusValueCurrentAndMaxString(player.currentHealth, charLevelUpState.maxHealth);
}

std::string LevelUpUiModel::getPlayerStamina(Game &game)
{
	const Player &player = game.player;
	const CharacterLevelUpState &charLevelUpState = *game.charLevelUpState;
	return CharacterSheetUiModel::getStatusValueCurrentAndMaxString(player.currentStamina, charLevelUpState.maxStamina);
}

std::string LevelUpUiModel::getPlayerSpellPoints(Game &game)
{
	const Player &player = game.player;
	const CharacterLevelUpState &charLevelUpState = *game.charLevelUpState;
	return CharacterSheetUiModel::getStatusValueCurrentAndMaxString(player.currentSpellPoints, charLevelUpState.maxSpellPoints);
}

std::string LevelUpUiModel::getBonusPointsRemainingText(Game &game)
{
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	return exeData.charCreation.chooseAttributesBonusPointsRemaining;
}

int LevelUpUiView::getRemainingPointsTextBoxTextureWidth(int textWidth)
{
	return textWidth + 12;
}

int LevelUpUiView::getRemainingPointsTextBoxTextureHeight(int textHeight)
{
	return textHeight + 12;
}
