#ifndef WORLD_MAP_PANEL_H
#define WORLD_MAP_PANEL_H

#include "Panel.h"

class Button;
class Renderer;

enum class ProvinceName;

class WorldMapPanel : public Panel
{
private:
	std::unique_ptr<Button> backToGameButton, provinceButton;
	std::unique_ptr<ProvinceName> provinceName;
public:
	WorldMapPanel(GameState *gameState);
	virtual ~WorldMapPanel();

	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
