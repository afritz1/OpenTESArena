#ifndef CHOOSE_GENDER_PANEL_H
#define CHOOSE_GENDER_PANEL_H

#include "Panel.h"
#include "../UI/Button.h"
#include "../UI/Texture.h"

class Renderer;
class TextBox;

class ChooseGenderPanel : public Panel
{
private:
	Texture parchment;
	std::unique_ptr<TextBox> genderTextBox, maleTextBox, femaleTextBox;
	Button<Game&> backToNameButton, maleButton, femaleButton;
public:
	ChooseGenderPanel(Game &game);
	virtual ~ChooseGenderPanel() = default;

	bool init();

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
