#ifndef CHOOSE_CLASS_CREATION_PANEL_H
#define CHOOSE_CLASS_CREATION_PANEL_H

#include "Panel.h"

// This panel is for the "How do you wish to select your class?" screen.

// The questions are not implemented yet for the "Generate" option.

// There could be new tooltips for each option. I always found it confusing what 
// the buttons meant.
// - "Answer questions" for "Generate".
// - "Choose from a list" for "Select".

class Button;
class Renderer;
class TextBox;

struct SDL_Texture;

class ChooseClassCreationPanel : public Panel
{
private:
	SDL_Texture *parchment;
	std::unique_ptr<TextBox> titleTextBox, generateTextBox, selectTextBox;
	std::unique_ptr<Button> backToMainMenuButton, generateButton, selectButton;
public:
	ChooseClassCreationPanel(Game *game);
	virtual ~ChooseClassCreationPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
