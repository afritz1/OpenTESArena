#ifndef AUTOMAP_UI_VIEW_H
#define AUTOMAP_UI_VIEW_H

#include <unordered_map>
#include <vector>

#include "../Assets/TextureAssetReference.h"
#include "../Game/CardinalDirectionName.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/FontName.h"
#include "../UI/TextAlignment.h"
#include "../World/Coord.h"

class ChunkManager;
class Renderer;
class Texture;
class TransitionDefinition;
class VoxelDefinition;

enum class CardinalDirectionName;

namespace AutomapUiView
{
	// Size of each automap pixel in the automap texture.
	constexpr int PixelSize = 3;

	// Number of chunks away from the player to display in the automap.
	constexpr int ChunkDistance = 1;

	// How fast the automap moves when scrolling.
	constexpr double ScrollSpeed = 100.0;

	// Click areas for compass directions.
	const Rect CompassUpRegion(264, 23, 14, 14);
	const Rect CompassDownRegion(264, 60, 14, 14);
	const Rect CompassLeftRegion(245, 41, 14, 14);
	const Rect CompassRightRegion(284, 41, 14, 14);

	// The "canvas" area for drawing automap content.
	const Rect DrawingArea(25, 40, 179, 125);

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

	const Int2 LocationTextBoxCenterPoint(120, 28);
	constexpr FontName LocationTextBoxFontName = FontName::A;
	const Color LocationTextBoxFontColor(56, 16, 12);
	constexpr TextAlignment LocationTextBoxTextAlignment = TextAlignment::Center;
	const Color LocationTextBoxShadowColor(150, 101, 52);
	const Int2 LocationTextBoxShadowOffset(2, 2);

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

	TextureAssetReference getBackgroundTextureAssetRef();
	TextureAssetReference getBackgroundPaletteTextureAssetRef();
	TextureAssetReference getCursorTextureAssetRef();
	TextureAssetReference getCursorPaletteTextureAssetRef();

	// Gets the display color for a pixel on the automap, given its associated floor and wall voxel definitions.
	// The color depends on a couple factors, like whether the voxel is a wall, door, water, etc., and some
	// context-sensitive cases like whether a dry chasm has a wall over it.
	const Color &getPixelColor(const VoxelDefinition &floorDef, const VoxelDefinition &wallDef,
		const TransitionDefinition *transitionDef);
	const Color &getWildPixelColor(const VoxelDefinition &floorDef, const VoxelDefinition &wallDef,
		const TransitionDefinition *transitionDef);

	// Generates a texture of the automap.
	Texture makeAutomap(const CoordInt2 &playerCoord, CardinalDirectionName playerCompassDir,
		bool isWild, const ChunkManager &chunkManager, Renderer &renderer);
}

#endif
