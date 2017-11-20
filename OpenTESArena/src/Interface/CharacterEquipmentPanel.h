#ifndef CHARACTER_EQUIPMENT_PANEL_H
#define CHARACTER_EQUIPMENT_PANEL_H

#include <vector>

#include "Button.h"
#include "Panel.h"
#include "../Math/Vector2.h"

class Renderer;
class TextBox;

class CharacterEquipmentPanel : public Panel
{
private:
	std::unique_ptr<TextBox> playerNameTextBox, playerRaceTextBox,
		playerClassTextBox;
	Button<Game&> backToStatsButton;
	Button<> spellbookButton;
	Button<Game&, int> dropButton;
	Button<CharacterEquipmentPanel*> scrollDownButton, scrollUpButton;
	std::vector<Int2> headOffsets;
public:
	CharacterEquipmentPanel(Game &game);
	virtual ~CharacterEquipmentPanel();

	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
