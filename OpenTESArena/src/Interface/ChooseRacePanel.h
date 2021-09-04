#ifndef CHOOSE_RACE_PANEL_H
#define CHOOSE_RACE_PANEL_H

#include "Panel.h"

class Renderer;

class ChooseRacePanel : public Panel
{
private:
	ScopedUiTextureRef backgroundTextureRef, noExitTextureRef, cursorTextureRef;
public:
	ChooseRacePanel(Game &game);
	~ChooseRacePanel() override = default;

	bool init();

	// Gets the initial parchment pop-up (public for the UI controller function).
	static std::unique_ptr<Panel> getInitialSubPanel(Game &game);
};

#endif
