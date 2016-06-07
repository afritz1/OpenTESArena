#ifndef CHOOSE_NAME_PANEL_H
#define CHOOSE_NAME_PANEL_H

#include "Panel.h"

// I looked into SDL_StartTextInput(), but I don't quite get it yet. I'll just
// listen for plain English characters for now, since international characters are
// way outside the project scope right now.

// No numbers or symbols (i.e., @, #, $) are allowed in the name for now.

class Button;
class CharacterClass;
class Surface;
class TextBox;

enum class CharacterGenderName;

class ChooseNamePanel : public Panel
{
private:
	static const int MAX_NAME_LENGTH;

	std::unique_ptr<Surface> parchment;
	std::unique_ptr<TextBox> titleTextBox, nameTextBox;
	std::unique_ptr<Button> backToClassButton, acceptButton;
	std::unique_ptr<CharacterGenderName> gender;
	std::unique_ptr<CharacterClass> charClass;
	std::string name;	
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	ChooseNamePanel(GameState *gameState, CharacterGenderName gender, 
		const CharacterClass &charClass);
	virtual ~ChooseNamePanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Surface *dst, const SDL_Rect *letterbox) override;
};

#endif
