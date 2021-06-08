#ifndef WORLD_MAP_PANEL_H
#define WORLD_MAP_PANEL_H

#include "Panel.h"
#include "WorldMapUiModel.h"
#include "../Math/Vector2.h"
#include "../UI/Button.h"

class Renderer;

class WorldMapPanel : public Panel
{
private:
	Button<Game&> backToGameButton;
	Button<Game&, int> provinceButton;
	Buffer<Int2> provinceNameOffsets; // Yellow province name positions.
public:
	WorldMapPanel(Game &game);
	virtual ~WorldMapPanel() = default;

	bool init();

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
