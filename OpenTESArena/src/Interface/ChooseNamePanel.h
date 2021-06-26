#ifndef CHOOSE_NAME_PANEL_H
#define CHOOSE_NAME_PANEL_H

#include <string>

#include "Panel.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"
#include "../UI/Texture.h"

class Renderer;

class ChooseNamePanel : public Panel
{
private:
	Texture parchment;
	TextBox titleTextBox, entryTextBox;
	Button<Game&> backToClassButton;
	Button<Game&, const std::string&> acceptButton;
	std::string name;
public:
	ChooseNamePanel(Game &game);
	~ChooseNamePanel() override = default;

	bool init();

	virtual std::optional<CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
