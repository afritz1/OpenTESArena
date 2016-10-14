#ifndef GAME_WORLD_PANEL_H
#define GAME_WORLD_PANEL_H

#include <array>
#include <memory>

#include "Panel.h"
#include "../Math/Float2.h"

// When the GameWorldPanel is active, the game world is ticking.

// There are two desired kinds of interfaces:
// - The original: compass, portrait, stat bars, and buttons.
// - A modern version: only compass and stat bars.

// The original interface needs the mouse cursor visible.
// The modern interface does not need the mouse cursor visible.

class Button;
class Rect;
class Renderer;
class TextBox;

class GameWorldPanel : public Panel
{
private:
	std::unique_ptr<TextBox> playerNameTextBox;
	std::unique_ptr<Button> automapButton, characterSheetButton, logbookButton, 
		pauseButton, worldMapButton;
	std::array<std::unique_ptr<Rect>, 9> nativeCursorRegions;

	// Gets the current strength of movement and rotation based on where the
	// cursor is in the window. X is used for turning or strafing, and Y is used 
	// for going forward or backward.
	Float2d getMotionMagnitudes(const Int2 &nativePoint);

	// Modifies the values in the native cursor regions array so rectangles in
	// the current window correctly represent regions for different arrow cursors.
	void updateCursorRegions(int width, int height);
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	// Constructs the game world panel. The GameData object in GameState must be
	// initialized.
	GameWorldPanel(GameState *gameState);
	virtual ~GameWorldPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(Renderer &renderer) override;
};

#endif
