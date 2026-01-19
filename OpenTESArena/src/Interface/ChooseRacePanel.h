#ifndef CHOOSE_RACE_PANEL_H
#define CHOOSE_RACE_PANEL_H

#include "Panel.h"

class ChooseRacePanel : public Panel
{
private:
public:
	ChooseRacePanel(Game &game);
	~ChooseRacePanel() override;

	bool init();
};

#endif
