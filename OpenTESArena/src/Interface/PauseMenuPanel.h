#ifndef PAUSE_MENU_PANEL_H
#define PAUSE_MENU_PANEL_H

#include <string>

#include "Panel.h"
#include "../UI/Button.h"
#include "../UI/Texture.h"

class Renderer;
class TextBox;

class PauseMenuPanel : public Panel
{
private:
	std::unique_ptr<TextBox> playerNameTextBox, musicTextBox, soundTextBox, optionsTextBox;
	Button<Game&> loadButton, exitButton;
	Button<Game&> newButton, saveButton, resumeButton, optionsButton;
	Button<Game&, PauseMenuPanel&> soundUpButton, soundDownButton, musicUpButton, musicDownButton;
	Texture optionsButtonTexture;
public:
	PauseMenuPanel(Game &game);
	virtual ~PauseMenuPanel() = default;

	bool init();

	void updateMusicText(double volume);
	void updateSoundText(double volume);

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
