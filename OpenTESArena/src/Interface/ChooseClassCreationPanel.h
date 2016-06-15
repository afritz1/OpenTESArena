#ifndef CHOOSE_CLASS_CREATION_PANEL_H
#define CHOOSE_CLASS_CREATION_PANEL_H

#include "Panel.h"

// This panel is for the "How do you wish to select your class?" screen.

// There is a rough draft version of the "Select" option already.
// The questions are not implemented yet for the "Generate" option.

// There could be new tooltips for each option. I always found it confusing what 
// the buttons meant.
// - "Answer questions" for "Generate".
// - "Choose from a list" for "Select".

class Button;
class TextBox;

class ChooseClassCreationPanel : public Panel
{
private:
	std::unique_ptr<Surface> parchment;
	std::unique_ptr<TextBox> titleTextBox, generateTextBox, selectTextBox;
	std::unique_ptr<Button> backToMainMenuButton, generateButton, selectButton;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	ChooseClassCreationPanel(GameState *gameState);
	virtual ~ChooseClassCreationPanel();
	
	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Surface *dst, const SDL_Rect *letterbox) override;
};

#endif
