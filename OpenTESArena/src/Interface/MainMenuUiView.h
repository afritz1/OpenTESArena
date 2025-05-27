#ifndef MAIN_MENU_UI_VIEW_H
#define MAIN_MENU_UI_VIEW_H

#include <string>

#include "../Assets/TextureAsset.h"
#include "../Assets/TextureUtils.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../Utilities/Color.h"

namespace MainMenuUiView
{
	Rect getLoadButtonRect();
	Rect getNewGameButtonRect();
	Rect getExitButtonRect();
	
	// Test values.
	Rect getTestButtonRect();
	constexpr TextureUtils::PatternType TestButtonPatternType = TextureUtils::PatternType::Custom1;
	const std::string TestButtonFontName = ArenaFontName::Arena;
	constexpr TextAlignment TestButtonTextAlignment = TextAlignment::MiddleCenter;

	Color getTestButtonTextColor();

	TextBoxInitInfo getTestButtonTextBoxInitInfo(const std::string_view text, const FontLibrary &fontLibrary);
	TextBoxInitInfo getTestTypeTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getTestNameTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getTestWeatherTextBoxInitInfo(const FontLibrary &fontLibrary);

	Rect getTestTypeUpButtonRect();
	Rect getTestTypeDownButtonRect();
	Rect getTestIndexUpButtonRect();
	Rect getTestIndexDownButtonRect();
	Rect getTestIndex2UpButtonRect();
	Rect getTestIndex2DownButtonRect();
	Rect getTestWeatherUpButtonRect();
	Rect getTestWeatherDownButtonRect();

	TextureAsset getBackgroundTextureAsset();
	TextureAsset getPaletteTextureAsset();
	TextureAsset getTestArrowsTextureAsset();
	TextureAsset getTestArrowsPaletteTextureAsset();

	UiTextureID allocBackgroundTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocTestArrowsTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocTestButtonTexture(TextureManager &textureManager, Renderer &renderer);
}

#endif
