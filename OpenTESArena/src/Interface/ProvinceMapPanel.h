#ifndef PROVINCE_MAP_PANEL_H
#define PROVINCE_MAP_PANEL_H

#include "Button.h"
#include "Panel.h"
#include "../World/Province.h"

class Renderer;

enum class ProvinceButtonName;

class ProvinceMapPanel : public Panel
{
private:
	std::unique_ptr<Button<>> searchButton, travelButton, backToWorldMapButton;
	Province province;

	void drawButtonTooltip(ProvinceButtonName buttonName, Renderer &renderer);
public:
	ProvinceMapPanel(Game *game, const Province &province);
	virtual ~ProvinceMapPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
