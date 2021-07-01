#include "OptionsUiView.h"

Color OptionsUiView::getTitleTextColor()
{
	return Color::White;
}

TextBox::InitInfo OptionsUiView::getTitleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		OptionsUiView::TitleTextBoxCenterPoint,
		OptionsUiView::TitleFontName,
		OptionsUiView::getTitleTextColor(),
		OptionsUiView::TitleTextAlignment,
		fontLibrary);
}

Color OptionsUiView::getBackToPauseMenuTextColor()
{
	return Color::White;
}

TextBox::InitInfo OptionsUiView::getBackToPauseMenuTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		OptionsUiView::BackToPauseMenuTextBoxCenterPoint,
		OptionsUiView::BackToPauseMenuFontName,
		OptionsUiView::getBackToPauseMenuTextColor(),
		OptionsUiView::BackToPauseMenuTextAlignment,
		fontLibrary);
}

Color OptionsUiView::getTabTextColor()
{
	return Color::White;
}

Color OptionsUiView::getOptionTextBoxColor()
{
	return Color::White;
}

Color OptionsUiView::getDescriptionTextColor()
{
	return Color::White;
}
