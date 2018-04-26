#ifndef LOAD_GAME_PANEL_H
#define LOAD_GAME_PANEL_H

#include <array>

#include "Button.h"
#include "Panel.h"
#include "TextBox.h"

class Renderer;
class Surface;

class LoadGamePanel : public Panel
{
private:
	std::array<std::unique_ptr<TextBox>, 10> saveTextBoxes;
	Button<Game&, int> loadButton;
	Button<Game&> backButton;
	// up/down arrow buttons, saved game buttons...

	// Returns a non-negative integer if the point is contained in a save's click area.
	static int getClickedIndex(const Int2 &point);
public:
	LoadGamePanel(Game &game);
	virtual ~LoadGamePanel() = default;

	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
