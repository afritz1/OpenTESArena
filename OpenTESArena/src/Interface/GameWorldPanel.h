#ifndef GAME_WORLD_PANEL_H
#define GAME_WORLD_PANEL_H

#include "Panel.h"
#include "../Assets/TextureUtils.h"
#include "../Math/Rect.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"
#include "../Voxels/VoxelUtils.h"

struct Player;

// When the GameWorldPanel is active, the game world is ticking.
// There are two desired kinds of interfaces:
// - The original: compass, portrait, stat bars, and buttons with original mouse.
// - A modern version: only compass and stat bars with free-look mouse.
class GameWorldPanel : public Panel
{
private:
	TextBox playerNameTextBox, triggerText, actionText, effectText;
	Button<Game&> characterSheetButton, statusButton, logbookButton;
	Button<Player&> drawWeaponButton;
	Button<> stealButton, magicButton, useItemButton, campButton;
	Button<GameWorldPanel&> scrollUpButton, scrollDownButton;
	Button<Game&, bool> mapButton;
	Buffer<ScopedUiTextureRef> arrowCursorTextureRefs, weaponAnimTextureRefs, keyTextureRefs, tooltipTextureRefs;
	ScopedUiTextureRef gameWorldInterfaceTextureRef, statusGradientTextureRef, playerPortraitTextureRef,
		noMagicTextureRef, compassFrameTextureRef, compassSliderTextureRef, defaultCursorTextureRef, modernModeReticleTextureRef;

	void initUiDrawCalls();
public:
	GameWorldPanel(Game &game);
	~GameWorldPanel() override;

	bool init();

	// @temp workaround until there are listener callbacks or something for updating text boxes from game logic
	TextBox &getTriggerTextBox();

	// Called by game loop for rendering the 3D scene.
	static bool renderScene(Game &game);

	virtual void onPauseChanged(bool paused) override;
};

#endif
