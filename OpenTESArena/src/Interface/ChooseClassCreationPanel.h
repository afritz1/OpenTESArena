#ifndef CHOOSE_CLASS_CREATION_PANEL_H
#define CHOOSE_CLASS_CREATION_PANEL_H

#include <string>

#include "Panel.h"
#include "../UI/Button.h"
#include "../UI/Texture.h"

class Renderer;
class TextBox;

class ChooseClassCreationPanel : public Panel
{
private:
	Texture parchment;
	std::unique_ptr<TextBox> titleTextBox, generateTextBox, selectTextBox;
	Button<Game&> backToMainMenuButton, generateButton, selectButton;

	void drawTooltip(const std::string &text, Renderer &renderer);
public:
	ChooseClassCreationPanel(Game &game);
	virtual ~ChooseClassCreationPanel() = default;

	bool init();

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
