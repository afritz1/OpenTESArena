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

enum class GenderName;

class ChooseRacePanel : public Panel
{
private:
	std::unique_ptr<Texture> parchment;
	std::unique_ptr<TextBox> initialTextBox;
	std::unique_ptr<Button<>> backToGenderButton;
	std::unique_ptr<Button<int>> acceptButton;
	CharacterClass charClass;
	GenderName gender;
	std::string name;
	bool initialTextBoxVisible;

	void drawProvinceTooltip(int provinceID, Renderer &renderer);
public:
	ChooseRacePanel(Game *game, const CharacterClass &charClass, 
		const std::string &name, GenderName gender);
	virtual ~ChooseRacePanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
