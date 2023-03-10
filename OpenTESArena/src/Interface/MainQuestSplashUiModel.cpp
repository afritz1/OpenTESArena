#include "MainQuestSplashUiModel.h"
#include "../Game/Game.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

std::string MainQuestSplashUiModel::getDungeonText(Game &game, int provinceID)
{
	const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
	const auto &textAssetLibrary = game.getTextAssetLibrary();

	// @todo: maybe don't split these two strings in the first place. And convert
	// the carriage return to a newline instead of removing it.
	const std::pair<std::string, std::string> &pair = [provinceID, &binaryAssetLibrary, &textAssetLibrary]()
	{
		const auto &exeData = binaryAssetLibrary.getExeData();
		const auto &splashIndices = exeData.travel.staffDungeonSplashIndices;
		DebugAssertIndex(splashIndices, provinceID);
		const int index = splashIndices[provinceID];
		
		const BufferView<const TextAssetLibrary::DungeonTxtEntry> dungeonPairs = textAssetLibrary.getDungeonTxtDungeons();
		DebugAssertIndex(dungeonPairs, index);
		return dungeonPairs[index];
	}();

	return pair.first + '\n' + pair.second;
}
