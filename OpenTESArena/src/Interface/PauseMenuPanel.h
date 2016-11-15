#ifndef PAUSE_MENU_PANEL_H
#define PAUSE_MENU_PANEL_H

#include <string>

#include "Panel.h"

class Button;
class Renderer;
class TextBox;

class PauseMenuPanel : public Panel
{
private:
	std::unique_ptr<TextBox> playerNameTextBox, musicTextBox, soundTextBox;
	std::unique_ptr<Button> loadButton, exitButton, newButton, saveButton, resumeButton,
		musicUpButton, musicDownButton, soundUpButton, soundDownButton;
	
	// Need to store heads filename and portrait ID before clicking "new game" because
	// when the game data is cleared, the pause menu still needs it for one more frame.
	// That... seems like a bad design to me.
	std::string headsFilename;
	int portraitID;
	bool classCanCastMagic;

	void updateMusicText(double volume);
	void updateSoundText(double volume);
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	PauseMenuPanel(GameState *gameState);
	virtual ~PauseMenuPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(Renderer &renderer) override;
};

#endif
