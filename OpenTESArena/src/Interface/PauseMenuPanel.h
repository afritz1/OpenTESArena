#ifndef PAUSE_MENU_PANEL_H
#define PAUSE_MENU_PANEL_H

#include <string>

#include "Button.h"
#include "Panel.h"

class AudioManager;
class Options;
class Renderer;
class TextBox;

class PauseMenuPanel : public Panel
{
private:
	std::unique_ptr<TextBox> playerNameTextBox, musicTextBox, soundTextBox, optionsTextBox;
	Button<Game&> loadButton;
	Button<> exitButton;
	Button<Game&> newButton, saveButton, resumeButton, optionsButton;
	Button<Options&, AudioManager&, PauseMenuPanel&> musicUpButton,
		musicDownButton, soundUpButton, soundDownButton;

	void updateMusicText(double volume);
	void updateSoundText(double volume);
public:
	PauseMenuPanel(Game &game);
	virtual ~PauseMenuPanel() = default;

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
