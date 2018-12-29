#include <cassert>
#include <cmath>
#include <unordered_map>
#include <vector>

#include "SDL.h"

#include "AutomapPanel.h"
#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "RichTextString.h"
#include "TextBox.h"
#include "../Game/CardinalDirection.h"
#include "../Game/CardinalDirectionName.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Interface/TextAlignment.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"
#include "../Utilities/Debug.h"
#include "../World/VoxelData.h"
#include "../World/VoxelDataType.h"
#include "../World/VoxelGrid.h"

namespace std
{
	// Hash specialization, required until GCC 6.1.
	template <>
	struct hash<CardinalDirectionName>
	{
		size_t operator()(const CardinalDirectionName &x) const
		{
			return static_cast<size_t>(x);
		}
	};
}

namespace
{
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

	// Sets of sub-pixel coordinates for drawing each of the player's arrow directions. 
	// These are offsets from the top-left corner of the 3x3 map pixel that the player 
	// is in.
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
	: Panel(game), automapOffset(playerPosition)
{
	this->locationTextBox = [&game, &locationName]()
	{
		const Int2 center(120, 28);

		const RichTextString richText(
			locationName,
			FontName::A,
			Color(56, 16, 12),
			TextAlignment::Center,
			game.getFontManager());

		const TextBox::ShadowData shadowData(Color(150, 101, 52), Int2(2, 2));
		return std::make_unique<TextBox>(center, richText, &shadowData, game.getRenderer());
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

	this->mapTexture = [this, &playerPosition, &playerDirection, &voxelGrid]()
	{
		// Create scratch surface. For the purposes of the automap, the bottom left corner
		// is (0, 0), left to right is the Z axis, and up and down is the X axis, because 
		// north is +X in-game. It is scaled by 3 so that all directions of the player's 
		// arrow are representable.
		Surface surface = Surface::createWithFormat(
			voxelGrid.getDepth() * 3, voxelGrid.getWidth() * 3, Renderer::DEFAULT_BPP,
			Renderer::DEFAULT_PIXELFORMAT);

		// Fill with transparent color first (used by floor voxels).
		SDL_FillRect(surface.get(), nullptr, AutomapFloor.toARGB());

		// Lambda for filling in a square in the map surface.
		auto drawSquare = [&surface](int x, int z, const Color &color)
		{
			SDL_Rect rect;
			rect.x = z * 3;
			rect.y = surface.getHeight() - 3 - (x * 3);
			rect.w = 3;
			rect.h = 3;

			SDL_FillRect(surface.get(), &rect, color.toARGB());
		};

		auto getVoxelData = [&voxelGrid](int x, int y, int z) -> const VoxelData&
		{
			const uint16_t voxelID = voxelGrid.getVoxel(x, y, z);
			return voxelGrid.getVoxelData(voxelID);
		};

		// For each voxel, start at the lowest Y and walk upwards. The color depends 
		// on a couple factors, like whether the voxel is a wall, a door, water, etc.,
		// and some context-sensitive cases like whether a dry chasm has a wall
		// over it.
		for (int x = 0; x < voxelGrid.getWidth(); x++)
		{
			for (int z = 0; z < voxelGrid.getDepth(); z++)
			{
				const VoxelData &floorData = getVoxelData(x, 0, z);
				const VoxelData &wallData = getVoxelData(x, 1, z);

				// Decide which color to use for the automap pixel.
				const Color &color = AutomapPanel::getPixelColor(floorData, wallData);

				// Draw the automap pixel.
				drawSquare(x, z, color);
			}
		}

		// Lambda for drawing the player's arrow in the automap. It's drawn differently 
		// depending on their direction.
		auto drawPlayer = [&surface](int x, int z, const Double2 &direction)
		{
			const CardinalDirectionName cardinalDirection =
				CardinalDirection::getDirectionName(direction);

			const int surfaceX = z * 3;
			const int surfaceY = surface.getHeight() - 3 - (x * 3);

			uint32_t *pixels = static_cast<uint32_t*>(surface.get()->pixels);

			// Draw the player's arrow within the 3x3 map pixel.
			const std::vector<Int2> &offsets = AutomapPlayerArrowPatterns.at(cardinalDirection);
			for (const auto &offset : offsets)
			{
				const int index = (surfaceX + offset.x) +
					((surfaceY + offset.y) * surface.getWidth());
				pixels[index] = AutomapPlayer.toARGB();
			}
		};

		const int playerVoxelX = static_cast<int>(std::floor(playerPosition.x));
		const int playerVoxelZ = static_cast<int>(std::floor(playerPosition.y));

		// Draw player last. Verify that the player is within the bounds of the map 
		// before drawing.
		if ((playerVoxelX >= 0) && (playerVoxelX < voxelGrid.getWidth()) &&
			(playerVoxelZ >= 0) && (playerVoxelZ < voxelGrid.getDepth()))
		{
			drawPlayer(playerVoxelX, playerVoxelZ, playerDirection);
		}

		auto &renderer = this->getGame().getRenderer();
		Texture texture(renderer.createTextureFromSurface(surface.get()));

		return texture;
	}();
}

const Color &AutomapPanel::getPixelColor(const VoxelData &floorData, const VoxelData &wallData)
{
	const VoxelDataType floorDataType = floorData.dataType;
	const VoxelDataType wallDataType = wallData.dataType;

	if (floorDataType == VoxelDataType::Chasm)
	{
		const VoxelData::ChasmData::Type chasmType = floorData.chasm.type;

		if (chasmType == VoxelData::ChasmData::Type::Dry)
		{
			// Dry chasms are a different color if a wall is over them.
			return (wallDataType == VoxelDataType::Wall) ? AutomapRaised : AutomapDryChasm;
		}
		else if (chasmType == VoxelData::ChasmData::Type::Lava)
		{
			// Lava chasms ignore all but raised platforms.
			return (wallDataType == VoxelDataType::Raised) ? AutomapRaised : AutomapLavaChasm;
		}
		else if (chasmType == VoxelData::ChasmData::Type::Wet)
		{
			// Water chasms ignore all but raised platforms.
			return (wallDataType == VoxelDataType::Raised) ? AutomapRaised : AutomapWetChasm;
		}
		else
		{
			DebugWarning("Unrecognized chasm type \"" +
				std::to_string(static_cast<int>(chasmType)) + "\".");
			return AutomapNotImplemented;
		}
	}
	else if (floorDataType == VoxelDataType::Floor)
	{
		// If nothing is over the floor, return transparent. Otherwise, choose from
		// a number of cases.
		if (wallDataType == VoxelDataType::None)
		{
			return AutomapFloor;
		}
		else if (wallDataType == VoxelDataType::Wall)
		{
			const VoxelData::WallData::Type wallType = wallData.wall.type;

			if (wallType == VoxelData::WallData::Type::Solid)
			{
				return AutomapWall;
			}
			else if (wallType == VoxelData::WallData::Type::LevelUp)
			{
				return AutomapLevelUp;
			}
			else if (wallType == VoxelData::WallData::Type::LevelDown)
			{
				return AutomapLevelDown;
			}
			else if (wallType == VoxelData::WallData::Type::Menu)
			{
				// Menu blocks are the same color as doors.
				return AutomapDoor;
			}
			else
			{
				DebugWarning("Unrecognized wall type \"" +
					std::to_string(static_cast<int>(wallType)) + "\".");
				return AutomapNotImplemented;
			}
		}
		else if (wallDataType == VoxelDataType::Raised)
		{
			return AutomapRaised;
		}
		else if (wallDataType == VoxelDataType::Diagonal)
		{
			return AutomapFloor;
		}
		else if (wallDataType == VoxelDataType::Door)
		{
			return AutomapDoor;
		}
		else if (wallDataType == VoxelDataType::TransparentWall)
		{
			// Transparent walls with collision (hedges) are shown, while
			// ones without collision (archways) are not.
			const VoxelData::TransparentWallData &transparentWallData = wallData.transparentWall;
			return transparentWallData.collider ? AutomapWall : AutomapFloor;
		}
		else if (wallDataType == VoxelDataType::Edge)
		{
			return AutomapWall;
		}
		else
		{
			DebugWarning("Unrecognized wall data type \"" +
				std::to_string(static_cast<int>(wallDataType)) + "\".");
			return AutomapNotImplemented;
		}
	}
	else
	{
		DebugWarning("Unrecognized floor data type \"" +
			std::to_string(static_cast<int>(floorDataType)) + "\".");
		return AutomapNotImplemented;
	}
}

std::pair<SDL_Texture*, CursorAlignment> AutomapPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::QuillCursor),
		TextureFile::fromName(TextureName::Automap), renderer);
	return std::make_pair(texture.get(), CursorAlignment::BottomLeft);
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
		const double scrollSpeed = 100.0;

		// Modify the automap offset based on input. Use the custom coordinate system
		// with +X as north and +Z as east (aliased as Y).
		if (UpRegion.contains(mouseOriginalPoint))
		{
			this->automapOffset = this->automapOffset + (Double2::UnitX * (scrollSpeed * dt));
		}
		else if (DownRegion.contains(mouseOriginalPoint))
		{
			this->automapOffset = this->automapOffset - (Double2::UnitX * (scrollSpeed * dt));
		}
		else if (RightRegion.contains(mouseOriginalPoint))
		{
			this->automapOffset = this->automapOffset + (Double2::UnitY * (scrollSpeed * dt));
		}
		else if (LeftRegion.contains(mouseOriginalPoint))
		{
			this->automapOffset = this->automapOffset - (Double2::UnitY * (scrollSpeed * dt));
		}
	}
}

