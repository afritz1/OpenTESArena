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
	std::unique_ptr<Button<Game*>> backButton;
	// up/down arrow buttons, saved game buttons...
public:
	LoadGamePanel(Game *game);
	virtual ~LoadGamePanel();

	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
