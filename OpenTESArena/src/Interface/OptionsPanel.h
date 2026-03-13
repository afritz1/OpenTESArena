#ifndef OPTIONS_PANEL_H
#define OPTIONS_PANEL_H

#include "Panel.h"

class OptionsPanel : public Panel
{
public:
	OptionsPanel(Game &game);
	~OptionsPanel() override;

	bool init();
};

#endif
