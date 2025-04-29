#ifndef CHOOSE_ATTRIBUTES_PANEL_H
#define CHOOSE_ATTRIBUTES_PANEL_H

#include "Panel.h"
#include "../Stats/PrimaryAttribute.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"

// For choosing character creation attributes and the portrait. I think it should be used for level-up
// purposes, since distributing points is basically identical to setting your character's original attributes.
// Maybe there could be a "LevelUpPanel" for that instead.
class ChooseAttributesPanel : public Panel
{
private:
	TextBox nameTextBox, raceTextBox, classTextBox;
	TextBox attributeTextBoxes[PrimaryAttributes::COUNT];
	TextBox experienceTextBox, levelTextBox;
	TextBox bonusPointsTextBox;
	Button<Game&, bool*> doneButton;
	Button<Game&, bool> portraitButton;
	Buffer<ScopedUiTextureRef> headTextureRefs;
	ScopedUiTextureRef bodyTextureRef, shirtTextureRef, pantsTextureRef, statsBgTextureRef, cursorTextureRef;
	ScopedUiTextureRef upDownTextureRef, bonusPointsTextureRef;
	Button<> upDownButtons[PrimaryAttributes::COUNT];
	bool attributesAreSaved; // Whether attributes have been saved and the player portrait can now be changed.
public:
	ChooseAttributesPanel(Game &game);
	~ChooseAttributesPanel() override = default;

	bool init();
};

#endif
