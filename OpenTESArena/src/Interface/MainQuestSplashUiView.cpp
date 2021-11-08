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

TextBox::InitInfo MainQuestSplashUiView::getDescriptionTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary)
{
	const std::string &fontName = MainQuestSplashUiView::DescriptionFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderUtils::TextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(text, fontDef);
	return TextBox::InitInfo::makeWithXY(
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

TextureAssetReference MainQuestSplashUiView::getSplashTextureAssetRef(Game &game, int provinceID)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	const auto &staffDungeonSplashIndices = exeData.travel.staffDungeonSplashIndices;
	DebugAssertIndex(staffDungeonSplashIndices, provinceID);
	const int index = staffDungeonSplashIndices[provinceID];

	const auto &staffDungeonSplashes = exeData.travel.staffDungeonSplashes;
	DebugAssertIndex(staffDungeonSplashes, index);
	return TextureAssetReference(String::toUppercase(staffDungeonSplashes[index]));
}

UiTextureID MainQuestSplashUiView::allocSplashTextureID(Game &game, int provinceID)
{
	TextureManager &textureManager = game.getTextureManager();
	Renderer &renderer = game.getRenderer();
	const TextureAssetReference textureAssetRef = MainQuestSplashUiView::getSplashTextureAssetRef(game, provinceID);
	const TextureAssetReference paletteTextureAssetRef = textureAssetRef;

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAssetRef, paletteTextureAssetRef, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for main quest splash \"" + textureAssetRef.filename + "\".");
	}

	return textureID;
}
