#ifndef CHARACTER_PANEL_H
#define CHARACTER_PANEL_H

#include "Panel.h"
#include "../Stats/PrimaryAttribute.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"

class Renderer;

// The character portrait panel that shows the player's attributes and derived stats.
// @todo Maybe rename this to "CharacterStatsPanel"?
class CharacterPanel : public Panel
{
private:
	TextBox nameTextBox, raceTextBox, classTextBox;
	TextBox attributeTextBoxes[PrimaryAttributes::COUNT];
	TextBox derivedAttributeTextBoxes[DerivedAttributes::COUNT];
	TextBox experienceTextBox, levelTextBox;
	TextBox healthTextBox, staminaTextBox, spellPointsTextBox;
	TextBox goldTextBox;
	Button<Game&> doneButton, nextPageButton;
	ScopedUiTextureRef bodyTextureRef, headTextureRef, shirtTextureRef, pantsTextureRef,
		statsBgTextureRef, nextPageTextureRef, cursorTextureRef;
public:
	CharacterPanel(Game &game);
	~CharacterPanel() override;

	bool init();
};

#endif
