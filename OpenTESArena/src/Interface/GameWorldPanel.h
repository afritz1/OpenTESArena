#ifndef GAME_WORLD_PANEL_H
#define GAME_WORLD_PANEL_H

#include <array>
#include <vector>

#include "Button.h"
#include "Panel.h"
#include "../Math/Rect.h"

// When the GameWorldPanel is active, the game world is ticking.

// There are two desired kinds of interfaces:
// - The original: compass, portrait, stat bars, and buttons with original mouse.
// - A modern version: only compass and stat bars with free-look mouse.

class Player;
class Renderer;
class TextBox;

class GameWorldPanel : public Panel
{
private:
	std::unique_ptr<TextBox> playerNameTextBox;
	std::unique_ptr<Button<Game&>> characterSheetButton, statusButton,
		logbookButton, pauseButton;
	std::unique_ptr<Button<Player&>> drawWeaponButton;
	std::unique_ptr<Button<>> stealButton, magicButton, useItemButton, campButton;
	std::unique_ptr<Button<GameWorldPanel*>> scrollUpButton, scrollDownButton;
	std::unique_ptr<Button<Game&, bool>> mapButton;
	std::array<Rect, 9> nativeCursorRegions;
	std::vector<Int2> weaponOffsets;

	// Display texts with their associated time remaining. The "trigger text" is for
	// lore messages from voxels. The "action text" is for descriptions of what the 
	// player is doing. The "effect text" appears when the player is hit with some 
	// negative effect (disease, drunk, silence, etc.).
	std::pair<double, std::unique_ptr<TextBox>> triggerText, actionText, effectText;

	// Modifies the values in the native cursor regions array so rectangles in
	// the current window correctly represent regions for different arrow cursors.
	void updateCursorRegions(int width, int height);

	// Handles input for the player camera.
	void handlePlayerTurning(double dt, const Int2 &mouseDelta);

	// Handles input for player movement in the game world.
	void handlePlayerMovement(double dt);

	// Handles input for the player's attack. Takes the change in mouse position since
	// the previous frame.
	void handlePlayerAttack(const Int2 &mouseDelta);

	// Sends an "on voxel enter" message for the given voxel and triggers any text or
	// sound events.
	void handleTriggers(const Int2 &voxel);

	// Draws a tooltip sitting on the top left of the game interface.
	void drawTooltip(const std::string &text, Renderer &renderer);

	// Draws some debug text.
	void drawDebugText(Renderer &renderer);
public:
	// Constructs the game world panel. The GameData object in Game must be
	// initialized.
	GameWorldPanel(Game &game);
	virtual ~GameWorldPanel();

	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void resize(int windowWidth, int windowHeight) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
