#pragma once

#include <string>
#include <vector>

#include "../Assets/TextureAsset.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../UI/TextRenderUtils.h"
#include "../Utilities/Color.h"
#include "../World/Coord.h"

#include "components/utilities/Buffer2D.h"

class Game;
class GameState;
class Renderer;
class VoxelChunkManager;

enum class CardinalDirectionName;
enum class MouseButtonType;

struct InputActionCallbackValues;
struct TransitionDefinition;
struct VoxelTraitsDefinition;

namespace AutomapUiModel
{
	// Calculates automap screen offset in pixels for rendering.
	Double2 makeAutomapOffset(const VoxelInt2 &playerVoxel);

	// Helper function for obtaining relative wild origin in new coordinate system.
	WorldInt2 makeRelativeWildOrigin(const WorldInt2 &voxel, SNInt gridWidth, WEInt gridDepth);
}

namespace AutomapUiView
{
	// Size of each automap pixel in the automap texture.
	constexpr int PixelSize = 3;

	// Number of chunks away from the player to display in the automap.
	constexpr int ChunkDistance = 1;

	// How fast the automap moves when scrolling.
	constexpr double ScrollSpeed = 100.0;

	// Click areas for compass directions.
	constexpr Rect CompassUpRegion(264, 23, 14, 14);
	constexpr Rect CompassDownRegion(264, 60, 14, 14);
	constexpr Rect CompassLeftRegion(245, 41, 14, 14);
	constexpr Rect CompassRightRegion(284, 41, 14, 14);

	// The "canvas" area for drawing automap content.
	constexpr Rect DrawingArea(25, 40, 179, 125);

	// Colors for automap pixels. Ground pixels (y == 0) are transparent.
	const Color ColorPlayer(247, 255, 0);
	const Color ColorFloor(0, 0, 0, 0);
	const Color ColorWall(130, 89, 48);
	const Color ColorRaised(97, 85, 60);
	const Color ColorDoor(146, 0, 0);
	const Color ColorLevelUp(0, 105, 0);
	const Color ColorLevelDown(0, 0, 255);
	const Color ColorDryChasm(20, 40, 40);
	const Color ColorWetChasm(109, 138, 174);
	const Color ColorLavaChasm(255, 0, 0);
	const Color ColorNotImplemented(255, 0, 255);

	// Colors for wilderness automap pixels.
	const Color ColorWildWall(109, 69, 32);
	const Color ColorWildDoor(255, 0, 0);

	TextureAsset getCursorTextureAsset();
	TextureAsset getCursorPaletteTextureAsset();

	// Gets the display color for a pixel on the automap, given its associated floor and wall voxel definitions.
	// The color depends on a couple factors, like whether the voxel is a wall, door, water, etc., and some
	// context-sensitive cases like whether a dry chasm has a wall over it.
	const Color &getPixelColor(const VoxelTraitsDefinition &floorDef, const VoxelTraitsDefinition &wallDef,
		const TransitionDefinition *transitionDef);
	const Color &getWildPixelColor(const VoxelTraitsDefinition &floorDef, const VoxelTraitsDefinition &wallDef,
		const TransitionDefinition *transitionDef);

	// Generates a texture of the automap.
	Buffer2D<uint32_t> makeAutomap(const CoordInt2 &playerCoord, CardinalDirectionName playerCompassDir,
		bool isWild, const WorldInt2 &levelDims, const VoxelChunkManager &voxelChunkManager);

	// Texture allocation functions (must be freed when done).
	UiTextureID allocMapTexture(const GameState &gameState, const CoordInt2 &playerCoordXZ,
		const VoxelDouble2 &playerDirection, const VoxelChunkManager &voxelChunkManager, Renderer &renderer);
	UiTextureID allocCursorTexture(TextureManager &textureManager, Renderer &renderer);
}

namespace AutomapUiController
{
	void onBackToGameButtonSelected(Game &game);
	void onBackToGameInputAction(const InputActionCallbackValues &values);

	void onMouseButtonHeld(Game &game, MouseButtonType buttonType, const Int2 &position, double dt, Double2 *automapOffset);
}
