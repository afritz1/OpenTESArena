#ifndef LOAD_SAVE_PANEL_H
#define LOAD_SAVE_PANEL_H

#include <vector>

#include "Panel.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"

class Renderer;

class LoadSavePanel : public Panel
{
public:
	enum class Type { Load, Save };
private:
	std::vector<TextBox> saveTextBoxes;
	ScopedUiTextureRef backgroundTextureRef, cursorTextureRef;
	LoadSavePanel::Type type;
public:
	LoadSavePanel(Game &game);
	~LoadSavePanel() override = default;

	bool init(LoadSavePanel::Type type);
};

#endif
