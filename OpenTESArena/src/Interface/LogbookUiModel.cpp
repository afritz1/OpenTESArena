#include "LogbookUiModel.h"
#include "../Game/Game.h"

std::string LogbookUiModel::getTitleText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	return exeData.logbook.isEmpty;
}
