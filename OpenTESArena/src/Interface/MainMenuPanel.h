#ifndef MAIN_MENU_PANEL_H
#define MAIN_MENU_PANEL_H

#include "Button.h"
#include "Panel.h"

class Renderer;

class MainMenuPanel : public Panel
{
private:
	Button<Game&> loadButton, newButton, fastStartButton;
	Button<> exitButton;
public:
	MainMenuPanel(Game &game);
	virtual ~MainMenuPanel();

	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
