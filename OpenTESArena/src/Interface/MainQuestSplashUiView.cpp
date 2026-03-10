#include "MainQuestSplashUiView.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextureAsset.h"

#include "components/utilities/String.h"

TextureAsset MainQuestSplashUiView::getSplashTextureAsset(int provinceID)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const auto &staffDungeonSplashIndices = exeData.travel.staffDungeonSplashIndices;
	DebugAssertIndex(staffDungeonSplashIndices, provinceID);
	const int index = staffDungeonSplashIndices[provinceID];

	const auto &staffDungeonSplashes = exeData.travel.staffDungeonSplashes;
	DebugAssertIndex(staffDungeonSplashes, index);
	return TextureAsset(String::toUppercase(staffDungeonSplashes[index]));
}
