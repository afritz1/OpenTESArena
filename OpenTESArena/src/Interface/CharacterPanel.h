#ifndef CHARACTER_PANEL_H
#define CHARACTER_PANEL_H

#include "Panel.h"
#include "../UI/Button.h"

// Maybe rename this to "CharacterStatsPanel"?

// This is the character portrait panel that shows the player's attributes and derived stats.

class Renderer;
class TextBox;

class CharacterPanel : public Panel
{
private:
	std::unique_ptr<TextBox> playerNameTextBox, playerRaceTextBox, playerClassTextBox;
	Button<Game&> doneButton, nextPageButton;
public:
	CharacterPanel(Game &game);
	~CharacterPanel() override = default;

	bool init();

	virtual std::optional<CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
