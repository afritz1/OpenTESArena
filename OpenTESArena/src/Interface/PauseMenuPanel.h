#ifndef PAUSE_MENU_PANEL_H
#define PAUSE_MENU_PANEL_H

#include "Panel.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"

class Renderer;

class PauseMenuPanel : public Panel
{
private:
	TextBox playerNameTextBox, musicTextBox, soundTextBox, optionsTextBox;
	Button<Game&> loadButton, exitButton, newButton, saveButton, resumeButton, optionsButton;
	Button<Game&, PauseMenuPanel&> soundUpButton, soundDownButton, musicUpButton, musicDownButton;
	ScopedUiTextureRef backgroundTextureRef, gameWorldInterfaceTextureRef, healthBarTextureRef, staminaBarTextureRef, spellPointsBarTextureRef,
		statusGradientTextureRef, playerPortraitTextureRef, noMagicTextureRef, optionsButtonTextureRef, cursorTextureRef;
public:
	PauseMenuPanel(Game &game);
	~PauseMenuPanel() override = default;

	bool init();

	void updateMusicText(double volume);
	void updateSoundText(double volume);
};

#endif
