#ifndef CHOOSE_NAME_PANEL_H
#define CHOOSE_NAME_PANEL_H

#include <string>

#include "Button.h"
#include "Panel.h"
#include "../Entities/CharacterClass.h"

// If Escape is pressed here, just go to the class list (even if the user went
// the answer questions path instead).

// I looked into SDL_StartTextInput(), but I don't quite get it yet. I'll just
// listen for plain English characters for now, since international characters are
// way outside the project scope right now.

// No numbers or symbols (i.e., @, #, $) are allowed in the name for now.

class Renderer;
class TextBox;
class Texture;

class ChooseNamePanel : public Panel
{
private:
	static const int MAX_NAME_LENGTH;

	std::unique_ptr<Texture> parchment;
	std::unique_ptr<TextBox> titleTextBox, nameTextBox;
	std::unique_ptr<Button<Game*>> backToClassButton;
	std::unique_ptr<Button<Game*, const CharacterClass&, const std::string&>> acceptButton;
	CharacterClass charClass;
	std::string name;
public:
	ChooseNamePanel(Game *game, const CharacterClass &charClass);
	virtual ~ChooseNamePanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
