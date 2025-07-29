#include "LogbookUiModel.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Game/Game.h"

std::string LogbookUiModel::getTitleText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	return exeData.logbook.isEmpty;
}
