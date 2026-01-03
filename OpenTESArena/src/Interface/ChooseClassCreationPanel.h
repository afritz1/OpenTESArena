#ifndef CHOOSE_CLASS_CREATION_PANEL_H
#define CHOOSE_CLASS_CREATION_PANEL_H

#include "Panel.h"

class ChooseClassCreationPanel : public Panel
{
public:
	ChooseClassCreationPanel(Game &game);
	~ChooseClassCreationPanel() override;

	bool init();
};

#endif
