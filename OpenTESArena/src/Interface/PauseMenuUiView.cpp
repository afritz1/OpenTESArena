#include "PauseMenuUiView.h"
#include "../Assets/TextureUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/Surface.h"

UiTextureID PauseMenuUiView::allocOptionsButtonTexture(TextureManager &textureManager, Renderer &renderer)
{
	constexpr Rect buttonRect(162, 88, 145, 15);
	const Surface surface = TextureUtils::generate(UiTexturePatternType::Custom1, buttonRect.width, buttonRect.height, textureManager, renderer);

	Span2D<const uint32_t> pixels = surface.getPixels();
	const UiTextureID textureID = renderer.createUiTexture(pixels.getWidth(), pixels.getHeight());
	if (textureID < 0)
	{
		DebugLogError("Couldn't create options button texture for pause menu.");
		return -1;
	}

	if (!renderer.populateUiTextureNoPalette(textureID, pixels))
	{
		DebugLogError("Couldn't populate options button texture for pause menu.");
	}

	return textureID;
}
