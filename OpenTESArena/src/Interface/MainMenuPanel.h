#ifndef MAIN_MENU_PANEL_H
#define MAIN_MENU_PANEL_H

#include "Panel.h"

class Button;

class MainMenuPanel : public Panel
{
private:
	std::unique_ptr<Button> loadButton, newButton, exitButton;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	MainMenuPanel(GameState *gameState);
	virtual ~MainMenuPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Surface *dst, const SDL_Rect *letterbox) override;
};

#endif
