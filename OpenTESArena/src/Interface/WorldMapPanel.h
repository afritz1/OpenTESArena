#ifndef WORLD_MAP_PANEL_H
#define WORLD_MAP_PANEL_H

#include "Button.h"
#include "Panel.h"

class Renderer;

class WorldMapPanel : public Panel
{
private:
	std::unique_ptr<Button<>> backToGameButton;
	std::unique_ptr<Button<int>> provinceButton;
public:
	WorldMapPanel(Game *game);
	virtual ~WorldMapPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
