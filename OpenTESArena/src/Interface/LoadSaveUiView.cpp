#include "LoadSaveUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/TextureUtils.h"
#include "../Rendering/ArenaRenderUtils.h"

Color LoadSaveUiView::getEntryTextColor()
{
	return Color::White;
}

Int2 LoadSaveUiView::getEntryCenterPoint(int index)
{
	return Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, 8 + (index * 14));
}

TextureAsset LoadSaveUiView::getPaletteTextureAsset()
{
	return TextureAsset(std::string(ArenaPaletteName::Default));
}

TextureAsset LoadSaveUiView::getLoadSaveTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::LoadSave));
}

UiTextureID LoadSaveUiView::allocBackgroundTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = LoadSaveUiView::getLoadSaveTextureAsset();
	const TextureAsset paletteTextureAsset = LoadSaveUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for load/save background \"" + textureAsset.filename + "\".");
	}

	return textureID;
}
