#include "LogbookUiView.h"
#include "../Assets/ArenaTextureName.h"

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
