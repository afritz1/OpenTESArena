#ifndef CHARACTER_EQUIPMENT_PANEL_H
#define CHARACTER_EQUIPMENT_PANEL_H

#include "Panel.h"

class Button;
class CharacterClass;
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
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	CharacterEquipmentPanel(GameState *gameState);
	virtual ~CharacterEquipmentPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(Renderer &renderer) override;
};

#endif