void AutomapPanel::drawTooltip(const std::string &text, Renderer &renderer)
{
	const Texture tooltip(Panel::createTooltip(
		text, FontName::D, this->getGame().getFontManager(), renderer));

	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);
	const int mouseX = originalPosition.x;
	const int mouseY = originalPosition.y;
	const int x = ((mouseX + 8 + tooltip.getWidth()) < Renderer::ORIGINAL_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < Renderer::ORIGINAL_HEIGHT) ?
		(mouseY - 1) : (mouseY - tooltip.getHeight());

	renderer.drawOriginal(tooltip.get(), x, y);
}

void AutomapPanel::tick(double dt)
{
	this->handleMouse(dt);
}

void AutomapPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Set palette.
	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw automap background.
	const auto &automapBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::Automap),
		PaletteFile::fromName(PaletteName::BuiltIn), renderer);
	renderer.drawOriginal(automapBackground.get());

	// Only draw the part of the automap within the drawing area.
	const Rect nativeDrawingArea = renderer.originalToNative(DrawingArea);
	renderer.setClipRect(&nativeDrawingArea.getRect());

	// Draw automap. Remember that +X is north and +Z is east (aliased as Y), and that
	// the map texture is scaled by 3 (for the 3x3 player pixel).
	const int offsetX = static_cast<int>(std::floor(this->automapOffset.y * 3.0));
	const int offsetY = static_cast<int>(std::floor(this->automapOffset.x * 3.0));
	const int mapX = (DrawingArea.getLeft() + (DrawingArea.getWidth() / 2)) - offsetX;
	const int mapY = (DrawingArea.getTop() + (DrawingArea.getHeight() / 2)) + offsetY -
		this->mapTexture.getHeight();
	renderer.drawOriginal(this->mapTexture.get(), mapX, mapY);

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
