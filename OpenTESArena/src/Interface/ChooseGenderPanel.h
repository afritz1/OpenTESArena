#ifndef CHOOSE_GENDER_PANEL_H
#define CHOOSE_GENDER_PANEL_H

#include "Panel.h"

class Button;
class CharacterClass;
class Surface;
class TextBox;

class ChooseGenderPanel : public Panel
{
private:
	std::unique_ptr<Surface> parchment;
	std::unique_ptr<TextBox> genderTextBox, maleTextBox, femaleTextBox;
	std::unique_ptr<Button> backToNameButton, maleButton, femaleButton;
	std::unique_ptr<CharacterClass> charClass;
	std::string name;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	ChooseGenderPanel(GameState *gameState, const CharacterClass &charClass,
		const std::string &name);
	virtual ~ChooseGenderPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Surface *dst, const SDL_Rect *letterbox) override;
};

#endif
