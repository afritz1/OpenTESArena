#ifndef WORLD_MAP_PANEL_H
#define WORLD_MAP_PANEL_H

#include "Panel.h"
#include "../Math/Vector2.h"

class WorldMapPanel : public Panel
{
private:
	Buffer<Int2> provinceNameOffsets; // Yellow province name positions.
	ScopedUiTextureRef backgroundTextureRef, highlightedTextTextureRef, cursorTextureRef;
public:
	WorldMapPanel(Game &game);
	~WorldMapPanel() override;

	bool init();
};

#endif
