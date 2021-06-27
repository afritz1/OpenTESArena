#ifndef OPTIONS_UI_VIEW_H
#define OPTIONS_UI_VIEW_H

#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/TextureUtils.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/FontName.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"

namespace OptionsUiView
{
	const Color BackgroundColor(60, 60, 68);
	const Color HighlightColor = BackgroundColor + Color(20, 20, 20);

	// Screen locations for various options things.
	const Int2 TabsOrigin(3, 38);
	const Int2 TabsDimensions(54, 16);
	const Int2 ListOrigin(
		TabsOrigin.x + TabsDimensions.x + 5,
		TabsOrigin.y);
	const Int2 ListDimensions(
		254,
		TabsDimensions.y * 5);
	const Int2 DescriptionOrigin(
		TabsOrigin.x + 2,
		TabsOrigin.y + (TabsDimensions.y * 5) + 4);

	const Rect GraphicsTabRect(
		TabsOrigin.x,
		TabsOrigin.y,
		TabsDimensions.x,
		TabsDimensions.y);
	const Rect AudioTabRect(
		TabsOrigin.x,
		TabsOrigin.y + TabsDimensions.y,
		TabsDimensions.x,
		TabsDimensions.y);
	const Rect InputTabRect(
		TabsOrigin.x,
		TabsOrigin.y + (TabsDimensions.y * 2),
		TabsDimensions.x,
		TabsDimensions.y);
	const Rect MiscTabRect(
		TabsOrigin.x,
		TabsOrigin.y + (TabsDimensions.y * 3),
		TabsDimensions.x,
		TabsDimensions.y);
	const Rect DevTabRect(
		TabsOrigin.x,
		TabsOrigin.y + (TabsDimensions.y * 4),
		TabsDimensions.x,
		TabsDimensions.y);

	const Int2 TitleTextBoxCenterPoint(160, 24);
	constexpr FontName TitleFontName = FontName::A;
	Color getTitleTextColor();  // Global initialization order workaround.
	constexpr TextAlignment TitleTextAlignment = TextAlignment::Center;

	TextBox::InitInfo getTitleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);

	const Int2 BackToPauseMenuTextBoxCenterPoint(
		ArenaRenderUtils::SCREEN_WIDTH - 30,
		ArenaRenderUtils::SCREEN_HEIGHT - 15);
	constexpr FontName BackToPauseMenuFontName = FontName::Arena;
	Color getBackToPauseMenuTextColor();
	constexpr TextAlignment BackToPauseMenuTextAlignment = TextAlignment::Center;

	TextBox::InitInfo getBackToPauseMenuTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);

	const Int2 BackToPauseMenuButtonCenterPoint = BackToPauseMenuTextBoxCenterPoint;
	constexpr int BackToPauseMenuButtonWidth = 40;
	constexpr int BackToPauseMenuButtonHeight = 16;

	constexpr FontName TabFontName = FontName::Arena;
	Color getTabTextColor();
	constexpr TextAlignment TabTextAlignment = TextAlignment::Center;
	constexpr TextureUtils::PatternType TabBackgroundPatternType = TextureUtils::PatternType::Custom1;

	constexpr FontName OptionTextBoxFontName = FontName::Arena;
	Color getOptionTextBoxColor();
	constexpr TextAlignment OptionTextBoxTextAlignment = TextAlignment::Left;

	constexpr FontName DescriptionTextFontName = FontName::Arena;
	Color getDescriptionTextColor();
	constexpr TextAlignment DescriptionTextAlignment = TextAlignment::Left;
}

#endif
