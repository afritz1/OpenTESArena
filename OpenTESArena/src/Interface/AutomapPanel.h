#ifndef AUTOMAP_PANEL_H
#define AUTOMAP_PANEL_H

#include <string>

#include "Button.h"
#include "Panel.h"
#include "Texture.h"
#include "../Math/Vector2.h"
#include "../Media/TextureUtils.h"
#include "../World/LevelData.h"
#include "../World/VoxelUtils.h"

// @todo: be able to click somewhere inside the drawable area of the automap and get a 2D voxel
// coordinate in world space for attaching a note to. Store the note in GameData or something.

// @todo: will need to redesign the map rendering code once the voxel grid is gone since it does an
// offset from the wilderness origin if in the wilderness.

// @todo: maybe split the automap into one texture per chunk. This would make it easier to add
// new functionality like zooming out, since it would only add/remove textures to the display
// list instead of resizing one and having to change its coordinates.
// - the only worry with this is SDL rendering imprecision, resulting in 1 pixel gaps between chunks.
// - Texture makeChunkTexture(??Int chunkX, ??Int chunkY);
// - Int2 getChunkPixelPosition(??Int chunkX, ??Int chunkY); // position on-screen in original render coords
// - just get the surrounding 3x3 chunks. Does it really matter that it's 2x2 like the original game?

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
	TextureBuilderID backgroundTextureBuilderID;
	PaletteID backgroundPaletteID;

	// XZ coordinate offset in automap space, stored as a real so scroll position can be sub-pixel.
	Double2 automapOffset;

	// Gets the display color for a pixel on the automap, given its associated floor and wall
	// voxel data definitions. The color depends on a couple factors, like whether the voxel is
	// a wall, door, water, etc., and some context-sensitive cases like whether a dry chasm
	// has a wall over it.
	static const Color &getPixelColor(const VoxelDefinition &floorDef, const VoxelDefinition &wallDef,
		const NewInt2 &voxel, const LevelData::Transitions &transitions);
	static const Color &getWildPixelColor(const VoxelDefinition &floorDef, const VoxelDefinition &wallDef,
		const NewInt2 &voxel, const LevelData::Transitions &transitions);

	// Generates a surface of the automap to be converted to a texture for rendering.
	static Surface makeAutomap(const NewInt2 &playerVoxel, CardinalDirectionName playerDir,
		bool isWild, const VoxelGrid &voxelGrid, const LevelData::Transitions &transitions);

	// Calculates screen offset of automap for rendering.
	static Double2 makeAutomapOffset(const NewInt2 &playerVoxel, bool isWild,
		SNInt gridWidth, WEInt gridDepth);

	// Helper function for obtaining relative wild origin in new coordinate system.
	static NewInt2 makeRelativeWildOrigin(const NewInt2 &voxel, SNInt gridWidth, WEInt gridDepth);

	// Listen for when the LMB is held on a compass direction.
	void handleMouse(double dt);

	void drawTooltip(const std::string &text, Renderer &renderer);
public:
	AutomapPanel(Game &game, const CoordDouble3 &playerPosition, const NewDouble2 &playerDirection,
		const VoxelGrid &voxelGrid, const LevelData::Transitions &transitions, const std::string &locationName);
	virtual ~AutomapPanel() = default;

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
