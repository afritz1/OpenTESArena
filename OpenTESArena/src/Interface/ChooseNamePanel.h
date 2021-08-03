#ifndef CHOOSE_NAME_PANEL_H
#define CHOOSE_NAME_PANEL_H

#include <string>

#include "Panel.h"
#include "../UI/TextBox.h"
#include "../UI/Texture.h"

class Renderer;

class ChooseNamePanel : public Panel
{
private:
	Texture parchment;
	TextBox titleTextBox, entryTextBox;
	std::string name;
public:
	ChooseNamePanel(Game &game);
	~ChooseNamePanel() override = default;

	bool init();

	virtual std::optional<CursorData> getCurrentCursor() const override;
	virtual void render(Renderer &renderer) override;
};

#endif
