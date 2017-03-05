#ifndef CHOOSE_RACE_PANEL_H
#define CHOOSE_RACE_PANEL_H

#include <string>

#include "Button.h"
#include "Panel.h"
#include "../Entities/CharacterClass.h"

// Skip the nitty-gritty details like the confirmation box and province details
// for now. Just click on a province and go.

class Renderer;
class TextBox;
class Texture;

enum class CharacterRaceName;
enum class GenderName;
enum class ProvinceName;

class ChooseRacePanel : public Panel
{
private:
	std::unique_ptr<Texture> parchment;
	std::unique_ptr<TextBox> initialTextBox;
	std::unique_ptr<Button<>> backToGenderButton, acceptButton;
	std::unique_ptr<CharacterRaceName> raceName; // Null until a province is clicked.
	CharacterClass charClass;
	GenderName gender;
	std::string name;
	bool initialTextBoxVisible;

	void drawProvinceTooltip(ProvinceName provinceName, Renderer &renderer);
public:
	ChooseRacePanel(Game *game, const CharacterClass &charClass, 
		const std::string &name, GenderName gender);
	virtual ~ChooseRacePanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
