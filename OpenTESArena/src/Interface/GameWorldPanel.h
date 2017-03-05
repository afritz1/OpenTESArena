#ifndef GAME_WORLD_PANEL_H
#define GAME_WORLD_PANEL_H

#include <array>

#include "Panel.h"
#include "../Math/Rect.h"

// When the GameWorldPanel is active, the game world is ticking.

// There are two desired kinds of interfaces:
// - The original: compass, portrait, stat bars, and buttons with original mouse.
// - A modern version: only compass and stat bars with free-look mouse.

class Button;
class Renderer;
class TextBox;

enum class PlayerInterface;

class GameWorldPanel : public Panel
{
private:
	std::unique_ptr<TextBox> playerNameTextBox;
	std::unique_ptr<Button> automapButton, characterSheetButton, logbookButton, 
		pauseButton, worldMapButton;
	std::array<Rect, 9> nativeCursorRegions;
	PlayerInterface playerInterface;

	// Modifies the values in the native cursor regions array so rectangles in
	// the current window correctly represent regions for different arrow cursors.
	void updateCursorRegions(int width, int height);

	// Handles input for the player camera.
	void handlePlayerTurning(double dt);
	
	// Handles input for player movement in the game world.
	void handlePlayerMovement(double dt);

	// Draws a tooltip sitting on the top left of the game interface.
	void drawTooltip(const std::string &text, Renderer &renderer);

	// Draws some debug text.
	void drawDebugText(Renderer &renderer);
public:
	// Constructs the game world panel. The GameData object in Game must be
	// initialized.
	GameWorldPanel(Game *game);
	virtual ~GameWorldPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
