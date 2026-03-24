#ifndef WORLD_MAP_PANEL_H
#define WORLD_MAP_PANEL_H

#include "Panel.h"

class WorldMapPanel : public Panel
{
public:
	WorldMapPanel(Game &game);
	~WorldMapPanel() override;

	bool init();
};

#endif
