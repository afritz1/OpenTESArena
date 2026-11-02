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

#include "components/utilities/Buffer.h"

class Game;
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
	Double2 makeAutomapOffset(const VoxelInt2 &voxel);

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
	// Fog of war colors are ordered from most visible (0%) to most fogged (100%).
	constexpr Color ColorPlayer(247, 255, 0);
	constexpr Color ColorFloor(0, 0, 0, 0);
	constexpr Color ColorWallFogOfWar[] = { Color(130, 89, 48), Color(138, 97, 56), Color(142, 101, 60), Color(150, 109, 65), Color(158, 117, 73) };
	constexpr Color ColorRaisedFogOfWar[] = { Color(97, 85, 60), Color(105, 81, 56), Color(113, 81, 48) };
	constexpr Color ColorDoorFogOfWar[] = { Color(146, 0, 0) };
	constexpr Color ColorLevelUpFogOfWar[] = { Color(0, 105, 0) };
	constexpr Color ColorLevelDownFogOfWar[] = { Color(0, 0, 255) };
	constexpr Color ColorDryChasmFogOfWar[] = { Color(20, 40, 40), Color(69, 60, 48), Color(113, 85, 60) };
	constexpr Color ColorWetChasmFogOfWar[] = { Color(109, 138, 174), Color(125, 130, 134), Color(146, 117, 93) };
	constexpr Color ColorLavaChasmFogOfWar[] = { Color(255, 0, 0), Color(227, 36, 16), Color(195, 69, 36) };
	constexpr Color ColorNotImplemented(255, 0, 255);

	// Colors for wilderness automap pixels.
	constexpr Color ColorWildWall(109, 69, 32);
	constexpr Color ColorWildDoor(255, 0, 0);

	// Color for player-authored notes.
	constexpr Color ColorNote(56, 16, 12);
	constexpr Color ColorNoteEditing(93, 52, 24);

	constexpr int DialogueDirectionsDetailDrawOrder = 2;
	constexpr int NoteDrawOrder = DialogueDirectionsDetailDrawOrder + 1;

	TextureAsset getCursorTextureAsset();
	TextureAsset getCursorPaletteTextureAsset();

	// Gets the display color for a pixel on the automap, given its associated floor and wall voxel definitions.
	// The color depends on a couple factors, like whether the voxel is a wall, door, water, etc., and some
	// context-sensitive cases like whether a dry chasm has a wall over it.
	Color getPixelColor(const VoxelTraitsDefinition &floorDef, const VoxelTraitsDefinition &wallDef,
		const TransitionDefinition *transitionDef, double fogOfWarPercent);
	Color getWildPixelColor(const VoxelTraitsDefinition &floorDef, const VoxelTraitsDefinition &wallDef,
		const TransitionDefinition *transitionDef);

	// Generates a texture of the automap.
	Buffer2D<uint32_t> makeAutomap(const CoordInt2 &playerCoord, CardinalDirectionName playerCompassDir,
		bool isWild, bool allowFogOfWar, const WorldInt2 &levelDims, const VoxelChunkManager &voxelChunkManager);

	// Texture allocation functions (must be freed when done).
	UiTextureID allocMapTexture(const Game &game, const CoordInt2 &playerCoordXZ,
		const VoxelDouble2 &playerDirection, const VoxelChunkManager &voxelChunkManager, Renderer &renderer);
	UiTextureID allocCursorTexture(TextureManager &textureManager, Renderer &renderer);
}
