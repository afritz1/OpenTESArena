#ifndef MAIN_MENU_PANEL_H
#define MAIN_MENU_PANEL_H

#include "Panel.h"

class MainMenuPanel : public Panel
{
public:
	MainMenuPanel(Game &game);
	~MainMenuPanel() override;

	bool init();
};

#endif
