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
public:
	CharacterEquipmentPanel(Game &game);
	~CharacterEquipmentPanel() override = default;

	bool init();

	virtual std::optional<CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
