#ifndef CHARACTER_PANEL_H
#define CHARACTER_PANEL_H

#include <vector>

#include "Button.h"
#include "Panel.h"
#include "../Math/Vector2.h"

// Maybe rename this to "CharacterStatsPanel"?

// This is the character portrait panel that shows the player's attributes and
// derived stats.

class Renderer;
class TextBox;

class CharacterPanel : public Panel
{
private:
	std::unique_ptr<TextBox> playerNameTextBox, playerRaceTextBox,
		playerClassTextBox;
	Button<Game&> doneButton, nextPageButton;
	std::vector<Int2> headOffsets;
public:
	CharacterPanel(Game &game);
	virtual ~CharacterPanel() = default;

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
