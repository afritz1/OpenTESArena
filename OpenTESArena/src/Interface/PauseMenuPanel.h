#ifndef PAUSE_MENU_PANEL_H
#define PAUSE_MENU_PANEL_H

#include "Panel.h"

class PauseMenuPanel : public Panel
{
public:
	PauseMenuPanel(Game &game);
	~PauseMenuPanel() override;

	bool init();
};

#endif
