#ifndef GAME_WORLD_PANEL_H
#define GAME_WORLD_PANEL_H

#include <array>
#include <vector>

#include "Button.h"
#include "Panel.h"
#include "TextBox.h"
#include "../Game/Physics.h"
#include "../Math/Rect.h"
#include "../Media/TextureUtils.h"
#include "../World/VoxelDefinition.h"
#include "../World/VoxelUtils.h"

// When the GameWorldPanel is active, the game world is ticking.

// There are two desired kinds of interfaces:
// - The original: compass, portrait, stat bars, and buttons with original mouse.
// - A modern version: only compass and stat bars with free-look mouse.

class Player;
class Renderer;
class TextureManager;

class GameWorldPanel : public Panel
{
private:
	std::unique_ptr<TextBox> playerNameTextBox;
	Button<Game&> characterSheetButton, statusButton,
		logbookButton, pauseButton;
	Button<Player&> drawWeaponButton;
	Button<> stealButton, magicButton, useItemButton, campButton;
	Button<GameWorldPanel&> scrollUpButton, scrollDownButton;
	Button<Game&, bool> mapButton;
	std::array<Rect, 9> nativeCursorRegions;
	std::vector<Int2> weaponOffsets;

	// Helper functions for various UI textures.
	static TextureBuilderID getGameWorldInterfaceTextureBuilderID(TextureManager &textureManager);
	TextureBuilderID getCompassFrameTextureBuilderID() const;
	TextureBuilderID getCompassSliderTextureBuilderID() const;
	TextureBuilderID getPlayerPortraitTextureBuilderID(const std::string &portraitsFilename, int portraitID) const;
	TextureBuilderID getStatusGradientTextureBuilderID(int gradientID) const;
	TextureBuilderID getNoSpellTextureBuilderID() const;
	TextureBuilderID getWeaponTextureBuilderID(const std::string &weaponFilename, int index) const;

	// Modifies the values in the native cursor regions array so rectangles in
	// the current window correctly represent regions for different arrow cursors.
	void updateCursorRegions(int width, int height);

	// Sets whether to change the mouse input for modern mode.
	void setFreeLookActive(bool active);

	// Handles input for the player camera.
	void handlePlayerTurning(double dt, const Int2 &mouseDelta);

	// Handles input for player movement in the game world.
	void handlePlayerMovement(double dt);

	// Handles input for the player's attack. Takes the change in mouse position since
	// the previous frame.
	void handlePlayerAttack(const Int2 &mouseDelta);

	// Handles the behavior of the player clicking in the game world. "primaryClick" is
	// true for left clicks, false for right clicks.
	void handleClickInWorld(const Int2 &nativePoint, bool primaryClick, bool debugFadeVoxel);

	// Handles changing night-light-related things on and off.
	void handleNightLightChange(bool active);

	// Sends an "on voxel enter" message for the given voxel and triggers any text or
	// sound events.
	void handleTriggers(const NewInt2 &voxel);

	// Handles updating of doors that are not closed.
	void handleDoors(double dt, const Double2 &playerPos);

	// Handles the behavior for when the player activates a *MENU block and transitions
	// from one world to another (i.e., from an interior to an exterior).
	void handleWorldTransition(const Physics::Hit &hit, int menuID);

	// Checks the given voxel to see if it's a transition voxel (i.e., level up/down),
	// and changes the current level if it is.
	void handleLevelTransition(const NewInt2 &playerVoxel, const NewInt2 &transitionVoxel);

	// Draws a tooltip sitting on the top left of the game interface.
	void drawTooltip(const std::string &text, Renderer &renderer);

	// Draws the compass for some given player direction in the XZ plane.
	void drawCompass(const NewDouble2 &direction, TextureManager &textureManager,
		Renderer &renderer);

	// Draws some debug profiler text.
	void drawProfiler(int profilerLevel, Renderer &renderer);
public:
	// Constructs the game world panel. The GameData object in Game must be initialized.
	GameWorldPanel(Game &game);
	virtual ~GameWorldPanel();

	// Gets the center of the screen for pop-up related functions. The position depends on
	// whether modern interface mode is set.
	static Int2 getInterfaceCenter(bool modernInterface, TextureManager &textureManager);

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void onPauseChanged(bool paused) override;
	virtual void resize(int windowWidth, int windowHeight) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
	virtual void renderSecondary(Renderer &renderer) override;
};

#endif
