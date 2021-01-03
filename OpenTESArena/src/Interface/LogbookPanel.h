#ifndef LOGBOOK_PANEL_H
#define LOGBOOK_PANEL_H

#include "Button.h"
#include "Panel.h"
#include "../Media/TextureUtils.h"

class Renderer;
class TextBox;

class LogbookPanel : public Panel
{
private:
	std::unique_ptr<TextBox> titleTextBox;
	Button<Game&> backButton;
public:
	LogbookPanel(Game &game);
	virtual ~LogbookPanel() = default;

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
