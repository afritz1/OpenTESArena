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
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	WorldMapPanel(GameState *gameState);
	virtual ~WorldMapPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(Renderer &renderer) override;
};

#endif
