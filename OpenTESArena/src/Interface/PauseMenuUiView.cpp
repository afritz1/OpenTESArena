#include "PauseMenuUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/TextureAssetReference.h"

TextureAssetReference PauseMenuUiView::getBackgroundPaletteTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaPaletteName::Default));
}

TextureAssetReference PauseMenuUiView::getBackgroundTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::PauseBackground));
}
