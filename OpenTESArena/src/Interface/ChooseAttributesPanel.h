#ifndef CHOOSE_ATTRIBUTES_PANEL_H
#define CHOOSE_ATTRIBUTES_PANEL_H

#include <string>

#include "Panel.h"

// This panel is for choosing character creation attributes and the portrait.

// I don't think it will be used for level-up purposes.

class Button;
class CharacterClass;
class TextBox;

enum class CharacterGenderName;
enum class CharacterRaceName;

class ChooseAttributesPanel : public Panel
{
private:
	std::unique_ptr<TextBox> instructionsTextBox, nameTextBox, raceTextBox, classTextBox;
	std::unique_ptr<Button> backToRaceButton, doneButton;
	std::unique_ptr<CharacterClass> charClass;
	std::unique_ptr<CharacterGenderName> gender;
	std::unique_ptr<CharacterRaceName> raceName;
	std::string name;
	int portraitIndex;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	ChooseAttributesPanel(GameState *gameState, const CharacterClass &charClass, 
		const std::string &name, CharacterGenderName gender, CharacterRaceName raceName);
	virtual ~ChooseAttributesPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Renderer *renderer, const SDL_Rect *letterbox) override;
};

#endif
