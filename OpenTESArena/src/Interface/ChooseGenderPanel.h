#ifndef CHOOSE_GENDER_PANEL_H
#define CHOOSE_GENDER_PANEL_H

#include <string>

#include "Button.h"
#include "Panel.h"
#include "Texture.h"

class Renderer;
class TextBox;

class ChooseGenderPanel : public Panel
{
private:
	Texture parchment;
	std::unique_ptr<TextBox> genderTextBox, maleTextBox, femaleTextBox;
	Button<Game&> backToNameButton;
	Button<Game&> maleButton, femaleButton;
public:
	ChooseGenderPanel(Game &game);
	virtual ~ChooseGenderPanel() = default;

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
