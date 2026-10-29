#include "MainQuestSplashUiMVC.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Assets/TextureAsset.h"
#include "../Game/Game.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

std::string MainQuestSplashUiModel::getDungeonText(Game &game, ProvinceDefinitionID provinceDefID)
{
	const ProvinceDefinition &provinceDef = ProvinceLibrary::getInstance().getDefinition(provinceDefID);

	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const TextAssetLibrary &textAssetLibrary = TextAssetLibrary::getInstance();
	const ExeData &exeData = binaryAssetLibrary.getExeData();
	const Span<const uint8_t> splashIndices = exeData.travel.staffDungeonSplashIndices;
	const int index = splashIndices[provinceDef.originalProvinceIndex];

	// @todo: maybe don't split these two strings in the first place. And convert
	// the carriage return to a newline instead of removing it.
	const Span<const ArenaDungeonTxtEntry> dungeonTxtEntries = textAssetLibrary.dungeonTxt;
	const std::pair<std::string, std::string> &textPair = dungeonTxtEntries[index];
	return textPair.first + '\n' + textPair.second;
}

TextureAsset MainQuestSplashUiView::getSplashTextureAsset(ProvinceDefinitionID provinceDefID)
{
	const ProvinceDefinition &provinceDef = ProvinceLibrary::getInstance().getDefinition(provinceDefID);
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const Span<const uint8_t> staffDungeonSplashIndices = exeData.travel.staffDungeonSplashIndices;
	const int index = staffDungeonSplashIndices[provinceDef.originalProvinceIndex];

	const Span<const std::string> staffDungeonSplashes = exeData.travel.staffDungeonSplashes;
	return TextureAsset(String::toUppercase(staffDungeonSplashes[index]));
}
