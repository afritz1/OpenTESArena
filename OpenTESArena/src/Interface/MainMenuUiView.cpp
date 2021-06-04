#include "MainMenuUiView.h"

Color MainMenuUiView::getTestButtonTextColor()
{
	return Color::White;
}

TextureAssetReference MainMenuUiView::getBackgroundTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::MainMenu));
}

TextureAssetReference MainMenuUiView::getPaletteTextureAssetRef()
{
	return MainMenuUiView::getBackgroundTextureAssetRef();
}

TextureAssetReference MainMenuUiView::getTestArrowsTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::UpDown));
}

TextureAssetReference MainMenuUiView::getTestArrowsPaletteTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaPaletteName::CharSheet));
}
