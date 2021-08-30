#ifndef CHOOSE_CLASS_CREATION_PANEL_H
#define CHOOSE_CLASS_CREATION_PANEL_H

#include "Panel.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"

class Renderer;

class ChooseClassCreationPanel : public Panel
{
private:
	TextBox titleTextBox, generateTextBox, selectTextBox;
	Button<Game&> generateButton, selectButton;
	ScopedUiTextureRef nightSkyTextureRef, parchmentTextureRef, cursorTextureRef;
public:
	ChooseClassCreationPanel(Game &game);
	~ChooseClassCreationPanel() override = default;

	bool init();
};

#endif
