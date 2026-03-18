#ifndef CINEMATIC_PANEL_H
#define CINEMATIC_PANEL_H

#include "Panel.h"

class CinematicPanel : public Panel
{
public:
	CinematicPanel(Game &game);
	~CinematicPanel() override;

	bool init();
};

#endif
