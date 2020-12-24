#include <cmath>
#include <unordered_map>
#include <vector>

#include "SDL.h"

#include "AutomapPanel.h"
#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "RichTextString.h"
#include "Surface.h"
#include "TextBox.h"
#include "../Game/CardinalDirection.h"
#include "../Game/CardinalDirectionName.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Interface/TextAlignment.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/FontLibrary.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../World/ArenaWildUtils.h"
#include "../World/MapType.h"
#include "../World/VoxelDefinition.h"
#include "../World/VoxelFacing2D.h"
#include "../World/VoxelGrid.h"
#include "../World/VoxelType.h"

#include "components/debug/Debug.h"

namespace
{
	// How fast the automap moves when scrolling.
	constexpr double AutomapScrollSpeed = 100.0;

	// Size of each automap pixel in the automap texture.
	constexpr int AutomapPixelSize = 3;

	// Click areas for compass directions.
	const Rect UpRegion(264, 23, 14, 14);
	const Rect DownRegion(264, 60, 14, 14);
	const Rect LeftRegion(245, 41, 14, 14);
	const Rect RightRegion(284, 41, 14, 14);

	// The "canvas" area for drawing automap content.
	const Rect DrawingArea(25, 40, 179, 125);

	// Colors for automap pixels. Ground pixels (y == 0) are transparent.
	const Color AutomapPlayer(247, 255, 0);
	const Color AutomapFloor(0, 0, 0, 0);
	const Color AutomapWall(130, 89, 48);
	const Color AutomapRaised(97, 85, 60);
	const Color AutomapDoor(146, 0, 0);
	const Color AutomapLevelUp(0, 105, 0);
	const Color AutomapLevelDown(0, 0, 255);
	const Color AutomapDryChasm(20, 40, 40);
	const Color AutomapWetChasm(109, 138, 174);
	const Color AutomapLavaChasm(255, 0, 0);
	const Color AutomapNotImplemented(255, 0, 255);

	// Colors for wilderness automap pixels.
	const Color AutomapWildWall(109, 69, 32);
	const Color AutomapWildDoor(255, 0, 0);

	// Sets of sub-pixel coordinates for drawing each of the player's arrow directions. 
	// These are offsets from the top-left corner of the map pixel that the player is in.
	const std::unordered_map<CardinalDirectionName, std::vector<Int2>> AutomapPlayerArrowPatterns =
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
}

AutomapPanel::AutomapPanel(Game &game, const Double2 &playerPosition,
	const Double2 &playerDirection, const VoxelGrid &voxelGrid, const std::string &locationName)
	: Panel(game)
{
	this->locationTextBox = [&game, &locationName]()
	{
		const Int2 center(120, 28);

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			locationName,
			FontName::A,
			Color(56, 16, 12),
			TextAlignment::Center,
			fontLibrary);

		const TextBox::ShadowData shadowData(Color(150, 101, 52), Int2(2, 2));
		return std::make_unique<TextBox>(center, richText, &shadowData,
			fontLibrary, game.getRenderer());
	}();

	this->backToGameButton = []()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH - 57, Renderer::ORIGINAL_HEIGHT - 29);
		int width = 38;
		int height = 13;
		auto function = [](Game &game)
		{
			game.setPanel<GameWorldPanel>(game);
		};
		return Button<Game&>(center, width, height, function);
	}();

	// Player's XZ voxel coordinate.
	const NewInt2 playerVoxel(
		static_cast<int>(std::floor(playerPosition.x)),
		static_cast<int>(std::floor(playerPosition.y)));

	const bool isWild = [&game]()
	{
		const auto &worldData = game.getGameData().getActiveWorld();
		return worldData.getMapType() == MapType::Wilderness;
	}();

	this->mapTexture = [&game, &playerDirection, &voxelGrid, &playerVoxel, isWild]()
	{
		const CardinalDirectionName playerDir = CardinalDirection::getDirectionName(playerDirection);

		auto &renderer = game.getRenderer();
		Surface surface = AutomapPanel::makeAutomap(playerVoxel, playerDir, isWild, voxelGrid);
		Texture texture = renderer.createTextureFromSurface(surface);

		return texture;
	}();

	auto &textureManager = game.getTextureManager();
	const std::string &backgroundTextureName = TextureFile::fromName(TextureName::Automap);
	const std::string &backgroundPaletteName = backgroundTextureName;
	PaletteID backgroundPaletteID;
	if (!textureManager.tryGetPaletteID(backgroundPaletteName.c_str(), &backgroundPaletteID))
	{
		DebugCrash("Couldn't get palette ID for \"" + backgroundPaletteName + "\".");
	}

	auto &renderer = game.getRenderer();
	if (!textureManager.tryGetTextureID(backgroundTextureName.c_str(), backgroundPaletteID,
		renderer, &this->backgroundTextureID))
	{
		DebugCrash("Couldn't get texture ID for \"" + backgroundTextureName + "\".");
	}

	this->automapOffset = AutomapPanel::makeAutomapOffset(
		playerVoxel, isWild, voxelGrid.getWidth(), voxelGrid.getDepth());
}

