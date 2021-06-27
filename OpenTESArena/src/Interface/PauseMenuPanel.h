#ifndef PAUSE_MENU_PANEL_H
#define PAUSE_MENU_PANEL_H

#include <string>

#include "Panel.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"
#include "../UI/Texture.h"

class Renderer;

class PauseMenuPanel : public Panel
{
private:
	TextBox playerNameTextBox, musicTextBox, soundTextBox, optionsTextBox;
	Button<Game&> loadButton, exitButton;
	Button<Game&> newButton, saveButton, resumeButton, optionsButton;
	Button<Game&, PauseMenuPanel&> soundUpButton, soundDownButton, musicUpButton, musicDownButton;
	Texture optionsButtonTexture;
public:
	PauseMenuPanel(Game &game);
	~PauseMenuPanel() override = default;

	bool init();

	void updateMusicText(double volume);
	void updateSoundText(double volume);

	virtual std::optional<CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
