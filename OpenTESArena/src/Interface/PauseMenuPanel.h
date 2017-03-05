#ifndef PAUSE_MENU_PANEL_H
#define PAUSE_MENU_PANEL_H

#include <string>

#include "Button.h"
#include "Panel.h"

class Renderer;
class TextBox;

class PauseMenuPanel : public Panel
{
private:
	std::unique_ptr<TextBox> playerNameTextBox, musicTextBox, soundTextBox,
		optionsTextBox, optionsShadowTextBox;
	std::unique_ptr<Button<>> loadButton, exitButton, newButton, saveButton, resumeButton,
		optionsButton, musicUpButton, musicDownButton, soundUpButton, soundDownButton;

	void updateMusicText(double volume);
	void updateSoundText(double volume);
public:
	PauseMenuPanel(Game *game);
	virtual ~PauseMenuPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
