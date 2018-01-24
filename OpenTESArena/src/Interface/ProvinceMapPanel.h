#ifndef PROVINCE_MAP_PANEL_H
#define PROVINCE_MAP_PANEL_H

#include "Button.h"
#include "Panel.h"

class Renderer;

enum class ProvinceButtonName;

class ProvinceMapPanel : public Panel
{
private:
	Button<> searchButton, travelButton;
	Button<Game&> backToWorldMapButton;
	int provinceID;

	void drawButtonTooltip(ProvinceButtonName buttonName, Renderer &renderer);

	// Draws the name of a location in the province. Intended for the location
	// closest to the mouse cursor.
	void drawLocationName(const std::string &name, const Int2 &center,
		Renderer &renderer);
public:
	ProvinceMapPanel(Game &game, int provinceID);
	virtual ~ProvinceMapPanel();

	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
	virtual void renderSecondary(Renderer &renderer) override;
};

#endif