const Color &AutomapPanel::getPixelColor(const VoxelDefinition &floorDef, const VoxelDefinition &wallDef)
{
	const VoxelType floorType = floorDef.type;
	const VoxelType wallType = wallDef.type;

	if (floorType == VoxelType::Chasm)
	{
		const VoxelDefinition::ChasmData::Type chasmType = floorDef.chasm.type;

		if (chasmType == VoxelDefinition::ChasmData::Type::Dry)
		{
			// Dry chasms are a different color if a wall is over them.
			return (wallType == VoxelType::Wall) ? AutomapRaised : AutomapDryChasm;
		}
		else if (chasmType == VoxelDefinition::ChasmData::Type::Lava)
		{
			// Lava chasms ignore all but raised platforms.
			return (wallType == VoxelType::Raised) ? AutomapRaised : AutomapLavaChasm;
		}
		else if (chasmType == VoxelDefinition::ChasmData::Type::Wet)
		{
			// Water chasms ignore all but raised platforms.
			return (wallType == VoxelType::Raised) ? AutomapRaised : AutomapWetChasm;
		}
		else
		{
			DebugLogWarning("Unrecognized chasm type \"" +
				std::to_string(static_cast<int>(chasmType)) + "\".");
			return AutomapNotImplemented;
		}
	}
	else if (floorType == VoxelType::Floor)
	{
		// If nothing is over the floor, return transparent. Otherwise, choose from
		// a number of cases.
		if (wallType == VoxelType::None)
		{
			return AutomapFloor;
		}
		else if (wallType == VoxelType::Wall)
		{
			const VoxelDefinition::WallData::Type wallType = wallDef.wall.type;

			if (wallType == VoxelDefinition::WallData::Type::Solid)
			{
				return AutomapWall;
			}
			else if (wallType == VoxelDefinition::WallData::Type::LevelUp)
			{
				return AutomapLevelUp;
			}
			else if (wallType == VoxelDefinition::WallData::Type::LevelDown)
			{
				return AutomapLevelDown;
			}
			else if (wallType == VoxelDefinition::WallData::Type::Menu)
			{
				// Menu blocks are the same color as doors.
				return AutomapDoor;
			}
			else
			{
				DebugLogWarning("Unrecognized wall type \"" +
					std::to_string(static_cast<int>(wallType)) + "\".");
				return AutomapNotImplemented;
			}
		}
		else if (wallType == VoxelType::Raised)
		{
			return AutomapRaised;
		}
		else if (wallType == VoxelType::Diagonal)
		{
			return AutomapFloor;
		}
		else if (wallType == VoxelType::Door)
		{
			return AutomapDoor;
		}
		else if (wallType == VoxelType::TransparentWall)
		{
			// Transparent walls with collision (hedges) are shown, while
			// ones without collision (archways) are not.
			const VoxelDefinition::TransparentWallData &transparentWallData = wallDef.transparentWall;
			return transparentWallData.collider ? AutomapWall : AutomapFloor;
		}
		else if (wallType == VoxelType::Edge)
		{
			return AutomapWall;
		}
		else
		{
			DebugLogWarning("Unrecognized wall data type \"" +
				std::to_string(static_cast<int>(wallType)) + "\".");
			return AutomapNotImplemented;
		}
	}
	else
	{
		DebugLogWarning("Unrecognized floor data type \"" +
			std::to_string(static_cast<int>(floorType)) + "\".");
		return AutomapNotImplemented;
	}
}

