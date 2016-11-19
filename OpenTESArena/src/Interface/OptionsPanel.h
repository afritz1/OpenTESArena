#ifndef OPTIONS_PANEL_H
#define OPTIONS_PANEL_H

#include "Panel.h"

// I'll find a use for this class eventually. Arena doesn't have an options menu,
// so I need to make one up somewhere.

class Button;
class Renderer;
class TextBox;

class OptionsPanel : public Panel
{
private:
	std::unique_ptr<TextBox> titleTextBox;
	std::unique_ptr<Button> backToPauseButton;
public:
	OptionsPanel(Game *game);
	virtual ~OptionsPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
