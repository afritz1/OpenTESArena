#include "OptionsUiView.h"
#include "../Rendering/Renderer.h"

UiTextureID OptionsUiView::allocBackgroundTexture(Renderer &renderer)
{
	// @todo what if we used the parchment generated texture instead? or starry night

	constexpr int width = 1;// ArenaRenderUtils::SCREEN_WIDTH; // Reduced to 1x1 since it's a solid color for now
	constexpr int height = 1;// ArenaRenderUtils::SCREEN_HEIGHT;
	const UiTextureID textureID = renderer.createUiTexture(width, height);
	if (textureID < 0)
	{
		DebugCrash("Couldn't create UI texture for options menu background.");
	}

	LockedTexture lockedTexture = renderer.lockUiTexture(textureID);
	if (!lockedTexture.isValid())
	{
		DebugCrash("Couldn't lock texels for updating options menu background.");
	}

	Span2D<uint32_t> texels = lockedTexture.getTexels32();
	const uint32_t color = Color(60, 60, 68).toRGBA();
	texels.fill(color);
	renderer.unlockUiTexture(textureID);

	return textureID;
}

UiTextureID OptionsUiView::allocHighlightTexture(Renderer &renderer)
{
	const int width = 254;
	const int height = 9;
	const UiTextureID textureID = renderer.createUiTexture(width, height);
	if (textureID < 0)
	{
		DebugCrash("Couldn't create UI texture for highlighted option.");
	}

	LockedTexture lockedTexture = renderer.lockUiTexture(textureID);
	if (!lockedTexture.isValid())
	{
		DebugCrash("Couldn't lock texels for updating highlighted option.");
	}

	Span2D<uint32_t> texels = lockedTexture.getTexels32();
	const uint32_t color = Color(80, 80, 88).toRGBA();
	texels.fill(color);
	renderer.unlockUiTexture(textureID);

	return textureID;
}
