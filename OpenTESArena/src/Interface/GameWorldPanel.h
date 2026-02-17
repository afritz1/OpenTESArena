#ifndef GAME_WORLD_PANEL_H
#define GAME_WORLD_PANEL_H

#include "Panel.h"

class GameWorldPanel : public Panel
{
public:
	GameWorldPanel(Game &game);
	~GameWorldPanel() override;

	bool init();

	virtual void onPauseChanged(bool paused) override;
};

#endif
