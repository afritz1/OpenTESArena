#ifndef FAST_TRAVEL_SUB_PANEL_H
#define FAST_TRAVEL_SUB_PANEL_H

#include "Panel.h"

#include "components/utilities/Buffer.h"

// The glue between the province map's travel button and the game world.

class FastTravelSubPanel : public Panel
{
private:
	Buffer<ScopedUiTextureRef> animTextureRefs;
	ScopedUiTextureRef cursorTextureRef;
	double currentSeconds, totalSeconds, targetSeconds;
	int frameIndex;
public:
	FastTravelSubPanel(Game &game);
	~FastTravelSubPanel() override = default;

	bool init();

	virtual void tick(double dt) override;
};

#endif
