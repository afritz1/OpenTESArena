#include "CommonUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"

UiTextureID CommonUiView::allocDefaultCursorTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAssetReference paletteTextureAssetRef = TextureAssetReference(std::string(ArenaPaletteName::Default));
	const TextureAssetReference textureAssetRef = TextureAssetReference(std::string(ArenaTextureName::SwordCursor));

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAssetRef, paletteTextureAssetRef, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for default cursor.");
	}

	return textureID;
}
