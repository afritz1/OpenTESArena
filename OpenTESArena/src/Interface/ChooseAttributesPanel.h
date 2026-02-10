#ifndef CHOOSE_ATTRIBUTES_PANEL_H
#define CHOOSE_ATTRIBUTES_PANEL_H

#include "Panel.h"

class ChooseAttributesPanel : public Panel
{
public:
	ChooseAttributesPanel(Game &game);
	~ChooseAttributesPanel() override;

	bool init();
};

#endif
