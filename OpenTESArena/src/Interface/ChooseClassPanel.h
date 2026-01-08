#ifndef CHOOSE_CLASS_PANEL_H
#define CHOOSE_CLASS_PANEL_H

#include "Panel.h"

class ChooseClassPanel : public Panel
{
public:
	ChooseClassPanel(Game &game);
	~ChooseClassPanel() override;

	bool init();
};

#endif
