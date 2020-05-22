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

class ChooseAttributesPanel : public Panel
{
private:
	std::unique_ptr<TextBox> nameTextBox, raceTextBox, classTextBox;
	Button<Game&> backToRaceButton, doneButton;
	Button<ChooseAttributesPanel&, bool> portraitButton;
	std::vector<Int2> headOffsets;
	CharacterClass charClass;
	bool male;
	int raceID;
	std::string name;
	int portraitID;
	bool canChangePortrait;
public:
	ChooseAttributesPanel(Game &game, const CharacterClass &charClass, 
		const std::string &name, bool male, int raceID);
	virtual ~ChooseAttributesPanel() = default;

	virtual Panel::CursorData getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
