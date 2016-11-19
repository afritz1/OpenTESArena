#ifndef CHARACTER_EQUIPMENT_PANEL_H
#define CHARACTER_EQUIPMENT_PANEL_H

#include <vector>

#include "Panel.h"

class Button;
class CharacterClass;
class Int2;
class Renderer;
class TextBox;

enum class CharacterGenderName;
enum class CharacterRaceName;

class CharacterEquipmentPanel : public Panel
{
private:
	std::unique_ptr<TextBox> playerNameTextBox, playerRaceTextBox,
		playerClassTextBox;
	std::unique_ptr<Button> backToStatsButton, spellbookButton, dropButton,
		scrollDownButton, scrollUpButton;
	std::vector<Int2> headOffsets;
public:
	CharacterEquipmentPanel(GameState *gameState);
	virtual ~CharacterEquipmentPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
