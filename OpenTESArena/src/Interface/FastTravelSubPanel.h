#ifndef FAST_TRAVEL_SUB_PANEL_H
#define FAST_TRAVEL_SUB_PANEL_H

#include <string>
#include <vector>

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
	// Each animation frame's time in seconds.
	static constexpr double FRAME_TIME = 1.0 / 24.0;

	ProvinceMapPanel::TravelData travelData; // To give to the game world's arrival pop-up.
	double currentSeconds, totalSeconds, targetSeconds;
	size_t frameIndex;

	// Gets the filename used for the world map image (intended for getting its palette).
	const std::string &getBackgroundFilename() const;

	// Gets the animation texture IDs for display.
	TextureBuilderIdGroup getAnimationTextureIDs() const;

	// Creates a text sub-panel for display when the player arrives at a city.
	// - @todo: holiday pop-up function.
	std::unique_ptr<Panel> makeCityArrivalPopUp() const;

	// Updates the game clock based on the travel data.
	void tickTravelTime(Random &random) const;

	// Called when the target animation time has been reached. Decides whether to go
	// straight to the game world panel or to a staff dungeon splash image panel.
	void switchToNextPanel();
public:
	FastTravelSubPanel(Game &game, const ProvinceMapPanel::TravelData &travelData);
	virtual ~FastTravelSubPanel() = default;

	static constexpr double MIN_SECONDS = 1.0;

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
