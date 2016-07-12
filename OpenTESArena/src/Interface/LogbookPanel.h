#ifndef LOGBOOK_PANEL_H
#define LOGBOOK_PANEL_H

#include "Panel.h"

class Button;
class Renderer;
class TextBox;

class LogbookPanel : public Panel
{
private:
	std::unique_ptr<TextBox> titleTextBox;
	std::unique_ptr<Button> backButton;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	LogbookPanel(GameState *gameState);
	virtual ~LogbookPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(Renderer &renderer) override;
};

#endif
