#include "MainQuestSplashUiView.h"
#include "../Game/Game.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/FontLibrary.h"

#include "components/utilities/String.h"

int MainQuestSplashUiView::getDescriptionTextBoxX(int textWidth)
{
	return (ArenaRenderUtils::SCREEN_WIDTH / 2) - (textWidth / 2);
}

int MainQuestSplashUiView::getDescriptionTextBoxY()
{
	return 133;
}

TextBoxInitInfo MainQuestSplashUiView::getDescriptionTextBoxInitInfo(const std::string_view text, const FontLibrary &fontLibrary)
{
	const std::string &fontName = MainQuestSplashUiView::DescriptionFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(text, fontDef);
	return TextBoxInitInfo::makeWithXY(
		text,
		MainQuestSplashUiView::getDescriptionTextBoxX(textureGenInfo.width),
		MainQuestSplashUiView::getDescriptionTextBoxY(),
		fontName,
		MainQuestSplashUiView::DescriptionTextColor,
		MainQuestSplashUiView::DescriptionTextAlignment,
		std::nullopt,
		MainQuestSplashUiView::DescriptionLineSpacing,
		fontLibrary);
}

TextureAsset MainQuestSplashUiView::getSplashTextureAsset(Game &game, int provinceID)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const auto &staffDungeonSplashIndices = exeData.travel.staffDungeonSplashIndices;
	DebugAssertIndex(staffDungeonSplashIndices, provinceID);
	const int index = staffDungeonSplashIndices[provinceID];

	const auto &staffDungeonSplashes = exeData.travel.staffDungeonSplashes;
	DebugAssertIndex(staffDungeonSplashes, index);
	return TextureAsset(String::toUppercase(staffDungeonSplashes[index]));
}

UiTextureID MainQuestSplashUiView::allocSplashTextureID(Game &game, int provinceID)
{
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	const TextureAsset textureAsset = MainQuestSplashUiView::getSplashTextureAsset(game, provinceID);
	const TextureAsset paletteTextureAsset = textureAsset;

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for main quest splash \"" + textureAsset.filename + "\".");
	}

	return textureID;
}
