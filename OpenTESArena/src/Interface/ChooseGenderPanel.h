#ifndef CHOOSE_GENDER_PANEL_H
#define CHOOSE_GENDER_PANEL_H

#include "Panel.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"

class ChooseGenderPanel : public Panel
{
private:
	TextBox titleTextBox, maleTextBox, femaleTextBox;
	Button<Game&> maleButton, femaleButton;
	ScopedUiTextureRef nightSkyTextureRef, parchmentTextureRef, cursorTextureRef;
public:
	ChooseGenderPanel(Game &game);
	~ChooseGenderPanel() override = default;

	bool init();
};

#endif
