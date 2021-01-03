#ifndef CHOOSE_CLASS_CREATION_PANEL_H
#define CHOOSE_CLASS_CREATION_PANEL_H

#include <string>

#include "Button.h"
#include "Panel.h"
#include "Texture.h"

// This panel is for the "How do you wish to select your class?" screen.

// I added new tooltips for each option. I always found it confusing what 
// the buttons meant exactly.

class Renderer;
class TextBox;

class ChooseClassCreationPanel : public Panel
{
private:
	Texture parchment;
	std::unique_ptr<TextBox> titleTextBox, generateTextBox, selectTextBox;
	Button<Game&> backToMainMenuButton, generateButton, selectButton;

	void drawTooltip(const std::string &text, Renderer &renderer);
public:
	ChooseClassCreationPanel(Game &game);
	virtual ~ChooseClassCreationPanel() = default;

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
