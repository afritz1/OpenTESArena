#ifndef CHOOSE_NAME_PANEL_H
#define CHOOSE_NAME_PANEL_H

#include <string>

#include "Button.h"
#include "Panel.h"
#include "Texture.h"

// If Escape is pressed here, just go to the class list (even if the user went
// the answer questions path instead).

// I looked into SDL_StartTextInput(), but I don't quite get it yet. I'll just
// listen for plain English characters for now, since international characters are
// way outside the project scope right now.

// No numbers or symbols (i.e., @, #, $) are allowed in the name for now.

class Renderer;
class TextBox;

class ChooseNamePanel : public Panel
{
private:
	Texture parchment;
	std::unique_ptr<TextBox> titleTextBox, nameTextBox;
	Button<Game&> backToClassButton;
	Button<Game&, const std::string&> acceptButton;
	std::string name;
public:
	ChooseNamePanel(Game &game);
	virtual ~ChooseNamePanel() = default;

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
