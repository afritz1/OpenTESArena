#include "LogbookUiView.h"
#include "../Assets/ArenaTextureName.h"

TextureAssetReference LogbookUiView::getBackgroundPaletteTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::Logbook));
}

TextureAssetReference LogbookUiView::getBackgroundTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::Logbook));
}
