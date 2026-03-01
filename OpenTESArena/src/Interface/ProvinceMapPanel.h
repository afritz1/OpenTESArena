#ifndef PROVINCE_MAP_PANEL_H
#define PROVINCE_MAP_PANEL_H

#include "Panel.h"

class ProvinceMapPanel : public Panel
{
public:
	ProvinceMapPanel(Game &game);
	~ProvinceMapPanel() override;

	bool init();
};

#endif
