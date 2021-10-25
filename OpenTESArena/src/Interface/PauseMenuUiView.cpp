#include "PauseMenuUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/TextureAssetReference.h"
#include "../Media/TextureUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/Surface.h"

namespace
{
	const std::string DummyVolumeText = "100"; // Worst-case text size for sound/music volume.
}

TextBox::InitInfo PauseMenuUiView::getSoundTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		DummyVolumeText,
		PauseMenuUiView::SoundTextBoxCenterPoint,
		PauseMenuUiView::VolumeFontName,
		PauseMenuUiView::VolumeColor,
		PauseMenuUiView::VolumeTextAlignment,
		fontLibrary);
}

TextBox::InitInfo PauseMenuUiView::getMusicTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		DummyVolumeText,
		PauseMenuUiView::MusicTextBoxCenterPoint,
		PauseMenuUiView::VolumeFontName,
		PauseMenuUiView::VolumeColor,
		PauseMenuUiView::VolumeTextAlignment,
		fontLibrary);
}

TextBox::InitInfo PauseMenuUiView::getOptionsTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary)
{
	const TextRenderUtils::TextShadowInfo shadow(
		PauseMenuUiView::OptionsButtonTextShadowOffsetX,
		PauseMenuUiView::OptionsButtonTextShadowOffsetY,
		PauseMenuUiView::OptionsButtonTextShadowColor);
	return TextBox::InitInfo::makeWithCenter(
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

TextureAssetReference PauseMenuUiView::getBackgroundPaletteTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaPaletteName::Default));
}

TextureAssetReference PauseMenuUiView::getBackgroundTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::PauseBackground));
}

UiTextureID PauseMenuUiView::allocBackgroundTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAssetReference textureAssetRef = PauseMenuUiView::getBackgroundTextureAssetRef();
	const TextureAssetReference paletteTextureAssetRef = PauseMenuUiView::getBackgroundPaletteTextureAssetRef();
	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAssetRef, paletteTextureAssetRef, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create background texture for pause menu.");
	}

	return textureID;
}

UiTextureID PauseMenuUiView::allocOptionsButtonTexture(TextureManager &textureManager, Renderer &renderer)
{
	const Rect buttonRect = PauseMenuUiView::getOptionsButtonRect();
	const Surface surface = TextureUtils::generate(PauseMenuUiView::OptionsButtonPatternType,
		buttonRect.getWidth(), buttonRect.getHeight(), textureManager, renderer);
	const BufferView2D<const uint32_t> surfaceTexelsView(
		static_cast<const uint32_t*>(surface.getPixels()), surface.getWidth(), surface.getHeight());

	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(surfaceTexelsView, &textureID))
	{
		DebugCrash("Couldn't create options button texture for pause menu.");
	}

	return textureID;
}
