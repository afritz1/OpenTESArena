#ifndef CHOOSE_NAME_PANEL_H
#define CHOOSE_NAME_PANEL_H

#include "Panel.h"

class Button;
class CharacterClass;
class Surface;
class TextBox;

enum class CharacterGenderName;

class ChooseNamePanel : public Panel
{
private:
	// static "default typing cursor position"

	// vector of characters, converted to surfaces when the textbox is updated.
	std::unique_ptr<TextBox> titleTextBox;
	std::unique_ptr<Button> backToClassButton, acceptButton;
	std::unique_ptr<CharacterGenderName> gender;
	std::unique_ptr<CharacterClass> charClass;
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
