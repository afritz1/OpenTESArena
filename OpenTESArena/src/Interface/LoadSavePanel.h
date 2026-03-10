#ifndef LOAD_SAVE_PANEL_H
#define LOAD_SAVE_PANEL_H

#include "Panel.h"

class LoadSavePanel : public Panel
{
public:
	LoadSavePanel(Game &game);
	~LoadSavePanel() override;

	bool init();
};

#endif
