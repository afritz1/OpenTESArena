#ifndef MAIN_QUEST_SPLASH_PANEL_H
#define MAIN_QUEST_SPLASH_PANEL_H

#include <string>

#include "Panel.h"
#include "../Assets/TextureAssetReference.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"

class MainQuestSplashPanel : public Panel
{
private:
	std::unique_ptr<TextBox> textBox;
	Button<Game&> exitButton;
	TextureAssetReference splashTextureAssetRef;
public:
	MainQuestSplashPanel(Game &game, int provinceID);
	virtual ~MainQuestSplashPanel() = default;

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
