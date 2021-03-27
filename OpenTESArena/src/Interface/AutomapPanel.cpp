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
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ArenaTypes.h"
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
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../World/ArenaWildUtils.h"
#include "../World/LevelUtils.h"
#include "../World/MapType.h"
#include "../World/VoxelDefinition.h"
#include "../World/VoxelFacing2D.h"

#include "components/debug/Debug.h"

namespace
{
	// How fast the automap moves when scrolling.
	constexpr double AutomapScrollSpeed = 100.0;

	// Size of each automap pixel in the automap texture.
	constexpr int AutomapPixelSize = 3;

	// Number of chunks away from the player to display in the automap.
	constexpr int AutomapChunkDistance = 1;

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

AutomapPanel::AutomapPanel(Game &game, const CoordDouble3 &playerCoord, const VoxelDouble2 &playerDirection,
	const ChunkManager &chunkManager, const std::string &locationName)
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
		Int2 center(ArenaRenderUtils::SCREEN_WIDTH - 57, ArenaRenderUtils::SCREEN_HEIGHT - 29);
		int width = 38;
		int height = 13;
		auto function = [](Game &game)
		{
			game.setPanel<GameWorldPanel>(game);
		};
		return Button<Game&>(center, width, height, function);
	}();

	const VoxelInt3 playerVoxel = VoxelUtils::pointToVoxel(playerCoord.point);
	const CoordInt2 playerCoordXZ(playerCoord.chunk, VoxelInt2(playerVoxel.x, playerVoxel.z));
	this->mapTexture = [&game, &playerDirection, &chunkManager, &playerCoordXZ]()
	{
		const CardinalDirectionName playerCompassDir = CardinalDirection::getDirectionName(playerDirection);
		const bool isWild = [&game]()
		{
			const GameState &gameState = game.getGameState();
			const MapDefinition &activeMapDef = gameState.getActiveMapDef();
			return activeMapDef.getMapType() == MapType::Wilderness;
		}();

		Texture texture = AutomapPanel::makeAutomap(playerCoordXZ, playerCompassDir, isWild,
			chunkManager, game.getRenderer());
		return texture;
	}();

	auto &textureManager = game.getTextureManager();
	const std::string &backgroundTextureName = ArenaTextureName::Automap;
	const std::string &backgroundPaletteName = backgroundTextureName;
	const std::optional<PaletteID> backgroundPaletteID = textureManager.tryGetPaletteID(backgroundPaletteName.c_str());
	if (!backgroundPaletteID.has_value())
	{
		DebugCrash("Couldn't get palette ID for \"" + backgroundPaletteName + "\".");
	}

	this->backgroundPaletteID = *backgroundPaletteID;

	const std::optional<TextureBuilderID> backgroundTextureBuilderID =
		textureManager.tryGetTextureBuilderID(backgroundTextureName.c_str());
	if (!backgroundTextureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + backgroundTextureName + "\".");
	}

	this->backgroundTextureBuilderID = *backgroundTextureBuilderID;
	this->automapOffset = AutomapPanel::makeAutomapOffset(playerCoordXZ.voxel);
}

