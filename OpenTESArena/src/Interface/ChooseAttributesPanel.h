#ifndef CHOOSE_ATTRIBUTES_PANEL_H
#define CHOOSE_ATTRIBUTES_PANEL_H

#include <string>

#include "Panel.h"

class Button;
class CharacterClass;
class TextBox;

enum class CharacterGenderName;
enum class CharacterRaceName;

class ChooseAttributesPanel : public Panel
{
private:
	std::unique_ptr<TextBox> titleTextBox;
	std::unique_ptr<Button> backToRaceButton, acceptButton;
	std::unique_ptr<CharacterClass> charClass;
	std::unique_ptr<CharacterGenderName> gender;
	std::unique_ptr<CharacterRaceName> raceName;
	std::string name;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	ChooseAttributesPanel(GameState *gameState, CharacterGenderName gender,
		const CharacterClass &charClass, const std::string &name,
		CharacterRaceName raceName);
	virtual ~ChooseAttributesPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Surface *dst, const SDL_Rect *letterbox) override;
};

#endif
