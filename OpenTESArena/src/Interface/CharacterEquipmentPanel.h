#ifndef CHARACTER_EQUIPMENT_PANEL_H
#define CHARACTER_EQUIPMENT_PANEL_H

#include "Panel.h"
#include "../UI/Button.h"
#include "../UI/ListBox.h"
#include "../UI/TextBox.h"

class Renderer;

class CharacterEquipmentPanel : public Panel
{
private:
	TextBox playerNameTextBox, playerRaceTextBox, playerClassTextBox;
	ListBox inventoryListBox;
	Button<Game&> backToStatsButton, spellbookButton;
	Button<Game&, int> dropButton;
	Button<ListBox&> scrollDownButton, scrollUpButton;
	ScopedUiTextureRef bodyTextureRef, headTextureRef, shirtTextureRef, pantsTextureRef,
		equipmentBgTextureRef, cursorTextureRef;
public:
	CharacterEquipmentPanel(Game &game);
	~CharacterEquipmentPanel() override;

	bool init();
};

#endif