const Color &AutomapPanel::getPixelColor(const VoxelDefinition &floorDef, const VoxelDefinition &wallDef,
	const TransitionDefinition *transitionDef)
{
	const ArenaTypes::VoxelType floorType = floorDef.type;
	const ArenaTypes::VoxelType wallType = wallDef.type;

	if (floorType == ArenaTypes::VoxelType::Chasm)
	{
		const ArenaTypes::ChasmType chasmType = floorDef.chasm.type;

		if (chasmType == ArenaTypes::ChasmType::Dry)
		{
			// Dry chasms are a different color if a wall is over them.
			return (wallType == ArenaTypes::VoxelType::Wall) ? AutomapRaised : AutomapDryChasm;
		}
		else if (chasmType == ArenaTypes::ChasmType::Lava)
		{
			// Lava chasms ignore all but raised platforms.
			return (wallType == ArenaTypes::VoxelType::Raised) ? AutomapRaised : AutomapLavaChasm;
		}
		else if (chasmType == ArenaTypes::ChasmType::Wet)
		{
			// Water chasms ignore all but raised platforms.
			return (wallType == ArenaTypes::VoxelType::Raised) ? AutomapRaised : AutomapWetChasm;
		}
		else
		{
			DebugLogWarning("Unrecognized chasm type \"" +
				std::to_string(static_cast<int>(chasmType)) + "\".");
			return AutomapNotImplemented;
		}
	}
	else if (floorType == ArenaTypes::VoxelType::Floor)
	{
		// If nothing is over the floor, return transparent. Otherwise, choose from
		// a number of cases.
		if (wallType == ArenaTypes::VoxelType::None)
		{
			return AutomapFloor;
		}
		else if (wallType == ArenaTypes::VoxelType::Wall)
		{
			if (transitionDef == nullptr)
			{
				// Not a transition.
				return AutomapWall;
			}
			else
			{
				const TransitionType transitionType = transitionDef->getType();
				if ((transitionType == TransitionType::CityGate) ||
					(transitionType == TransitionType::EnterInterior) ||
					(transitionType == TransitionType::ExitInterior))
				{
					return AutomapDoor;
				}
				else if (transitionType == TransitionType::LevelChange)
				{
					const TransitionDefinition::LevelChangeDef &levelChangeDef = transitionDef->getLevelChange();
					return levelChangeDef.isLevelUp ? AutomapLevelUp : AutomapLevelDown;
				}
				else
				{
					DebugLogWarning("Unrecognized transition type \"" +
						std::to_string(static_cast<int>(transitionType)) + "\".");
					return AutomapNotImplemented;
				}
			}
		}
		else if (wallType == ArenaTypes::VoxelType::Raised)
		{
			return AutomapRaised;
		}
		else if (wallType == ArenaTypes::VoxelType::Diagonal)
		{
			return AutomapFloor;
		}
		else if (wallType == ArenaTypes::VoxelType::Door)
		{
			return AutomapDoor;
		}
		else if (wallType == ArenaTypes::VoxelType::TransparentWall)
		{
			// Transparent walls with collision (hedges) are shown, while
			// ones without collision (archways) are not.
			const VoxelDefinition::TransparentWallData &transparentWallData = wallDef.transparentWall;
			return transparentWallData.collider ? AutomapWall : AutomapFloor;
		}
		else if (wallType == ArenaTypes::VoxelType::Edge)
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

const Color &AutomapPanel::getWildPixelColor(const VoxelDefinition &floorDef, const VoxelDefinition &wallDef,
	const TransitionDefinition *transitionDef)
{
	// The wilderness automap focuses more on displaying floor voxels than wall voxels.
	// It's harder to make sense of in general compared to city and interior automaps,
	// so the colors should probably be replaceable by an option or a mod at some point.
	const ArenaTypes::VoxelType floorType = floorDef.type;
	const ArenaTypes::VoxelType wallType = wallDef.type;

	if (floorType == ArenaTypes::VoxelType::Chasm)
	{
		// The wilderness only has wet chasms, but support all of them just because.
		const ArenaTypes::ChasmType chasmType = floorDef.chasm.type;

		if (chasmType == ArenaTypes::ChasmType::Dry)
		{
			// Dry chasms are a different color if a wall is over them.
			return (wallType == ArenaTypes::VoxelType::Wall) ? AutomapWildWall : AutomapDryChasm;
		}
		else if (chasmType == ArenaTypes::ChasmType::Lava)
		{
			// Lava chasms ignore all but raised platforms.
			return (wallType == ArenaTypes::VoxelType::Raised) ? AutomapWildWall : AutomapLavaChasm;
		}
		else if (chasmType == ArenaTypes::ChasmType::Wet)
		{
			// Water chasms ignore all but raised platforms.
			return (wallType == ArenaTypes::VoxelType::Raised) ? AutomapWildWall : AutomapWetChasm;
		}
		else
		{
			DebugLogWarning("Unrecognized chasm type \"" +
				std::to_string(static_cast<int>(chasmType)) + "\".");
			return AutomapNotImplemented;
		}
	}
	else if (floorType == ArenaTypes::VoxelType::Floor)
	{
		if (wallType == ArenaTypes::VoxelType::None)
		{
			// Regular ground is transparent; all other grounds are wall color.
			const VoxelDefinition::FloorData &floorData = floorDef.floor;
			const bool isRegularGround = !floorData.isWildWallColored;

			if (isRegularGround)
			{
				return AutomapFloor;
			}
			else
			{
				return AutomapWildWall;
			}
		}
		else if (wallType == ArenaTypes::VoxelType::Wall)
		{
			if (transitionDef == nullptr)
			{
				return AutomapWildWall;
			}
			else
			{
				const TransitionType transitionType = transitionDef->getType();
				if ((transitionType == TransitionType::CityGate) ||
					(transitionType == TransitionType::EnterInterior) ||
					(transitionType == TransitionType::ExitInterior))
				{
					// Certain wilderness voxels are rendered like walls.
					// @todo: may need to revisit this for the new VoxelDefinition design (see ArenaWildUtils::menuIsDisplayedInWildAutomap()).
					// - can't just rely on the new floor.isWildWallColored?
					const bool isHidden = false; // @todo
					return isHidden ? AutomapWildWall : AutomapWildDoor;
				}
				else if (transitionType == TransitionType::LevelChange)
				{
					const TransitionDefinition::LevelChangeDef &levelChangeDef = transitionDef->getLevelChange();
					return levelChangeDef.isLevelUp ? AutomapLevelUp : AutomapLevelDown;
				}
				else
				{
					DebugLogWarning("Unrecognized transition type \"" +
						std::to_string(static_cast<int>(transitionType)) + "\".");
					return AutomapNotImplemented;
				}
			}			
		}
		else if (wallType == ArenaTypes::VoxelType::Raised)
		{
			return AutomapWildWall;
		}
		else if (wallType == ArenaTypes::VoxelType::Diagonal)
		{
			return AutomapFloor;
		}
		else if (wallType == ArenaTypes::VoxelType::Door)
		{
			return AutomapWildDoor;
		}
		else if (wallType == ArenaTypes::VoxelType::TransparentWall)
		{
			return AutomapFloor;
		}
		else if (wallType == ArenaTypes::VoxelType::Edge)
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

Texture AutomapPanel::makeAutomap(const CoordInt2 &playerCoord, CardinalDirectionName playerCompassDir,
	bool isWild, const ChunkManager &chunkManager, Renderer &renderer)
{
	// Create scratch surface triple the size of the voxel area so that all directions of the player's arrow
	// are representable in the same texture. This may change in the future for memory optimization.
	constexpr int automapDim = ChunkUtils::CHUNK_DIM * ((AutomapChunkDistance * 2) + 1);
	constexpr int surfaceDim = automapDim * AutomapPixelSize;
	Surface surface = Surface::createWithFormat(surfaceDim, surfaceDim,
		Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);

	// Fill with transparent color first (used by floor voxels).
	surface.fill(AutomapFloor.toARGB());

	const ChunkInt2 &playerChunk = playerCoord.chunk;
	ChunkInt2 minChunk, maxChunk;
	ChunkUtils::getSurroundingChunks(playerChunk, AutomapChunkDistance, &minChunk, &maxChunk);

	// Lambda for filling in a chunk voxel in the map surface.
	auto drawSquare = [&surface, &minChunk, &maxChunk](const CoordInt2 &coord, const Color &color)
	{
		// The min chunk origin is at the top right corner of the texture. +X is south, +Z is west
		// (strangely, flipping the horizontal coordinate here does not mirror the resulting texture,
		// therefore the mirroring is done in the pixel drawing loop).
		const int automapX = ((coord.chunk.y - minChunk.y) * ChunkUtils::CHUNK_DIM) + coord.voxel.y;
		const int automapY = ((coord.chunk.x - minChunk.x) * ChunkUtils::CHUNK_DIM) + coord.voxel.x;

		const int surfaceWidth = surface.getWidth();
		const int xOffset = automapX * AutomapPixelSize;
		const int yOffset = automapY * AutomapPixelSize;
		const uint32_t colorARGB = color.toARGB();
		uint32_t *pixels = static_cast<uint32_t*>(surface.getPixels());

		for (int h = 0; h < AutomapPixelSize; h++)
		{
			const int yCoord = yOffset + h;
			for (int w = 0; w < AutomapPixelSize; w++)
			{
				const int xCoord = xOffset + w;
				const int index = (surfaceWidth - xCoord - 1) + (yCoord * surfaceWidth);
				pixels[index] = colorARGB;
			}
		}
	};

	// Fill in squares on the automap.
	for (SNInt chunkX = minChunk.x; chunkX <= maxChunk.x; chunkX++)
	{
		for (WEInt chunkZ = minChunk.y; chunkZ <= maxChunk.y; chunkZ++)
		{
			const ChunkInt2 chunkPos(chunkX, chunkZ);
			const Chunk *chunk = chunkManager.tryGetChunk(chunkPos);
			DebugAssert(chunk != nullptr);

			for (SNInt x = 0; x < ChunkUtils::CHUNK_DIM; x++)
			{
				for (WEInt z = 0; z < ChunkUtils::CHUNK_DIM; z++)
				{
					const Chunk::VoxelID floorVoxelID = chunk->getVoxel(x, 0, z);
					const Chunk::VoxelID wallVoxelID = chunk->getVoxel(x, 1, z);
					const VoxelDefinition &floorVoxelDef = chunk->getVoxelDef(floorVoxelID);
					const VoxelDefinition &wallVoxelDef = chunk->getVoxelDef(wallVoxelID);
					const TransitionDefinition *transitionDef = chunk->tryGetTransition(VoxelInt3(x, 1, z));

					// Decide which color to use for the automap pixel.
					const Color &color = !isWild ?
						AutomapPanel::getPixelColor(floorVoxelDef, wallVoxelDef, transitionDef) :
						AutomapPanel::getWildPixelColor(floorVoxelDef, wallVoxelDef, transitionDef);

					drawSquare(CoordInt2(chunkPos, VoxelInt2(x, z)), color);
				}
			}
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

	// Player will always be rendered in the center chunk, "local" to the rendered chunks.
	const SNInt playerLocalX = (AutomapChunkDistance * ChunkUtils::CHUNK_DIM) + playerCoord.voxel.x;
	const WEInt playerLocalZ = (AutomapChunkDistance * ChunkUtils::CHUNK_DIM) + playerCoord.voxel.y;
	drawPlayer(playerLocalX, playerLocalZ, playerCompassDir);

	return renderer.createTextureFromSurface(surface);
}

Double2 AutomapPanel::makeAutomapOffset(const VoxelInt2 &playerVoxel)
{
	// Offsets from the top-left corner of the automap texture. Always at least one full chunk because
	// the player is in the middle of the active chunks.
	const VoxelInt2 chunkOffset(
		AutomapChunkDistance * ChunkUtils::CHUNK_DIM,
		AutomapChunkDistance * ChunkUtils::CHUNK_DIM);
	const VoxelInt2 playerVoxelOffset(ChunkUtils::CHUNK_DIM - playerVoxel.y - 1, playerVoxel.x);

	// Convert to real since the automap scrolling is in vector space.
	const Double2 offsetReal = VoxelUtils::getVoxelCenter(chunkOffset + playerVoxelOffset);

	// Negate the offset so it's how much the automap is pushed. It's the vector opposite of the automap
	// origin to the player's position.
	return -offsetReal;
}

NewInt2 AutomapPanel::makeRelativeWildOrigin(const NewInt2 &voxel, SNInt gridWidth, WEInt gridDepth)
{
	return ArenaWildUtils::getCenteredWildOrigin(voxel);
}

std::optional<Panel::CursorData> AutomapPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();

	const std::string &paletteFilename = ArenaTextureName::Automap;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
	if (!paletteID.has_value())
	{
		DebugLogWarning("Couldn't get palette ID for \"" + paletteFilename + "\".");
		return std::nullopt;
	}

	const std::string &textureFilename = ArenaTextureName::QuillCursor;
	const std::optional<TextureBuilderID> textureBuilderID =
		textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugLogWarning("Couldn't get texture builder ID for \"" + textureFilename + "\".");
		return std::nullopt;
	}

	return CursorData(*textureBuilderID, *paletteID, CursorAlignment::BottomLeft);
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
	const int x = ((mouseX + 8 + tooltip.getWidth()) < ArenaRenderUtils::SCREEN_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < ArenaRenderUtils::SCREEN_HEIGHT) ?
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
	renderer.drawOriginal(this->backgroundTextureBuilderID, this->backgroundPaletteID, textureManager);

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
