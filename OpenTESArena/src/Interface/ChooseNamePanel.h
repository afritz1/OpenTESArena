#ifndef CHOOSE_NAME_PANEL_H
#define CHOOSE_NAME_PANEL_H

#include "Panel.h"

class ChooseNamePanel : public Panel
{
public:
	ChooseNamePanel(Game &game);
	~ChooseNamePanel() override;

	bool init();
};

#endif