const Color &AutomapPanel::getWildPixelColor(const VoxelDefinition &floorDef, const VoxelDefinition &wallDef)
{
	// The wilderness automap focuses more on displaying floor voxels than wall voxels.
	// It's harder to make sense of in general compared to city and interior automaps,
	// so the colors should probably be replaceable by an option or a mod at some point.
	const VoxelType floorType = floorDef.type;
	const VoxelType wallType = wallDef.type;

	if (floorType == VoxelType::Chasm)
	{
		// The wilderness only has wet chasms, but support all of them just because.
		const VoxelDefinition::ChasmData::Type chasmType = floorDef.chasm.type;

		if (chasmType == VoxelDefinition::ChasmData::Type::Dry)
		{
			// Dry chasms are a different color if a wall is over them.
			return (wallType == VoxelType::Wall) ? AutomapWildWall : AutomapDryChasm;
		}
		else if (chasmType == VoxelDefinition::ChasmData::Type::Lava)
		{
			// Lava chasms ignore all but raised platforms.
			return (wallType == VoxelType::Raised) ? AutomapWildWall : AutomapLavaChasm;
		}
		else if (chasmType == VoxelDefinition::ChasmData::Type::Wet)
		{
			// Water chasms ignore all but raised platforms.
			return (wallType == VoxelType::Raised) ? AutomapWildWall : AutomapWetChasm;
		}
		else
		{
			DebugLogWarning("Unrecognized chasm type \"" +
				std::to_string(static_cast<int>(chasmType)) + "\".");
			return AutomapNotImplemented;
		}
	}
	else if (floorType == VoxelType::Floor)
	{
		if (wallType == VoxelType::None)
		{
			// Regular ground is transparent; all other grounds are wall color.
			const VoxelDefinition::FloorData &floorData = floorDef.floor;
			const bool isRegularGround = (floorData.id == 0) || (floorData.id == 2) ||
				(floorData.id == 3) || (floorData.id == 4);

			if (isRegularGround)
			{
				return AutomapFloor;
			}
			else
			{
				return AutomapWildWall;
			}
		}
		else if (wallType == VoxelType::Wall)
		{
			const VoxelDefinition::WallData &wallData = wallDef.wall;
			const VoxelDefinition::WallData::Type wallType = wallData.type;

			if (wallType == VoxelDefinition::WallData::Type::Solid)
			{
				return AutomapWildWall;
			}
			else if (wallType == VoxelDefinition::WallData::Type::LevelUp)
			{
				return AutomapLevelUp;
			}
			else if (wallType == VoxelDefinition::WallData::Type::LevelDown)
			{
				return AutomapLevelDown;
			}
			else if (wallType == VoxelDefinition::WallData::Type::Menu)
			{
				// Certain wilderness *MENU blocks are rendered like walls.
				const bool isHiddenMenu = !ArenaWildUtils::menuIsDisplayedInWildAutomap(wallData.menuID);

				if (isHiddenMenu)
				{
					return AutomapWildWall;
				}
				else
				{
					return AutomapWildDoor;
				}
			}
			else
			{
				DebugLogWarning("Unrecognized wall type \"" +
					std::to_string(static_cast<int>(wallType)) + "\".");
				return AutomapNotImplemented;
			}
		}
		else if (wallType == VoxelType::Raised)
		{
			return AutomapWildWall;
		}
		else if (wallType == VoxelType::Diagonal)
		{
			return AutomapFloor;
		}
		else if (wallType == VoxelType::Door)
		{
			return AutomapWildDoor;
		}
		else if (wallType == VoxelType::TransparentWall)
		{
			return AutomapFloor;
		}
		else if (wallType == VoxelType::Edge)
		{
			const VoxelDefinition::EdgeData &edgeData = wallDef.edge;

			// For some reason, most edges are hidden.
			const bool isHiddenEdge = (edgeData.facing == VoxelFacing2D::PositiveX) ||
				(edgeData.facing == VoxelFacing2D::NegativeX) ||
				(edgeData.facing == VoxelFacing2D::NegativeZ);

			if (isHiddenEdge)
			{
				return AutomapFloor;
			}
			else
			{
				return AutomapWildWall;
			}
		}
		else
		{
			DebugLogWarning("Unrecognized wall data type \"" +
				std::to_string(static_cast<int>(wallType)) + "\".");
			return AutomapNotImplemented;
		}
	}
	else
	{
		DebugLogWarning("Unrecognized floor data type \"" +
			std::to_string(static_cast<int>(floorType)) + "\".");
		return AutomapNotImplemented;
	}
}

