#ifndef PROVINCE_MAP_PANEL_H
#define PROVINCE_MAP_PANEL_H

#include "Panel.h"

class Button;
class Province;
class Renderer;

enum class ProvinceButtonName;

class ProvinceMapPanel : public Panel
{
private:
	std::unique_ptr<Button> searchButton, travelButton, backToWorldMapButton;
	std::unique_ptr<Province> province;

	void drawButtonTooltip(ProvinceButtonName buttonName, Renderer &renderer);
public:
	ProvinceMapPanel(GameState *gameState, const Province &province);
	virtual ~ProvinceMapPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
