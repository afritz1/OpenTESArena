#ifndef LOGBOOK_PANEL_H
#define LOGBOOK_PANEL_H

#include "Button.h"
#include "Panel.h"

class Renderer;
class TextBox;

class LogbookPanel : public Panel
{
private:
	std::unique_ptr<TextBox> titleTextBox;
	std::unique_ptr<Button<Game*>> backButton;
public:
	LogbookPanel(Game *game);
	virtual ~LogbookPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
