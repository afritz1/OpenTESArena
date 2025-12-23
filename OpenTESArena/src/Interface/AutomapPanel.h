#ifndef AUTOMAP_PANEL_H
#define AUTOMAP_PANEL_H

#include "Panel.h"

class AutomapPanel : public Panel
{
public:
	AutomapPanel(Game &game);
	~AutomapPanel() override;

	bool init();
};

#endif
