#include "AutomapUiView.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ArenaTypes.h"
#include "../Rendering/Renderer.h"
#include "../UI/Surface.h"
#include "../World/Chunk.h"
#include "../World/ChunkManager.h"
#include "../World/ChunkUtils.h"
#include "../World/TransitionDefinition.h"
#include "../World/TransitionType.h"
#include "../World/VoxelDefinition.h"
#include "../World/VoxelFacing2D.h"

#include "components/debug/Debug.h"

TextureAssetReference AutomapUiView::getBackgroundTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::Automap));
}

TextureAssetReference AutomapUiView::getBackgroundPaletteTextureAssetRef()
{
	return AutomapUiView::getBackgroundTextureAssetRef();
}

TextureAssetReference AutomapUiView::getCursorTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::QuillCursor));
}

TextureAssetReference AutomapUiView::getCursorPaletteTextureAssetRef()
{
	return AutomapUiView::getBackgroundPaletteTextureAssetRef();
}

const Color &AutomapUiView::getPixelColor(const VoxelDefinition &floorDef, const VoxelDefinition &wallDef,
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
			return (wallType == ArenaTypes::VoxelType::Wall) ? AutomapUiView::ColorRaised : AutomapUiView::ColorDryChasm;
		}
		else if (chasmType == ArenaTypes::ChasmType::Lava)
		{
			// Lava chasms ignore all but raised platforms.
			return (wallType == ArenaTypes::VoxelType::Raised) ? AutomapUiView::ColorRaised : AutomapUiView::ColorLavaChasm;
		}
		else if (chasmType == ArenaTypes::ChasmType::Wet)
		{
			// Water chasms ignore all but raised platforms.
			return (wallType == ArenaTypes::VoxelType::Raised) ? AutomapUiView::ColorRaised : AutomapUiView::ColorWetChasm;
		}
		else
		{
			DebugLogWarning("Unrecognized chasm type \"" +
				std::to_string(static_cast<int>(chasmType)) + "\".");
			return AutomapUiView::ColorNotImplemented;
		}
	}
	else if (floorType == ArenaTypes::VoxelType::Floor)
	{
		// If nothing is over the floor, return transparent. Otherwise, choose from
		// a number of cases.
		if (wallType == ArenaTypes::VoxelType::None)
		{
			return AutomapUiView::ColorFloor;
		}
		else if (wallType == ArenaTypes::VoxelType::Wall)
		{
			if (transitionDef == nullptr)
			{
				// Not a transition.
				return AutomapUiView::ColorWall;
			}
			else
			{
				const TransitionType transitionType = transitionDef->getType();
				if ((transitionType == TransitionType::CityGate) ||
					(transitionType == TransitionType::EnterInterior) ||
					(transitionType == TransitionType::ExitInterior))
				{
					return AutomapUiView::ColorDoor;
				}
				else if (transitionType == TransitionType::LevelChange)
				{
					const TransitionDefinition::LevelChangeDef &levelChangeDef = transitionDef->getLevelChange();
					return levelChangeDef.isLevelUp ? AutomapUiView::ColorLevelUp : AutomapUiView::ColorLevelDown;
				}
				else
				{
					DebugLogWarning("Unrecognized transition type \"" +
						std::to_string(static_cast<int>(transitionType)) + "\".");
					return AutomapUiView::ColorNotImplemented;
				}
			}
		}
		else if (wallType == ArenaTypes::VoxelType::Raised)
		{
			return AutomapUiView::ColorRaised;
		}
		else if (wallType == ArenaTypes::VoxelType::Diagonal)
		{
			return AutomapUiView::ColorFloor;
		}
		else if (wallType == ArenaTypes::VoxelType::Door)
		{
			return AutomapUiView::ColorDoor;
		}
		else if (wallType == ArenaTypes::VoxelType::TransparentWall)
		{
			// Transparent walls with collision (hedges) are shown, while
			// ones without collision (archways) are not.
			const VoxelDefinition::TransparentWallData &transparentWallData = wallDef.transparentWall;
			return transparentWallData.collider ? AutomapUiView::ColorWall : AutomapUiView::ColorFloor;
		}
		else if (wallType == ArenaTypes::VoxelType::Edge)
		{
			return AutomapUiView::ColorWall;
		}
		else
		{
			DebugLogWarning("Unrecognized wall data type \"" +
				std::to_string(static_cast<int>(wallType)) + "\".");
			return AutomapUiView::ColorNotImplemented;
		}
	}
	else
	{
		DebugLogWarning("Unrecognized floor data type \"" +
			std::to_string(static_cast<int>(floorType)) + "\".");
		return AutomapUiView::ColorNotImplemented;
	}
}

