#ifndef MAIN_QUEST_SPLASH_UI_VIEW_H
#define MAIN_QUEST_SPLASH_UI_VIEW_H

#include "../Media/Color.h"
#include "../UI/FontName.h"
#include "../UI/TextAlignment.h"

namespace MainQuestSplashUiView
{
	constexpr FontName DescriptionFontName = FontName::Teeny;
	const Color DescriptionTextColor(195, 158, 0);
	constexpr TextAlignment DescriptionTextAlignment = TextAlignment::Center;
	constexpr int DescriptionLineSpacing = 1;

	int getDescriptionTextBoxX(int textWidth);
	int getDescriptionTextBoxY();

	constexpr int ExitButtonX = 272;
	constexpr int ExitButtonY = 183;
	constexpr int ExitButtonWidth = 43;
	constexpr int ExitButtonHeight = 13;
}

#endif
