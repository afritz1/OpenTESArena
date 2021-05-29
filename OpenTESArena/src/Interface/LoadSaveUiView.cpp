#include "LoadSaveUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Rendering/ArenaRenderUtils.h"

Int2 LoadSaveUiView::getEntryCenterPoint(int index)
{
	return Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, 8 + (index * 14));
}

TextureAssetReference LoadSaveUiView::getPaletteTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaPaletteName::Default));
}

TextureAssetReference LoadSaveUiView::getLoadSaveTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::LoadSave));
}
