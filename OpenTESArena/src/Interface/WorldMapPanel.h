#ifndef WORLD_MAP_PANEL_H
#define WORLD_MAP_PANEL_H

#include "Button.h"
#include "Panel.h"

class Renderer;

class WorldMapPanel : public Panel
{
private:
	std::unique_ptr<Button<Game&>> backToGameButton;
	std::unique_ptr<Button<Game&, int>> provinceButton;
public:
	WorldMapPanel(Game &game);
	virtual ~WorldMapPanel();

	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
