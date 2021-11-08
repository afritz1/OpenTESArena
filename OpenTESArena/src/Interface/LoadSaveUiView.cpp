#include "LoadSaveUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Media/TextureUtils.h"
#include "../Rendering/ArenaRenderUtils.h"

Color LoadSaveUiView::getEntryTextColor()
{
	return Color::White;
}

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

UiTextureID LoadSaveUiView::allocBackgroundTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAssetReference textureAssetRef = LoadSaveUiView::getLoadSaveTextureAssetRef();
	const TextureAssetReference paletteTextureAssetRef = LoadSaveUiView::getPaletteTextureAssetRef();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAssetRef, paletteTextureAssetRef, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for load/save background \"" + textureAssetRef.filename + "\".");
	}

	return textureID;
}
