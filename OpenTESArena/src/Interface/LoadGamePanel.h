#ifndef LOAD_GAME_PANEL_H
#define LOAD_GAME_PANEL_H

#include "Panel.h"

class Button;
class Surface;
class TextBox;

class LoadGamePanel : public Panel
{
private:
	// up/down arrow buttons, saved game buttons...
	std::unique_ptr<Button> backToMainMenuButton;
	std::unique_ptr<TextBox> underConstructionTextBox;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	LoadGamePanel(GameState *gameState);
	virtual ~LoadGamePanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Surface *dst, const SDL_Rect *letterbox) override;
};

#endif
