#ifndef LOAD_GAME_PANEL_H
#define LOAD_GAME_PANEL_H

#include "Button.h"
#include "Panel.h"

class Renderer;
class Surface;
class TextBox;

class LoadGamePanel : public Panel
{
private:
	std::unique_ptr<TextBox> underConstructionTextBox;
	std::unique_ptr<Button<Game*>> backButton;
	// up/down arrow buttons, saved game buttons...
public:
	// This could get the game data boolean from the game state itself to determine
	// whether to return to the main menu or the pause menu.
	LoadGamePanel(Game *game);
	virtual ~LoadGamePanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