Surface AutomapPanel::makeAutomap(const NewInt2 &playerVoxel, CardinalDirectionName playerDir,
	bool isWild, const VoxelGrid &voxelGrid)
{
	// Create scratch surface triple the size of the voxel area to display so that all directions
	// of the player's arrow are representable in the same texture. This may change in the future
	// for memory optimization.
	const int maxSurfaceSize = (RMDFile::WIDTH * 2) * AutomapPixelSize;
	Surface surface = Surface::createWithFormat(
		std::min(voxelGrid.getDepth() * AutomapPixelSize, maxSurfaceSize),
		std::min(voxelGrid.getWidth() * AutomapPixelSize, maxSurfaceSize),
		Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);

	// Fill with transparent color first (used by floor voxels).
	surface.fill(AutomapFloor.toARGB());

	// Lambda for filling in a square in the map surface. It is provided an XY pixel which is
	// expanded into a square.
	auto drawSquare = [&surface](int x, int y, const Color &color)
	{
		const int surfaceWidth = surface.getWidth();
		const int xOffset = x * AutomapPixelSize;
		const int yOffset = y * AutomapPixelSize;
		const uint32_t colorARGB = color.toARGB();
		uint32_t *pixels = static_cast<uint32_t*>(surface.getPixels());

		for (int h = 0; h < AutomapPixelSize; h++)
		{
			const int yCoord = yOffset + h;
			for (int w = 0; w < AutomapPixelSize; w++)
			{
				const int xCoord = xOffset + w;
				const int index = xCoord + (yCoord * surfaceWidth);
				pixels[index] = colorARGB;
			}
		}
	};

	auto getVoxelDef = [&voxelGrid](SNInt x, int y, WEInt z) -> const VoxelDefinition&
	{
		const uint16_t voxelID = voxelGrid.getVoxel(x, y, z);
		return voxelGrid.getVoxelDef(voxelID);
	};

	// Calculate voxel grid loop values based on whether it's the wilderness.
	SNInt startX;
	WEInt startZ;
	SNInt loopWidth;
	WEInt loopDepth;
	if (!isWild)
	{
		startX = 0;
		startZ = 0;
		loopWidth = voxelGrid.getWidth();
		loopDepth = voxelGrid.getDepth();
	}
	else
	{
		// Get relative wild origin in new coordinate system (bottom left corner).
		const NewInt2 wildOrigin = AutomapPanel::makeRelativeWildOrigin(
			playerVoxel, voxelGrid.getWidth(), voxelGrid.getDepth());

		startX = wildOrigin.x;
		startZ = wildOrigin.y;

		const int offsetDist = RMDFile::WIDTH * 2;
		loopWidth = offsetDist;
		loopDepth = offsetDist;
	}

	// For each voxel, start at the lowest Y and walk upwards.
	for (SNInt x = 0; x < loopWidth; x++)
	{
		for (WEInt z = 0; z < loopDepth; z++)
		{
			const SNInt voxelX = startX + x;
			const WEInt voxelZ = startZ + z;

			const VoxelDefinition &floorDef = getVoxelDef(voxelX, 0, voxelZ);
			const VoxelDefinition &wallDef = getVoxelDef(voxelX, 1, voxelZ);

			// Decide which color to use for the automap pixel.
			const Color &color = !isWild ?
				AutomapPanel::getPixelColor(floorDef, wallDef) :
				AutomapPanel::getWildPixelColor(floorDef, wallDef);

			// Convert world XZ coordinates to automap XY coordinates.
			const int automapX = (loopDepth - 1) - z;
			const int automapY = x;

			// Fill in the automap square.
			drawSquare(automapX, automapY, color);
		}
	}

	// Lambda for drawing the player's arrow in the automap. It's drawn differently 
	// depending on their direction.
	auto drawPlayer = [&surface](SNInt x, WEInt z, CardinalDirectionName cardinalDirection)
	{
		const int surfaceX = surface.getWidth() - AutomapPixelSize - (z * AutomapPixelSize);
		const int surfaceY = x * AutomapPixelSize;

		uint32_t *pixels = static_cast<uint32_t*>(surface.get()->pixels);

		// Draw the player's arrow within the map pixel.
		const std::vector<Int2> &offsets = AutomapPlayerArrowPatterns.at(cardinalDirection);
		for (const auto &offset : offsets)
		{
			const int index = (surfaceX + offset.x) + ((surfaceY + offset.y) * surface.getWidth());
			pixels[index] = AutomapPlayer.toARGB();
		}
	};

	// Calculate player voxel in automap. Depends on the relative origin if in the wilderness.
	SNInt playerX;
	WEInt playerZ;
	if (!isWild)
	{
		playerX = playerVoxel.x;
		playerZ = playerVoxel.y;
	}
	else
	{
		playerX = playerVoxel.x - startX;
		playerZ = playerVoxel.y - startZ;
	}

	// Draw player last. Verify that the player is within the bounds of the map before drawing.
	if ((playerX >= 0) && (playerX < loopWidth) && (playerZ >= 0) && (playerZ < loopDepth))
	{
		drawPlayer(playerX, playerZ, playerDir);
	}

	return surface;
}

