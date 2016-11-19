#ifndef AUTOMAP_PANEL_H
#define AUTOMAP_PANEL_H

#include "Panel.h"

class Button;
class Renderer;
class TextBox;

class AutomapPanel : public Panel
{
private:
	std::unique_ptr<Button> backToGameButton;
	// Name of location...

	// Listen for when the LMB is held on a compass direction.
	void handleMouse(double dt);
public:
	AutomapPanel(Game *game);
	virtual ~AutomapPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
