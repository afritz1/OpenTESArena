#ifndef MAIN_QUEST_SPLASH_PANEL_H
#define MAIN_QUEST_SPLASH_PANEL_H

#include "Panel.h"

class MainQuestSplashPanel : public Panel
{
public:
	MainQuestSplashPanel(Game &game);
	~MainQuestSplashPanel() override;

	bool init();
};

#endif
