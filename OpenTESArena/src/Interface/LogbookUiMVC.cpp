#include "LogbookUiMVC.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Game/Game.h"

std::string LogbookUiModel::getNoEntriesText(Game &game)
{
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	return exeData.logbook.isEmpty;
}
