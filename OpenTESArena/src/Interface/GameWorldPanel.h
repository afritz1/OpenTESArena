#ifndef GAME_WORLD_PANEL_H
#define GAME_WORLD_PANEL_H

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
	Buffer<ScopedUiTextureRef> arrowCursorTextureRefs, weaponAnimTextureRefs, tooltipTextureRefs;
	ScopedUiTextureRef gameWorldInterfaceTextureRef, statusGradientTextureRef, playerPortraitTextureRef,
		noMagicTextureRef, compassFrameTextureRef, compassSliderTextureRef, defaultCursorTextureRef;

	void initUiDrawCalls();

	// Called by game loop for rendering the 3D scene.
	static bool gameWorldRenderCallback(Game &game);
public:
	GameWorldPanel(Game &game);
	~GameWorldPanel() override;

	bool init();

	// @temp workaround until there are listener callbacks or something for updating text boxes from game logic
	TextBox &getTriggerTextBox();

	virtual void onPauseChanged(bool paused) override;
};

#endif
