#ifndef GAME_WORLD_PANEL_H
#define GAME_WORLD_PANEL_H

#include <array>
#include <vector>

#include "Panel.h"
#include "../Game/Physics.h"
#include "../Math/Rect.h"
#include "../Media/TextureUtils.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"
#include "../World/VoxelDefinition.h"
#include "../World/VoxelUtils.h"

// When the GameWorldPanel is active, the game world is ticking.

// There are two desired kinds of interfaces:
// - The original: compass, portrait, stat bars, and buttons with original mouse.
// - A modern version: only compass and stat bars with free-look mouse.

class Player;
class Renderer;
class TextureManager;
class TransitionDefinition;

class GameWorldPanel : public Panel
{
private:
	std::unique_ptr<TextBox> playerNameTextBox;
	Button<Game&> characterSheetButton, statusButton, logbookButton, pauseButton;
	Button<Player&> drawWeaponButton;
	Button<> stealButton, magicButton, useItemButton, campButton;
	Button<GameWorldPanel&> scrollUpButton, scrollDownButton;
	Button<Game&, bool> mapButton;
	std::array<Rect, 9> nativeCursorRegions;

	// Draws a tooltip sitting on the top left of the game interface.
	void drawTooltip(const std::string &text, Renderer &renderer);

	// Draws the compass for some given player direction in the XZ plane.
	void drawCompass(const VoxelDouble2 &direction, TextureManager &textureManager, Renderer &renderer);
public:
	GameWorldPanel(Game &game);
	virtual ~GameWorldPanel();

	bool init();

	virtual std::optional<CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void onPauseChanged(bool paused) override;
	virtual void resize(int windowWidth, int windowHeight) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
	virtual void renderSecondary(Renderer &renderer) override;
};

#endif
