#ifndef AUTOMAP_PANEL_H
#define AUTOMAP_PANEL_H

#include "Panel.h"

class Button;
class TextBox;

class AutomapPanel : public Panel
{
private:
	std::unique_ptr<Button> backToGameButton;
	// Name of location...
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	AutomapPanel(GameState *gameState);
	virtual ~AutomapPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Renderer *renderer, const SDL_Rect *letterbox) override;
};

#endif
