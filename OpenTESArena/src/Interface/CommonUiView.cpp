#include "CommonUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/TextureAsset.h"
#include "../Assets/TextureManager.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"

UiTextureID CommonUiView::allocDefaultCursorTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset paletteTextureAsset = TextureAsset(std::string(ArenaPaletteName::Default));
	const TextureAsset textureAsset = TextureAsset(std::string(ArenaTextureName::SwordCursor));

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for default cursor.");
	}

	return textureID;
}

Color CommonUiView::getDebugInfoTextBoxColor()
{
	return Colors::White;
}
