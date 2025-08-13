#include "PauseMenuUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/TextureAsset.h"
#include "../Assets/TextureUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/Surface.h"

namespace
{
	const std::string DummyVolumeText = "100"; // Worst-case text size for sound/music volume.
}

TextBoxInitInfo PauseMenuUiView::getSoundTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithCenter(
		DummyVolumeText,
		PauseMenuUiView::SoundTextBoxCenterPoint,
		PauseMenuUiView::VolumeFontName,
		PauseMenuUiView::VolumeColor,
		PauseMenuUiView::VolumeTextAlignment,
		fontLibrary);
}

TextBoxInitInfo PauseMenuUiView::getMusicTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithCenter(
		DummyVolumeText,
		PauseMenuUiView::MusicTextBoxCenterPoint,
		PauseMenuUiView::VolumeFontName,
		PauseMenuUiView::VolumeColor,
		PauseMenuUiView::VolumeTextAlignment,
		fontLibrary);
}

TextBoxInitInfo PauseMenuUiView::getOptionsTextBoxInitInfo(const std::string_view text, const FontLibrary &fontLibrary)
{
	const TextRenderShadowInfo shadow(
		PauseMenuUiView::OptionsButtonTextShadowOffsetX,
		PauseMenuUiView::OptionsButtonTextShadowOffsetY,
		PauseMenuUiView::OptionsButtonTextShadowColor);
	return TextBoxInitInfo::makeWithCenter(
		text,
		PauseMenuUiView::OptionsTextBoxCenterPoint,
		PauseMenuUiView::OptionsButtonFontName,
		PauseMenuUiView::OptionsButtonTextColor,
		PauseMenuUiView::OptionsButtonTextAlignment,
		shadow,
		0,
		fontLibrary);
}

Rect PauseMenuUiView::getNewGameButtonRect()
{
	return Rect(0, 118, 65, 29);
}

Rect PauseMenuUiView::getLoadButtonRect()
{
	return Rect(65, 118, 64, 29);
}

Rect PauseMenuUiView::getSaveButtonRect()
{
	return Rect(129, 118, 64, 29);
}

Rect PauseMenuUiView::getExitButtonRect()
{
	return Rect(193, 118, 64, 29);
}

Rect PauseMenuUiView::getResumeButtonRect()
{
	return Rect(257, 118, 64, 29);
}

Rect PauseMenuUiView::getOptionsButtonRect()
{
	return Rect(162, 88, 145, 15);
}

Rect PauseMenuUiView::getSoundUpButtonRect()
{
	return Rect(46, 79, 17, 9);
}

Rect PauseMenuUiView::getSoundDownButtonRect()
{
	return Rect(46, 104, 17, 9);
}

Rect PauseMenuUiView::getMusicUpButtonRect()
{
	return Rect(119, 79, 17, 9);
}

Rect PauseMenuUiView::getMusicDownButtonRect()
{
	return Rect(119, 104, 17, 9);
}

TextureAsset PauseMenuUiView::getBackgroundPaletteTextureAsset()
{
	return TextureAsset(std::string(ArenaPaletteName::Default));
}

TextureAsset PauseMenuUiView::getBackgroundTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::PauseBackground));
}

UiTextureID PauseMenuUiView::allocBackgroundTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = PauseMenuUiView::getBackgroundTextureAsset();
	const TextureAsset paletteTextureAsset = PauseMenuUiView::getBackgroundPaletteTextureAsset();
	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create background texture for pause menu.");
	}

	return textureID;
}

UiTextureID PauseMenuUiView::allocOptionsButtonTexture(TextureManager &textureManager, Renderer &renderer)
{
	const Rect buttonRect = PauseMenuUiView::getOptionsButtonRect();
	const Surface surface = TextureUtils::generate(PauseMenuUiView::OptionsButtonPatternType, buttonRect.width, buttonRect.height, textureManager, renderer);
	
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
