#ifndef CHOOSE_GENDER_PANEL_H
#define CHOOSE_GENDER_PANEL_H

#include <string>

#include "Panel.h"

class Button;
class CharacterClass;
class Renderer;
class TextBox;

struct SDL_Texture;

class ChooseGenderPanel : public Panel
{
private:
	SDL_Texture *parchment;
	std::unique_ptr<TextBox> genderTextBox, maleTextBox, femaleTextBox;
	std::unique_ptr<Button> backToNameButton, maleButton, femaleButton;
	std::unique_ptr<CharacterClass> charClass;
	std::string name;
public:
	ChooseGenderPanel(GameState *gameState, const CharacterClass &charClass,
		const std::string &name);
	virtual ~ChooseGenderPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