const Color &AutomapUiView::getWildPixelColor(const VoxelDefinition &floorDef, const VoxelDefinition &wallDef,
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
			return (wallType == ArenaTypes::VoxelType::Wall) ? AutomapUiView::ColorWildWall : AutomapUiView::ColorDryChasm;
		}
		else if (chasmType == ArenaTypes::ChasmType::Lava)
		{
			// Lava chasms ignore all but raised platforms.
			return (wallType == ArenaTypes::VoxelType::Raised) ? AutomapUiView::ColorWildWall : AutomapUiView::ColorLavaChasm;
		}
		else if (chasmType == ArenaTypes::ChasmType::Wet)
		{
			// Water chasms ignore all but raised platforms.
			return (wallType == ArenaTypes::VoxelType::Raised) ? AutomapUiView::ColorWildWall : AutomapUiView::ColorWetChasm;
		}
		else
		{
			DebugLogWarning("Unrecognized chasm type \"" +
				std::to_string(static_cast<int>(chasmType)) + "\".");
			return AutomapUiView::ColorNotImplemented;
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
				return AutomapUiView::ColorFloor;
			}
			else
			{
				return AutomapUiView::ColorWildWall;
			}
		}
		else if (wallType == ArenaTypes::VoxelType::Wall)
		{
			if (transitionDef == nullptr)
			{
				return AutomapUiView::ColorWildWall;
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
					return isHidden ? AutomapUiView::ColorWildWall : AutomapUiView::ColorWildDoor;
				}
				else if (transitionType == TransitionType::LevelChange)
				{
					const TransitionDefinition::LevelChangeDef &levelChangeDef = transitionDef->getLevelChange();
					return levelChangeDef.isLevelUp ? AutomapUiView::ColorLevelUp : AutomapUiView::ColorLevelDown;
				}
				else
				{
					DebugLogWarning("Unrecognized transition type \"" +
						std::to_string(static_cast<int>(transitionType)) + "\".");
					return AutomapUiView::ColorNotImplemented;
				}
			}
		}
		else if (wallType == ArenaTypes::VoxelType::Raised)
		{
			return AutomapUiView::ColorWildWall;
		}
		else if (wallType == ArenaTypes::VoxelType::Diagonal)
		{
			return AutomapUiView::ColorFloor;
		}
		else if (wallType == ArenaTypes::VoxelType::Door)
		{
			return AutomapUiView::ColorWildDoor;
		}
		else if (wallType == ArenaTypes::VoxelType::TransparentWall)
		{
			return AutomapUiView::ColorFloor;
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
				return AutomapUiView::ColorFloor;
			}
			else
			{
				return AutomapUiView::ColorWildWall;
			}
		}
		else
		{
			DebugLogWarning("Unrecognized wall data type \"" +
				std::to_string(static_cast<int>(wallType)) + "\".");
			return AutomapUiView::ColorNotImplemented;
		}
	}
	else
	{
		DebugLogWarning("Unrecognized floor data type \"" +
			std::to_string(static_cast<int>(floorType)) + "\".");
		return AutomapUiView::ColorNotImplemented;
	}
}

