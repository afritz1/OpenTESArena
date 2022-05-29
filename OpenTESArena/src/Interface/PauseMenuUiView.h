#ifndef PAUSE_MENU_UI_VIEW_H
#define PAUSE_MENU_UI_VIEW_H

#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/TextureUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"

struct TextureAsset;

namespace PauseMenuUiView
{
	const std::string VolumeFontName = ArenaFontName::Arena;
	const Color VolumeColor(12, 73, 16);
	constexpr TextAlignment VolumeTextAlignment = TextAlignment::MiddleCenter;

	const Int2 SoundTextBoxCenterPoint(54, 96);
	const Int2 MusicTextBoxCenterPoint(127, 96);

	TextBox::InitInfo getSoundTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBox::InitInfo getMusicTextBoxInitInfo(const FontLibrary &fontLibrary);
	
	const Int2 OptionsTextBoxCenterPoint(235, 96);
	const std::string OptionsButtonFontName = ArenaFontName::Arena;
	const Color OptionsButtonTextColor(215, 158, 4);
	constexpr TextAlignment OptionsButtonTextAlignment = TextAlignment::MiddleCenter;
	constexpr TextureUtils::PatternType OptionsButtonPatternType = TextureUtils::PatternType::Custom1;
	const Color OptionsButtonTextShadowColor(101, 77, 24);
	constexpr int OptionsButtonTextShadowOffsetX = -1;
	constexpr int OptionsButtonTextShadowOffsetY = 1;

	TextBox::InitInfo getOptionsTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);

	Rect getNewGameButtonRect();
	Rect getLoadButtonRect();
	Rect getSaveButtonRect();
	Rect getExitButtonRect();
	Rect getResumeButtonRect();
	Rect getOptionsButtonRect();
	Rect getSoundUpButtonRect();
	Rect getSoundDownButtonRect();
	Rect getMusicUpButtonRect();
	Rect getMusicDownButtonRect();

	TextureAsset getBackgroundPaletteTextureAsset();
	TextureAsset getBackgroundTextureAsset();

	UiTextureID allocBackgroundTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocOptionsButtonTexture(TextureManager &textureManager, Renderer &renderer);
}

#endif
