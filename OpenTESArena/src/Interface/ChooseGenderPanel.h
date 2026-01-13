#ifndef CHOOSE_GENDER_PANEL_H
#define CHOOSE_GENDER_PANEL_H

#include "Panel.h"

class ChooseGenderPanel : public Panel
{
public:
	ChooseGenderPanel(Game &game);
	~ChooseGenderPanel() override;

	bool init();
};

#endif
