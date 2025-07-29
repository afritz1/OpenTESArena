#ifndef CHOOSE_NAME_PANEL_H
#define CHOOSE_NAME_PANEL_H

#include <string>

#include "Panel.h"
#include "../UI/TextBox.h"

class Renderer;

class ChooseNamePanel : public Panel
{
private:
	TextBox titleTextBox, entryTextBox;
	std::string name;
	ScopedUiTextureRef nightSkyTextureRef, parchmentTextureRef, cursorTextureRef;
public:
	ChooseNamePanel(Game &game);
	~ChooseNamePanel() override = default;

	bool init();
};

#endif
