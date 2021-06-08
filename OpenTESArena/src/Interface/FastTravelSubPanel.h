#ifndef FAST_TRAVEL_SUB_PANEL_H
#define FAST_TRAVEL_SUB_PANEL_H

#include "Panel.h"
#include "ProvinceMapPanel.h"
#include "../Media/TextureUtils.h"

// This sub-panel is the glue between the province map's travel button and the game world.

class Random;
class Renderer;
class Texture;

class FastTravelSubPanel : public Panel
{
private:
	double currentSeconds, totalSeconds, targetSeconds;
	size_t frameIndex;
public:
	FastTravelSubPanel(Game &game);
	virtual ~FastTravelSubPanel() = default;

	bool init();

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
