#ifndef MAIN_QUEST_SPLASH_PANEL_H
#define MAIN_QUEST_SPLASH_PANEL_H

#include <string>

#include "Button.h"
#include "TextBox.h"
#include "Panel.h"

class MainQuestSplashPanel : public Panel
{
private:
	std::unique_ptr<TextBox> textBox;
	Button<Game&> exitButton;
	std::string splashFilename;
public:
	MainQuestSplashPanel(Game &game, int provinceID);
	virtual ~MainQuestSplashPanel() = default;

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
