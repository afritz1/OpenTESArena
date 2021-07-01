#include "PauseMenuUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/TextureAssetReference.h"

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

TextureAssetReference PauseMenuUiView::getBackgroundPaletteTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaPaletteName::Default));
}

TextureAssetReference PauseMenuUiView::getBackgroundTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::PauseBackground));
}
