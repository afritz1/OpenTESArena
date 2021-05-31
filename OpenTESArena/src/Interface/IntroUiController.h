#ifndef INTRO_UI_CONTROLLER_H
#define INTRO_UI_CONTROLLER_H

class Game;

namespace IntroUiController
{
	void onIntroBookFinished(Game &game);
	void onIntroTitleFinished(Game &game);
	void onIntroQuoteFinished(Game &game);
	void onOpeningScrollFinished(Game &game);
	void onIntroStoryFinished(Game &game);
}

#endif
