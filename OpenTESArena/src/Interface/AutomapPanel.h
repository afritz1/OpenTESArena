#ifndef AUTOMAP_PANEL_H
#define AUTOMAP_PANEL_H

#include <string>

#include "Button.h"
#include "Panel.h"
#include "../Math/Vector2.h"
#include "../Rendering/Texture.h"

class Color;
class Renderer;
class Surface;
class TextBox;
class VoxelData;
class VoxelGrid;

enum class CardinalDirectionName;

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

	// Generates a surface of the automap to be converted to a texture for rendering.
	static Surface makeAutomap(const Int2 &playerVoxel, CardinalDirectionName playerDir,
		bool isWild, const VoxelGrid &voxelGrid);

	// Calculates screen offset of automap for rendering.
	static Double2 makeAutomapOffset(const Int2 &playerVoxel, bool isWild,
		int gridWidth, int gridDepth);

	// Helper function for obtaining relative wild origin in new coordinate system.
	static Int2 makeRelativeWildOrigin(const Int2 &voxel, int gridWidth, int gridDepth);

	// Listen for when the LMB is held on a compass direction.
	void handleMouse(double dt);

	void drawTooltip(const std::string &text, Renderer &renderer);
public:
	AutomapPanel(Game &game, const Double2 &playerPosition, const Double2 &playerDirection,
		const VoxelGrid &voxelGrid, const std::string &locationName);
	virtual ~AutomapPanel() = default;

	virtual Panel::CursorData getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
