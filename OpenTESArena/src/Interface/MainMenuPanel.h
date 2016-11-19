#ifndef MAIN_MENU_PANEL_H
#define MAIN_MENU_PANEL_H

#include "Panel.h"

class Button;
class Renderer;

class MainMenuPanel : public Panel
{
private:
	std::unique_ptr<Button> loadButton, newButton, exitButton;
public:
	MainMenuPanel(Game *game);
	virtual ~MainMenuPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
