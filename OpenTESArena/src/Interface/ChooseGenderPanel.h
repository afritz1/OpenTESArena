#ifndef CHOOSE_GENDER_PANEL_H
#define CHOOSE_GENDER_PANEL_H

#include "Panel.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"
#include "../UI/Texture.h"

class Renderer;

class ChooseGenderPanel : public Panel
{
private:
	Texture parchment;
	TextBox titleTextBox, maleTextBox, femaleTextBox;
	Button<Game&> maleButton, femaleButton;
public:
	ChooseGenderPanel(Game &game);
	~ChooseGenderPanel() override = default;

	bool init();

	virtual std::optional<CursorData> getCurrentCursor() const override;
	virtual void render(Renderer &renderer) override;
};

#endif
