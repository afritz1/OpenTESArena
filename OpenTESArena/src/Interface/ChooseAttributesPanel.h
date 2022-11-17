#ifndef CHOOSE_ATTRIBUTES_PANEL_H
#define CHOOSE_ATTRIBUTES_PANEL_H

#include <map>
#include <string>
#include <vector>

#include "Panel.h"
#include "../Entities/PrimaryAttributeName.h"
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
	std::map<PrimaryAttributeName, TextBox> attributeTextBoxes;
	Button<Game&, bool*> doneButton;
	Button<Game&, bool> portraitButton;
	Buffer<ScopedUiTextureRef> headTextureRefs;
	ScopedUiTextureRef bodyTextureRef, shirtTextureRef, pantsTextureRef, statsBgTextureRef, cursorTextureRef;
	bool attributesAreSaved; // Whether attributes have been saved and the player portrait can now be changed.
public:
	ChooseAttributesPanel(Game &game);
	~ChooseAttributesPanel() override = default;

	bool init();
};

#endif
