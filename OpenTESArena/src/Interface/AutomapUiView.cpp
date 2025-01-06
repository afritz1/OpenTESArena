#include "AutomapUiView.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/TextureManager.h"
#include "../Game/GameState.h"
#include "../Rendering/Renderer.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../Voxels/VoxelChunk.h"
#include "../Voxels/VoxelChunkManager.h"
#include "../Voxels/VoxelFacing2D.h"
#include "../Voxels/VoxelTraitsDefinition.h"
#include "../World/CardinalDirection.h"
#include "../World/ChunkManager.h"
#include "../World/ChunkUtils.h"
#include "../World/MapType.h"
#include "../World/TransitionDefinition.h"
#include "../World/TransitionType.h"

#include "components/debug/Debug.h"

TextBox::InitInfo AutomapUiView::getLocationTextBoxInitInfo(const std::string_view text, const FontLibrary &fontLibrary)
{
	const TextRenderUtils::TextShadowInfo shadowInfo(
		AutomapUiView::LocationTextBoxShadowOffsetX,
		AutomapUiView::LocationTextBoxShadowOffsetY,
		AutomapUiView::LocationTextBoxShadowColor);

	return TextBox::InitInfo::makeWithCenter(
		text,
		AutomapUiView::LocationTextBoxCenterPoint,
		AutomapUiView::LocationTextBoxFontName,
		AutomapUiView::LocationTextBoxFontColor,
		AutomapUiView::LocationTextBoxTextAlignment,
		shadowInfo,
		0,
		fontLibrary);
}

TextureAsset AutomapUiView::getBackgroundTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::Automap));
}

TextureAsset AutomapUiView::getBackgroundPaletteTextureAsset()
{
	return AutomapUiView::getBackgroundTextureAsset();
}

TextureAsset AutomapUiView::getCursorTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::QuillCursor));
}

TextureAsset AutomapUiView::getCursorPaletteTextureAsset()
{
	return AutomapUiView::getBackgroundPaletteTextureAsset();
}

const Color &AutomapUiView::getPixelColor(const VoxelTraitsDefinition &floorDef, const VoxelTraitsDefinition &wallDef,
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
			DebugLogWarning("Unrecognized chasm type \"" + std::to_string(static_cast<int>(chasmType)) + "\".");
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
				const TransitionType transitionType = transitionDef->type;
				if ((transitionType == TransitionType::CityGate) ||
					(transitionType == TransitionType::EnterInterior) ||
					(transitionType == TransitionType::ExitInterior))
				{
					return AutomapUiView::ColorDoor;
				}
				else if (transitionType == TransitionType::InteriorLevelChange)
				{
					const InteriorLevelChangeTransitionDefinition &levelChangeDef = transitionDef->interiorLevelChange;
					return levelChangeDef.isLevelUp ? AutomapUiView::ColorLevelUp : AutomapUiView::ColorLevelDown;
				}
				else
				{
					DebugLogWarning("Unrecognized transition type \"" + std::to_string(static_cast<int>(transitionType)) + "\".");
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
			// Transparent walls with collision (hedges) are shown, while ones without collision (archways) are not.
			const VoxelTraitsDefinition::TransparentWall &transparentWall = wallDef.transparentWall;
			return transparentWall.collider ? AutomapUiView::ColorWall : AutomapUiView::ColorFloor;
		}
		else if (wallType == ArenaTypes::VoxelType::Edge)
		{
			return AutomapUiView::ColorWall;
		}
		else
		{
			DebugLogWarning("Unrecognized wall data type \"" + std::to_string(static_cast<int>(wallType)) + "\".");
			return AutomapUiView::ColorNotImplemented;
		}
	}
	else
	{
		DebugLogWarning("Unrecognized floor data type \"" + std::to_string(static_cast<int>(floorType)) + "\".");
		return AutomapUiView::ColorNotImplemented;
	}
}

