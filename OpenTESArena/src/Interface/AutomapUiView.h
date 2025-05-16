#ifndef AUTOMAP_UI_VIEW_H
#define AUTOMAP_UI_VIEW_H

#include <unordered_map>
#include <vector>

#include "../Assets/TextureAsset.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../UI/TextRenderUtils.h"
#include "../Utilities/Color.h"
#include "../World/CardinalDirectionName.h"
#include "../World/Coord.h"

#include "components/utilities/Buffer2D.h"

class GameState;
class Renderer;
class Texture;
class VoxelChunkManager;

enum class CardinalDirectionName;

struct TransitionDefinition;
struct VoxelTraitsDefinition;

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

	const Int2 LocationTextBoxCenterPoint(121, 29);
	const std::string LocationTextBoxFontName = ArenaFontName::A;
	const Color LocationTextBoxFontColor(56, 16, 12);
	constexpr TextAlignment LocationTextBoxTextAlignment = TextAlignment::TopCenter;
	const Color LocationTextBoxShadowColor(150, 101, 52);
	constexpr int LocationTextBoxShadowOffsetX = 2;
	constexpr int LocationTextBoxShadowOffsetY = 2;

	TextBox::InitInfo getLocationTextBoxInitInfo(const std::string_view text, const FontLibrary &fontLibrary);

	const Int2 BackToGameButtonCenterPoint(ArenaRenderUtils::SCREEN_WIDTH - 57, ArenaRenderUtils::SCREEN_HEIGHT - 29);
	constexpr int BackToGameButtonWidth = 38;
	constexpr int BackToGameButtonHeight = 13;

	// Sets of sub-pixel coordinates for drawing each of the player's arrow directions. 
	// These are offsets from the top-left corner of the map pixel that the player is in.
	const std::unordered_map<CardinalDirectionName, std::vector<Int2>> PlayerArrowPatterns =
	{
		{ CardinalDirectionName::North, { Int2(1, 0), Int2(0, 1), Int2(2, 1) } },
		{ CardinalDirectionName::NorthEast, { Int2(0, 0), Int2(1, 0), Int2(2, 0), Int2(2, 1), Int2(2, 2) } },
		{ CardinalDirectionName::East, { Int2(1, 0), Int2(2, 1), Int2(1, 2) } },
		{ CardinalDirectionName::SouthEast, { Int2(2, 0), Int2(2, 1), Int2(0, 2), Int2(1, 2), Int2(2, 2) } },
		{ CardinalDirectionName::South, { Int2(0, 1), Int2(2, 1), Int2(1, 2) } },
		{ CardinalDirectionName::SouthWest, { Int2(0, 0), Int2(0, 1), Int2(0, 2), Int2(1, 2), Int2(2, 2) } },
		{ CardinalDirectionName::West, { Int2(1, 0), Int2(0, 1), Int2(1, 2) } },
		{ CardinalDirectionName::NorthWest, { Int2(0, 0), Int2(1, 0), Int2(2, 0), Int2(0, 1), Int2(0, 2) } }
	};

	TextureAsset getBackgroundTextureAsset();
	TextureAsset getBackgroundPaletteTextureAsset();
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
	UiTextureID allocBgTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocCursorTexture(TextureManager &textureManager, Renderer &renderer);
}

#endif
