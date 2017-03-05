#ifndef WORLD_MAP_PANEL_H
#define WORLD_MAP_PANEL_H

#include "Button.h"
#include "Panel.h"

class Renderer;

enum class ProvinceName;

class WorldMapPanel : public Panel
{
private:
	std::unique_ptr<Button<>> backToGameButton, provinceButton;
	std::unique_ptr<ProvinceName> provinceName; // Null until a province is clicked.
public:
	WorldMapPanel(Game *game);
	virtual ~WorldMapPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
