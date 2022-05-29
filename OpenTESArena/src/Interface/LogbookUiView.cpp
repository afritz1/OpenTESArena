#include "LogbookUiView.h"
#include "../Assets/ArenaTextureName.h"
#include "../Media/TextureUtils.h"

TextBox::InitInfo LogbookUiView::getTitleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		LogbookUiView::TitleTextCenterPoint,
		LogbookUiView::TitleFontName,
		LogbookUiView::TitleTextColor,
		LogbookUiView::TitleTextAlignment,
		fontLibrary);
}

TextureAsset LogbookUiView::getBackgroundPaletteTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::Logbook));
}

TextureAsset LogbookUiView::getBackgroundTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::Logbook));
}

UiTextureID LogbookUiView::allocBackgroundTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = LogbookUiView::getBackgroundTextureAsset();
	const TextureAsset paletteTextureAsset = LogbookUiView::getBackgroundPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for logbook background \"" + textureAsset.filename + "\".");
	}

	return textureID;
}