const Color &AutomapUiView::getWildPixelColor(const VoxelTraitsDefinition &floorDef, const VoxelTraitsDefinition &wallDef,
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
			DebugLogWarning("Unrecognized chasm type \"" + std::to_string(static_cast<int>(chasmType)) + "\".");
			return AutomapUiView::ColorNotImplemented;
		}
	}
	else if (floorType == ArenaTypes::VoxelType::Floor)
	{
		if (wallType == ArenaTypes::VoxelType::None)
		{
			// Regular ground is transparent; all other grounds are wall color.
			const VoxelTraitsDefinition::Floor &floor = floorDef.floor;
			const bool isRegularGround = !floor.isWildWallColored;

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
				const TransitionType transitionType = transitionDef->type;
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
				else if (transitionType == TransitionType::InteriorLevelChange)
				{
					const InteriorLevelChangeTransitionDefinition &levelChangeDef = transitionDef->interiorLevelChange;
					return levelChangeDef.isLevelUp ? AutomapUiView::ColorLevelUp : AutomapUiView::ColorLevelDown;
				}
				else
				{
					DebugLogWarning("Unrecognized transition type \"" + std::to_string(static_cast<int>(transitionType)) + "\".");
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
			const VoxelTraitsDefinition::Edge &edge = wallDef.edge;

			// For some reason, most edges are hidden.
			const bool isHiddenEdge = (edge.facing == VoxelFacing2D::PositiveX) ||
				(edge.facing == VoxelFacing2D::NegativeX) ||
				(edge.facing == VoxelFacing2D::NegativeZ);

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

Buffer2D<uint32_t> AutomapUiView::makeAutomap(const CoordInt2 &playerCoord, CardinalDirectionName playerCompassDir,
	bool isWild, const WorldInt2 &levelDims, const VoxelChunkManager &voxelChunkManager)
{
	// Create scratch surface triple the size of the voxel area so that all directions of the player's arrow
	// are representable in the same texture. This may change in the future for memory optimization.
	constexpr int automapDim = ChunkUtils::CHUNK_DIM * ((AutomapUiView::ChunkDistance * 2) + 1);
	constexpr int surfaceDim = automapDim * AutomapUiView::PixelSize;
	Buffer2D<uint32_t> dstBuffer(surfaceDim, surfaceDim);

	// Fill with transparent color first (used by floor voxels).
	const Color &floorColor = AutomapUiView::ColorFloor;
	dstBuffer.fill(floorColor.toARGB());

	const ChunkInt2 &playerChunk = playerCoord.chunk;
	ChunkInt2 minChunk, maxChunk;
	ChunkUtils::getSurroundingChunks(playerChunk, AutomapUiView::ChunkDistance, &minChunk, &maxChunk);

	// Lambda for filling in a chunk voxel in the map surface.
	auto drawSquare = [&dstBuffer, &minChunk, &maxChunk](const CoordInt2 &coord, const Color &color)
	{
		// The min chunk origin is at the top right corner of the texture. +X is south, +Z is west
		// (strangely, flipping the horizontal coordinate here does not mirror the resulting texture,
		// therefore the mirroring is done in the pixel drawing loop).
		const int automapX = ((coord.chunk.y - minChunk.y) * ChunkUtils::CHUNK_DIM) + coord.voxel.y;
		const int automapY = ((coord.chunk.x - minChunk.x) * ChunkUtils::CHUNK_DIM) + coord.voxel.x;

		const int surfaceWidth = dstBuffer.getWidth();
		const int xOffset = automapX * AutomapUiView::PixelSize;
		const int yOffset = automapY * AutomapUiView::PixelSize;
		const uint32_t colorARGB = color.toARGB();
		uint32_t *pixels = dstBuffer.begin();

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
			const VoxelChunk &chunk = voxelChunkManager.getChunkAtPosition(chunkPos);

			for (SNInt x = 0; x < ChunkUtils::CHUNK_DIM; x++)
			{
				for (WEInt z = 0; z < ChunkUtils::CHUNK_DIM; z++)
				{
					const VoxelTraitsDefID floorVoxelTraitsDefID = chunk.getTraitsDefID(x, 0, z);
					const VoxelTraitsDefID wallVoxelTraitsDefID = chunk.getTraitsDefID(x, 1, z);
					const VoxelTraitsDefinition &floorVoxelTraitsDef = chunk.getTraitsDef(floorVoxelTraitsDefID);
					const VoxelTraitsDefinition &wallVoxelTraitsDef = chunk.getTraitsDef(wallVoxelTraitsDefID);
					
					VoxelTransitionDefID transitionDefID;
					const TransitionDefinition *transitionDef = nullptr;
					if (chunk.tryGetTransitionDefID(x, 1, z, &transitionDefID))
					{
						transitionDef = &chunk.getTransitionDef(transitionDefID);
					}

					// Decide which color to use for the automap pixel.
					Color color;
					if (isWild)
					{
						color = AutomapUiView::getWildPixelColor(floorVoxelTraitsDef, wallVoxelTraitsDef, transitionDef);
					}
					else
					{
						// @todo: make a coord-to-level-voxel function for this
						const WorldInt2 levelPos(
							(chunkPos.x * ChunkUtils::CHUNK_DIM) + x,
							(chunkPos.y * ChunkUtils::CHUNK_DIM) + z);
						const bool isInsideLevelBounds = (chunkX >= 0) && (chunkZ >= 0) && (levelPos.x < levelDims.x) && (levelPos.y < levelDims.y);

						if (isInsideLevelBounds)
						{
							color = AutomapUiView::getPixelColor(floorVoxelTraitsDef, wallVoxelTraitsDef, transitionDef);
						}
						else
						{
							color = floorColor;
						}
					}

					drawSquare(CoordInt2(chunkPos, VoxelInt2(x, z)), color);
				}
			}
		}
	}

	// Lambda for drawing the player's arrow in the automap. It's drawn differently 
	// depending on their direction.
	auto drawPlayer = [&dstBuffer](SNInt x, WEInt z, CardinalDirectionName cardinalDirection)
	{
		const int surfaceX = dstBuffer.getWidth() - AutomapUiView::PixelSize - (z * AutomapUiView::PixelSize);
		const int surfaceY = x * AutomapUiView::PixelSize;

		uint32_t *pixels = dstBuffer.begin();

		// Draw the player's arrow within the map pixel.
		BufferView<const Int2> offsets = AutomapUiView::PlayerArrowPatterns.at(cardinalDirection);
		for (const Int2 &offset : offsets)
		{
			const int index = (surfaceX + offset.x) + ((surfaceY + offset.y) * dstBuffer.getWidth());
			pixels[index] = AutomapUiView::ColorPlayer.toARGB();
		}
	};

	// Player will always be rendered in the center chunk, "local" to the rendered chunks.
	const SNInt playerLocalX = (AutomapUiView::ChunkDistance * ChunkUtils::CHUNK_DIM) + playerCoord.voxel.x;
	const WEInt playerLocalZ = (AutomapUiView::ChunkDistance * ChunkUtils::CHUNK_DIM) + playerCoord.voxel.y;
	drawPlayer(playerLocalX, playerLocalZ, playerCompassDir);

	return dstBuffer;
}

UiTextureID AutomapUiView::allocMapTexture(const GameState &gameState, const CoordInt2 &playerCoordXZ,
	const VoxelDouble2 &playerDirection, const VoxelChunkManager &voxelChunkManager, Renderer &renderer)
{
	const CardinalDirectionName playerCompassDir = CardinalDirection::getDirectionName(playerDirection);
	const MapDefinition &activeMapDef = gameState.getActiveMapDef();
	const bool isWild = activeMapDef.getMapType() == MapType::Wilderness;
	const BufferView<const LevelDefinition> levelDefs = activeMapDef.getLevels();
	const LevelDefinition &activeLevelDef = levelDefs[gameState.getActiveLevelIndex()];
	const WorldInt2 levelDims(activeLevelDef.getWidth(), activeLevelDef.getDepth());

	Buffer2D<uint32_t> automapBuffer = AutomapUiView::makeAutomap(playerCoordXZ, playerCompassDir, isWild, levelDims, voxelChunkManager);
	const BufferView2D<const uint32_t> automapBufferView(automapBuffer);

	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(automapBufferView, &textureID))
	{
		DebugCrash("Couldn't create UI texture for automap.");
	}

	return textureID;
}

UiTextureID AutomapUiView::allocBgTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset paletteTextureAsset = AutomapUiView::getBackgroundPaletteTextureAsset();
	const TextureAsset textureAsset = AutomapUiView::getBackgroundTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for automap background.");
	}

	return textureID;
}

UiTextureID AutomapUiView::allocCursorTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset paletteTextureAsset = AutomapUiView::getCursorPaletteTextureAsset();
	const TextureAsset textureAsset = AutomapUiView::getCursorTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for automap cursor.");
	}

	return textureID;
}
