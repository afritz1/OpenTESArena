#ifndef OPTIONS_PANEL_H
#define OPTIONS_PANEL_H

#include "Panel.h"

// For now, the options menu will only be accessible from the pause menu.

class Button;
class TextBox;

class OptionsPanel : public Panel
{
private:
	std::unique_ptr<TextBox> titleTextBox;
	std::unique_ptr<Button> backToPauseButton;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	OptionsPanel(GameState *gameState);
	virtual ~OptionsPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Surface *dst, const SDL_Rect *letterbox) override;
};

#endif