Double2 AutomapPanel::makeAutomapOffset(const NewInt2 &playerVoxel, bool isWild,
	SNInt gridWidth, WEInt gridDepth)
{
	const NewDouble2 worldOffset = [&playerVoxel, isWild, gridWidth, gridDepth]()
	{
		if (!isWild)
		{
			// Cities/interiors, offset from (0, 0) of the level.
			return NewDouble2(
				static_cast<double>(playerVoxel.x) + 0.50,
				static_cast<double>((gridDepth - 1) - playerVoxel.y) + 0.50);
		}
		else
		{
			// Wilderness, offset from the relative 2x2 wild origin, dependent on player position.
			const NewInt2 relativeOrigin =
				AutomapPanel::makeRelativeWildOrigin(playerVoxel, gridWidth, gridDepth);

			// The returned value should be within [32, 95] because that's where the player loops
			// between in the original coordinates.
			return NewDouble2(
				static_cast<double>(playerVoxel.x - relativeOrigin.x) + 0.50,
				static_cast<double>(((RMDFile::WIDTH * 2) - 1) - (playerVoxel.y - relativeOrigin.y)) + 0.50);
		}
	}();

	return Double2(-worldOffset.y, -worldOffset.x);
}

NewInt2 AutomapPanel::makeRelativeWildOrigin(const NewInt2 &voxel, SNInt gridWidth, WEInt gridDepth)
{
	return ArenaWildUtils::getCenteredWildOrigin(voxel);
}

Panel::CursorData AutomapPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();

	const std::string &paletteFilename = TextureFile::fromName(TextureName::Automap);
	PaletteID paletteID;
	if (!textureManager.tryGetPaletteID(paletteFilename.c_str(), &paletteID))
	{
		DebugLogWarning("Couldn't get palette ID for \"" + paletteFilename + "\".");
		return CursorData::EMPTY;
	}

	const std::string &textureFilename = TextureFile::fromName(TextureName::QuillCursor);
	TextureID textureID;
	if (!textureManager.tryGetTextureID(textureFilename.c_str(), paletteID, renderer, &textureID))
	{
		DebugLogWarning("Couldn't get texture ID for \"" + textureFilename + "\".");
		return CursorData::EMPTY;
	}

	const Texture &texture = textureManager.getTextureHandle(textureID);
	return CursorData(&texture, CursorAlignment::BottomLeft);
}

void AutomapPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool nPressed = inputManager.keyPressed(e, SDLK_n);

	if (escapePressed || nPressed)
	{
		this->backToGameButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		// Check if "Exit" was clicked.
		if (this->backToGameButton.contains(mouseOriginalPoint))
		{
			this->backToGameButton.click(this->getGame());
		}
	}

	// @todo: text events if in text mode
}

void AutomapPanel::handleMouse(double dt)
{
	const auto &inputManager = this->getGame().getInputManager();
	const bool leftClick = inputManager.mouseButtonIsDown(SDL_BUTTON_LEFT);

	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 mouseOriginalPoint = this->getGame().getRenderer()
		.nativeToOriginal(mousePosition);

	// Check if the LMB is held on one of the compass directions.
	if (leftClick)
	{
		const double scrollSpeed = AutomapScrollSpeed * dt;

		// Modify the automap offset based on input. The directions are reversed because
		// to go right means to push the map left.
		if (RightRegion.contains(mouseOriginalPoint))
		{
			this->automapOffset = this->automapOffset - (Double2::UnitX * scrollSpeed);
		}
		else if (LeftRegion.contains(mouseOriginalPoint))
		{
			this->automapOffset = this->automapOffset + (Double2::UnitX * scrollSpeed);
		}
		else if (UpRegion.contains(mouseOriginalPoint))
		{
			this->automapOffset = this->automapOffset + (Double2::UnitY * scrollSpeed);
		}
		else if (DownRegion.contains(mouseOriginalPoint))
		{
			this->automapOffset = this->automapOffset - (Double2::UnitY * scrollSpeed);
		}
	}
}

void AutomapPanel::drawTooltip(const std::string &text, Renderer &renderer)
{
	const Texture tooltip = Panel::createTooltip(
		text, FontName::D, this->getGame().getFontLibrary(), renderer);

	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);
	const int mouseX = originalPosition.x;
	const int mouseY = originalPosition.y;
	const int x = ((mouseX + 8 + tooltip.getWidth()) < Renderer::ORIGINAL_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < Renderer::ORIGINAL_HEIGHT) ?
		(mouseY - 1) : (mouseY - tooltip.getHeight());

	renderer.drawOriginal(tooltip, x, y);
}

void AutomapPanel::tick(double dt)
{
	this->handleMouse(dt);
}

void AutomapPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw automap background.
	const auto &textureManager = this->getGame().getTextureManager();
	const TextureRef backgroundTexture = textureManager.getTextureRef(this->backgroundTextureID);
	renderer.drawOriginal(backgroundTexture.get());

	// Only draw the part of the automap within the drawing area.
	const Rect nativeDrawingArea = renderer.originalToNative(DrawingArea);
	renderer.setClipRect(&nativeDrawingArea.getRect());

	// Draw automap.
	constexpr double pixelSizeReal = static_cast<double>(AutomapPixelSize);
	const int offsetX = static_cast<int>(std::floor(this->automapOffset.x * pixelSizeReal));
	const int offsetY = static_cast<int>(std::floor(this->automapOffset.y * pixelSizeReal));
	const int mapX = (DrawingArea.getLeft() + (DrawingArea.getWidth() / 2)) + offsetX;
	const int mapY = (DrawingArea.getTop() + (DrawingArea.getHeight() / 2)) + offsetY;
	renderer.drawOriginal(this->mapTexture, mapX, mapY);

	// Reset renderer clipping to normal.
	renderer.setClipRect(nullptr);

	// Draw text: title.
	renderer.drawOriginal(this->locationTextBox->getTexture(),
		this->locationTextBox->getX(), this->locationTextBox->getY());

	// Check if the mouse is over the compass directions for tooltips.
	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);

	if (UpRegion.contains(originalPosition))
	{
		this->drawTooltip("Up", renderer);
	}
	else if (DownRegion.contains(originalPosition))
	{
		this->drawTooltip("Down", renderer);
	}
	else if (LeftRegion.contains(originalPosition))
	{
		this->drawTooltip("Left", renderer);
	}
	else if (RightRegion.contains(originalPosition))
	{
		this->drawTooltip("Right", renderer);
	}
}
