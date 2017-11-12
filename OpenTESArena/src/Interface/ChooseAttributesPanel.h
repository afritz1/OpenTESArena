#ifndef CHOOSE_ATTRIBUTES_PANEL_H
#define CHOOSE_ATTRIBUTES_PANEL_H

#include <string>
#include <vector>

#include "Button.h"
#include "Panel.h"
#include "../Entities/CharacterClass.h"
#include "../Math/Vector2.h"

// This panel is for choosing character creation attributes and the portrait.

// I think it should be used for level-up purposes, since distributing points is
// basically identical to setting your character's original attributes.

// Maybe there could be a "LevelUpPanel" for that instead.

class Renderer;
class TextBox;

enum class GenderName;

class ChooseAttributesPanel : public Panel
{
private:
	std::unique_ptr<TextBox> nameTextBox, raceTextBox, classTextBox;
	std::unique_ptr<Button<Game&>> backToRaceButton, doneButton;
	std::unique_ptr<Button<ChooseAttributesPanel*, bool>> portraitButton;
	std::vector<Int2> headOffsets;
	CharacterClass charClass;
	GenderName gender;
	int raceID;
	std::string name;
	int portraitID;
public:
	ChooseAttributesPanel(Game &game, const CharacterClass &charClass, 
		const std::string &name, GenderName gender, int raceID);
	virtual ~ChooseAttributesPanel();

	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
