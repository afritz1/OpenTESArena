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
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	ProvinceMapPanel(GameState *gameState, const Province &province);
	virtual ~ProvinceMapPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(Renderer &renderer) override;
};

#endif
