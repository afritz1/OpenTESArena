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

class GameWorldPanel : public Panel
{
private:
	TextBox playerNameTextBox, triggerText, actionText, effectText;
	Button<Game&> characterSheetButton, statusButton, logbookButton;
	Button<Player&> drawWeaponButton;
	Button<> stealButton, magicButton, useItemButton, campButton;
	Button<GameWorldPanel&> scrollUpButton, scrollDownButton;
	Button<Game&, bool> mapButton;
	std::array<Rect, 9> nativeCursorRegions;
	Buffer<ScopedUiTextureRef> arrowCursorTextureRefs, weaponAnimTextureRefs, tooltipTextureRefs;
	ScopedUiTextureRef gameWorldInterfaceTextureRef, statusGradientTextureRef, playerPortraitTextureRef,
		noMagicTextureRef, compassFrameTextureRef, compassSliderTextureRef, defaultCursorTextureRef;

	void initUiDrawCalls();

	// Draws a tooltip sitting on the top left of the game interface.
	void drawTooltip(const std::string &text, Renderer &renderer);

	// Called by game loop for rendering the 3D scene.
	static bool gameWorldRenderCallback(Game &game);
public:
	GameWorldPanel(Game &game);
	~GameWorldPanel() override;

	bool init();

	virtual void onPauseChanged(bool paused) override;
	virtual void resize(int windowWidth, int windowHeight) override;
	virtual void tick(double dt) override;
};

#endif
