#include "MainQuestSplashUiModel.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Game/Game.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

std::string MainQuestSplashUiModel::getDungeonText(Game &game, int provinceID)
{
	const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const auto &textAssetLibrary = TextAssetLibrary::getInstance();
	const auto &exeData = binaryAssetLibrary.getExeData();
	const auto &splashIndices = exeData.travel.staffDungeonSplashIndices;
	DebugAssertIndex(splashIndices, provinceID);
	const int index = splashIndices[provinceID];

	const BufferView<const ArenaDungeonTxtEntry> dungeonTxtEntries = textAssetLibrary.dungeonTxt;

	// @todo: maybe don't split these two strings in the first place. And convert
	// the carriage return to a newline instead of removing it.
	const std::pair<std::string, std::string> &textPair = dungeonTxtEntries[index];	
	return textPair.first + '\n' + textPair.second;
}