Texture AutomapUiView::makeAutomap(const CoordInt2 &playerCoord, CardinalDirectionName playerCompassDir, bool isWild,
	const ChunkManager &chunkManager, Renderer &renderer)
{
	// Create scratch surface triple the size of the voxel area so that all directions of the player's arrow
	// are representable in the same texture. This may change in the future for memory optimization.
	constexpr int automapDim = ChunkUtils::CHUNK_DIM * ((AutomapUiView::ChunkDistance * 2) + 1);
	constexpr int surfaceDim = automapDim * AutomapUiView::PixelSize;
	Surface surface = Surface::createWithFormat(surfaceDim, surfaceDim,
		Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);

	// Fill with transparent color first (used by floor voxels).
	surface.fill(AutomapUiView::ColorFloor.toARGB());

	const ChunkInt2 &playerChunk = playerCoord.chunk;
	ChunkInt2 minChunk, maxChunk;
	ChunkUtils::getSurroundingChunks(playerChunk, AutomapUiView::ChunkDistance, &minChunk, &maxChunk);

	// Lambda for filling in a chunk voxel in the map surface.
	auto drawSquare = [&surface, &minChunk, &maxChunk](const CoordInt2 &coord, const Color &color)
	{
		// The min chunk origin is at the top right corner of the texture. +X is south, +Z is west
		// (strangely, flipping the horizontal coordinate here does not mirror the resulting texture,
		// therefore the mirroring is done in the pixel drawing loop).
		const int automapX = ((coord.chunk.y - minChunk.y) * ChunkUtils::CHUNK_DIM) + coord.voxel.y;
		const int automapY = ((coord.chunk.x - minChunk.x) * ChunkUtils::CHUNK_DIM) + coord.voxel.x;

		const int surfaceWidth = surface.getWidth();
		const int xOffset = automapX * AutomapUiView::PixelSize;
		const int yOffset = automapY * AutomapUiView::PixelSize;
		const uint32_t colorARGB = color.toARGB();
		uint32_t *pixels = static_cast<uint32_t*>(surface.getPixels());

		for (int h = 0; h < AutomapUiView::PixelSize; h++)
		{
			const int yCoord = yOffset + h;
			for (int w = 0; w < AutomapUiView::PixelSize; w++)
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
						AutomapUiView::getPixelColor(floorVoxelDef, wallVoxelDef, transitionDef) :
						AutomapUiView::getWildPixelColor(floorVoxelDef, wallVoxelDef, transitionDef);

					drawSquare(CoordInt2(chunkPos, VoxelInt2(x, z)), color);
				}
			}
		}
	}

	// Lambda for drawing the player's arrow in the automap. It's drawn differently 
	// depending on their direction.
	auto drawPlayer = [&surface](SNInt x, WEInt z, CardinalDirectionName cardinalDirection)
	{
		const int surfaceX = surface.getWidth() - AutomapUiView::PixelSize - (z * AutomapUiView::PixelSize);
		const int surfaceY = x * AutomapUiView::PixelSize;

		uint32_t *pixels = static_cast<uint32_t*>(surface.get()->pixels);

		// Draw the player's arrow within the map pixel.
		const std::vector<Int2> &offsets = AutomapUiView::PlayerArrowPatterns.at(cardinalDirection);
		for (const auto &offset : offsets)
		{
			const int index = (surfaceX + offset.x) + ((surfaceY + offset.y) * surface.getWidth());
			pixels[index] = AutomapUiView::ColorPlayer.toARGB();
		}
	};

	// Player will always be rendered in the center chunk, "local" to the rendered chunks.
	const SNInt playerLocalX = (AutomapUiView::ChunkDistance * ChunkUtils::CHUNK_DIM) + playerCoord.voxel.x;
	const WEInt playerLocalZ = (AutomapUiView::ChunkDistance * ChunkUtils::CHUNK_DIM) + playerCoord.voxel.y;
	drawPlayer(playerLocalX, playerLocalZ, playerCompassDir);

	return renderer.createTextureFromSurface(surface);
}
