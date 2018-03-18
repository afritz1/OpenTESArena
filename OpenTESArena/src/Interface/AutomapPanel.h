#ifndef AUTOMAP_PANEL_H
#define AUTOMAP_PANEL_H

#include <string>

#include "Button.h"
#include "Panel.h"
#include "../Math/Vector2.h"
#include "../Rendering/Texture.h"

class Color;
class Renderer;
class TextBox;
class VoxelData;
class VoxelGrid;

class AutomapPanel : public Panel
{
private:
	std::unique_ptr<TextBox> locationTextBox;
	Button<Game&> backToGameButton;
	Texture mapTexture;
	Double2 automapOffset; // Displayed XZ coordinate offset from (0, 0).

	// Gets the display color for a pixel on the automap, given its associated floor
	// and wall voxel data definitions.
	static const Color &getPixelColor(const VoxelData &floorData, const VoxelData &wallData);

	// Listen for when the LMB is held on a compass direction.
	void handleMouse(double dt);

	void drawTooltip(const std::string &text, Renderer &renderer);
public:
	AutomapPanel(Game &game, const Double2 &playerPosition, const Double2 &playerDirection,
		const VoxelGrid &voxelGrid, const std::string &locationName);
	virtual ~AutomapPanel();

	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
