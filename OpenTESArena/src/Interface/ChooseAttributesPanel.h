#ifndef CHOOSE_ATTRIBUTES_PANEL_H
#define CHOOSE_ATTRIBUTES_PANEL_H

#include <string>
#include <vector>

#include "Panel.h"
#include "../Math/Vector2.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"

// This panel is for choosing character creation attributes and the portrait.

// I think it should be used for level-up purposes, since distributing points is
// basically identical to setting your character's original attributes.

// Maybe there could be a "LevelUpPanel" for that instead.

class Renderer;

class ChooseAttributesPanel : public Panel
{
private:
	TextBox nameTextBox, raceTextBox, classTextBox;
	Button<Game&> backToRaceButton;
	Button<Game&, bool*> doneButton;
	Button<Game&, bool> portraitButton;
	bool attributesAreSaved; // Whether attributes have been saved and the player portrait can now be changed.
public:
	ChooseAttributesPanel(Game &game);
	~ChooseAttributesPanel() override = default;

	bool init();

	virtual std::optional<CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
