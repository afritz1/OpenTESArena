#ifndef CHARACTER_PANEL_H
#define CHARACTER_PANEL_H

#include <vector>

#include "Panel.h"

// Maybe rename this to "CharacterStatsPanel"?

// This is the character portrait panel that shows the player's attributes and
// derived stats.

class Button;
class CharacterClass;
class Int2;
class Renderer;
class TextBox;

enum class CharacterGenderName;
enum class CharacterRaceName;

class CharacterPanel : public Panel
{
private:
	std::unique_ptr<TextBox> playerNameTextBox, playerRaceTextBox,
		playerClassTextBox;
	std::unique_ptr<Button> doneButton, nextPageButton;
	std::vector<Int2> headOffsets;
public:
	CharacterPanel(GameState *gameState);
	virtual ~CharacterPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
