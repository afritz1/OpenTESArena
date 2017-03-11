#ifndef PROVINCE_MAP_PANEL_H
#define PROVINCE_MAP_PANEL_H

#include "Button.h"
#include "Panel.h"

class Renderer;

enum class ProvinceButtonName;

class ProvinceMapPanel : public Panel
{
private:
	std::unique_ptr<Button<>> searchButton, travelButton;
	std::unique_ptr<Button<Game*>> backToWorldMapButton;
	int provinceID;

	void drawButtonTooltip(ProvinceButtonName buttonName, Renderer &renderer);
public:
	ProvinceMapPanel(Game *game, int provinceID);
	virtual ~ProvinceMapPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
