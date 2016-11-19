#ifndef CHOOSE_RACE_PANEL_H
#define CHOOSE_RACE_PANEL_H

#include <string>

#include "Panel.h"

// Skip the nitty-gritty details like the confirmation box and province details
// for now. Just click on a province and go.

class Button;
class CharacterClass;
class Renderer;
class TextBox;

enum class CharacterGenderName;
enum class CharacterRaceName;
enum class ProvinceName;

struct SDL_Texture;

class ChooseRacePanel : public Panel
{
private:
	SDL_Texture *parchment;
	std::unique_ptr<TextBox> initialTextBox;
	std::unique_ptr<Button> backToGenderButton, acceptButton;
	std::unique_ptr<CharacterClass> charClass;
	std::unique_ptr<CharacterGenderName> gender;
	std::unique_ptr<CharacterRaceName> raceName; // Null until a province is clicked.
	std::string name;
	bool initialTextBoxVisible;

	void drawProvinceTooltip(ProvinceName provinceName, Renderer &renderer);
public:
	ChooseRacePanel(GameState *gameState, const CharacterClass &charClass, 
		const std::string &name, CharacterGenderName gender);
	virtual ~ChooseRacePanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
