#ifndef CHOOSE_RACE_PANEL_H
#define CHOOSE_RACE_PANEL_H

#include <optional>
#include <string>

#include "Panel.h"
#include "../Math/Vector2.h"
#include "../UI/Button.h"
#include "../UI/Texture.h"

class Renderer;

class ChooseRacePanel : public Panel
{
private:
	Button<Game&> backToGenderButton;
	Button<Game&, int> selectProvinceButton;

	void drawProvinceTooltip(int provinceID, Renderer &renderer);	
public:
	ChooseRacePanel(Game &game);
	~ChooseRacePanel() override = default;

	bool init();

	// Gets the initial parchment pop-up (public for the UI controller function).
	static std::unique_ptr<Panel> getInitialSubPanel(Game &game);

	virtual std::optional<CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
	virtual void renderSecondary(Renderer &renderer) override;
};

#endif
