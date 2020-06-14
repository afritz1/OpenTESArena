#ifndef AUTOMAP_PANEL_H
#define AUTOMAP_PANEL_H

#include <string>

#include "Button.h"
#include "Panel.h"
#include "../Math/Vector2.h"
#include "../Rendering/Texture.h"
#include "../World/VoxelUtils.h"

class Color;
class Renderer;
class Surface;
class TextBox;
class VoxelDefinition;
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
	static const Color &getPixelColor(const VoxelDefinition &floorDef,
		const VoxelDefinition &wallDef);
	static const Color &getWildPixelColor(const VoxelDefinition &floorDef,
		const VoxelDefinition &wallDef);

	// Generates a surface of the automap to be converted to a texture for rendering.
	static Surface makeAutomap(const Int2 &playerVoxel, CardinalDirectionName playerDir,
		bool isWild, const VoxelGrid &voxelGrid);

	// Calculates screen offset of automap for rendering.
	static Double2 makeAutomapOffset(const NewInt2 &playerVoxel, bool isWild,
		SNInt gridWidth, WEInt gridDepth);

	// Helper function for obtaining relative wild origin in new coordinate system.
	static NewInt2 makeRelativeWildOrigin(const NewInt2 &voxel, SNInt gridWidth, WEInt gridDepth);

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
