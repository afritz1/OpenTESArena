#ifndef CHARACTER_PANEL_H
#define CHARACTER_PANEL_H

#include <vector>

#include "Panel.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"

// Maybe rename this to "CharacterStatsPanel"?

// This is the character portrait panel that shows the player's attributes and derived stats.

class Renderer;
class TextBox;

class CharacterPanel : public Panel
{
private:
	TextBox playerNameTextBox, playerRaceTextBox, playerClassTextBox;
	std::vector<TextBox> playerAttributeTextBoxes;
	Button<Game&> doneButton, nextPageButton;
	ScopedUiTextureRef bodyTextureRef, headTextureRef, shirtTextureRef, pantsTextureRef,
		statsBgTextureRef, nextPageTextureRef, cursorTextureRef;
public:
	CharacterPanel(Game &game);
	~CharacterPanel() override;

	bool init();
};

#endif
