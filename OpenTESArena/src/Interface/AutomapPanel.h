#ifndef AUTOMAP_PANEL_H
#define AUTOMAP_PANEL_H

#include <string>

#include "Button.h"
#include "Panel.h"
#include "../Math/Vector2.h"
#include "../Rendering/Texture.h"

class Renderer;
class TextBox;
class VoxelGrid;

class AutomapPanel : public Panel
{
private:
	std::unique_ptr<TextBox> locationTextBox;
	std::unique_ptr<Button<Game*>> backToGameButton;
	Texture mapTexture;
	Double2 automapOffset; // Displayed XZ coordinate offset from (0, 0).

	// Listen for when the LMB is held on a compass direction.
	void handleMouse(double dt);

	void drawTooltip(const std::string &text, Renderer &renderer);
public:
	AutomapPanel(Game *game, const Double2 &playerPosition, const Double2 &playerDirection,
		const VoxelGrid &voxelGrid, const std::string &locationName);
	virtual ~AutomapPanel();

	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
