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

TextureAssetReference LogbookUiView::getBackgroundPaletteTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::Logbook));
}

TextureAssetReference LogbookUiView::getBackgroundTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::Logbook));
}

UiTextureID LogbookUiView::allocBackgroundTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAssetReference textureAssetRef = LogbookUiView::getBackgroundTextureAssetRef();
	const TextureAssetReference paletteTextureAssetRef = LogbookUiView::getBackgroundPaletteTextureAssetRef();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAssetRef, paletteTextureAssetRef, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for logbook background \"" + textureAssetRef.filename + "\".");
	}

	return textureID;
}
