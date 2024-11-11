#ifndef MAIN_QUEST_SPLASH_UI_VIEW_H
#define MAIN_QUEST_SPLASH_UI_VIEW_H

#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../Utilities/Color.h"

class Game;

struct TextureAsset;

namespace MainQuestSplashUiView
{
	const std::string DescriptionFontName = ArenaFontName::Teeny;
	const Color DescriptionTextColor(195, 158, 0);
	constexpr TextAlignment DescriptionTextAlignment = TextAlignment::TopCenter;
	constexpr int DescriptionLineSpacing = 1;

	// @todo: should use center point instead I think
	int getDescriptionTextBoxX(int textWidth);
	int getDescriptionTextBoxY();

	TextBox::InitInfo getDescriptionTextBoxInitInfo(const std::string_view text, const FontLibrary &fontLibrary);

	constexpr int ExitButtonX = 272;
	constexpr int ExitButtonY = 183;
	constexpr int ExitButtonWidth = 43;
	constexpr int ExitButtonHeight = 13;

	TextureAsset getSplashTextureAsset(Game &game, int provinceID);

	UiTextureID allocSplashTextureID(Game &game, int provinceID);
}

#endif
