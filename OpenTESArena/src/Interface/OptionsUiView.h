#ifndef OPTIONS_UI_VIEW_H
#define OPTIONS_UI_VIEW_H

#include "../Assets/TextureUtils.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../Utilities/Color.h"

namespace OptionsUiView
{
	const Color BackgroundColor(60, 60, 68);
	const Color HighlightColor = BackgroundColor + Color(20, 20, 20);

	const Rect getTabRect(int index);
	const Rect getListRect();
	const Int2 getDescriptionXY();

	const Int2 BackButtonTextBoxCenterPoint(
		ArenaRenderUtils::SCREEN_WIDTH - 30,
		ArenaRenderUtils::SCREEN_HEIGHT - 15);
	const std::string BackButtonFontName = ArenaFontName::Arena;
	Color getBackButtonTextColor();
	constexpr TextAlignment BackButtonTextAlignment = TextAlignment::MiddleCenter;

	TextBoxInitInfo getBackButtonTextBoxInitInfo(const std::string_view text, const FontLibrary &fontLibrary);

	Rect getBackButtonRect();

	const std::string TabFontName = ArenaFontName::Arena;
	Color getTabTextColor();
	constexpr TextAlignment TabTextAlignment = TextAlignment::MiddleCenter;
	constexpr TextureUtils::PatternType TabBackgroundPatternType = TextureUtils::PatternType::Custom1;

	const std::string OptionTextBoxFontName = ArenaFontName::Arena;
	Color getOptionTextBoxColor();
	constexpr TextAlignment OptionTextBoxTextAlignment = TextAlignment::MiddleLeft;

	const std::string DescriptionTextFontName = ArenaFontName::Arena;
	Color getDescriptionTextColor();
	constexpr TextAlignment DescriptionTextAlignment = TextAlignment::TopLeft;

	TextBoxInitInfo getTabTextBoxInitInfo(int index, const std::string_view text, const FontLibrary &fontLibrary);
	TextBoxInitInfo getOptionTextBoxInitInfo(int index, const FontLibrary &fontLibrary);
	TextBoxInitInfo getDescriptionTextBoxInitInfo(const FontLibrary &fontLibrary);

	UiTextureID allocBackgroundTexture(Renderer &renderer);
	UiTextureID allocTabTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocHighlightTexture(Renderer &renderer);
	UiTextureID allocBackButtonTexture(TextureManager &textureManager, Renderer &renderer);
}

#endif
