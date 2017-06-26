#include <cassert>
#include <cmath>
#include <map>
#include <vector>

#include "SDL.h"

#include "AutomapPanel.h"

#include "GameWorldPanel.h"
#include "TextBox.h"
#include "../Game/CardinalDirection.h"
#include "../Game/CardinalDirectionName.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Interface/TextAlignment.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"
#include "../World/VoxelData.h"
#include "../World/VoxelGrid.h"

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
	const Color AutomapWall(130, 89, 48);
	const Color AutomapDoor(146, 0, 0);
	const Color AutomapFloorUp(0, 105, 0);
	const Color AutomapFloorDown(0, 0, 255);
	const Color AutomapWater(109, 138, 174);
	const Color AutomapLava(255, 0, 0);

	// Sets of sub-pixel coordinates for drawing each of the player's arrow directions. 
	// These are offsets from the top-left corner of the 3x3 map pixel that the player 
	// is in.
	const std::map<CardinalDirectionName, std::vector<Int2>> AutomapPlayerArrowPatterns =
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

AutomapPanel::AutomapPanel(Game *game, const Double2 &playerPosition,
	const Double2 &playerDirection, const VoxelGrid &voxelGrid, const std::string &locationName)
	: Panel(game), automapOffset(playerPosition)
{
	this->locationTextBox = [game, &locationName]()
	{
		Int2 center(120, 28);
		Color color(56, 16, 12);
		Color shadowColor(150, 101, 52);
		auto &font = game->getFontManager().getFont(FontName::A);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			shadowColor,
			locationName,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->backToGameButton = []()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH - 57, Renderer::ORIGINAL_HEIGHT - 29);
		int width = 38;
		int height = 13;
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> gamePanel(new GameWorldPanel(game));
			game->setPanel(std::move(gamePanel));
		};
		return std::unique_ptr<Button<Game*>>(
			new Button<Game*>(center, width, height, function));
	}();

	this->mapTexture = [this, &playerPosition, &playerDirection, &voxelGrid]()
	{
		// Create scratch surface. For the purposes of the automap, the bottom left corner
		// is (0, 0), left to right is the Z axis, and up and down is the X axis, because 
		// north is +X in-game. It is scaled by 3 so that all directions of the player's 
		// arrow are representable.
		SDL_Surface *surface = Surface::createSurfaceWithFormat(
			voxelGrid.getDepth() * 3, voxelGrid.getWidth() * 3, Renderer::DEFAULT_BPP,
			Renderer::DEFAULT_PIXELFORMAT);

		// Fill with transparent color first.
		SDL_FillRect(surface, nullptr, SDL_MapRGBA(surface->format, 0, 0, 0, 0));

		// Lambda for returning the Y coordinate of the highest non-air voxel in a column.
		auto getYHitFloor = [&voxelGrid](int x, int z)
		{
			const char *voxels = voxelGrid.getVoxels();
			for (int y = (voxelGrid.getHeight() - 1); y >= 0; --y)
			{
				const int index = x + (y * voxelGrid.getWidth()) +
					(z * voxelGrid.getWidth() * voxelGrid.getHeight());

				// If not air, then get the Y coordinate.
				if (voxels[index] != 0)
				{
					return y;
				}
			}

			// No solid voxel was hit, so return below the ground floor.
			// - Perhaps the voxel grid should store the "kind" of water (i.e., lava)?
			return -1;
		};

		// Lambda for filling a square in the map surface.
		auto drawSquare = [surface](int x, int z, uint32_t color)
		{
			SDL_Rect rect;
			rect.x = z * 3;
			rect.y = surface->h - 3 - (x * 3);
			rect.w = 3;
			rect.h = 3;

			SDL_FillRect(surface, &rect, color);
		};

		// For each voxel, walk from the highest Y to the lowest. The color depends 
		// on the final height that was reached (y == -1 is water/lava, 0 is ground, etc.).
		for (int x = 0; x < voxelGrid.getWidth(); ++x)
		{
			for (int z = 0; z < voxelGrid.getDepth(); ++z)
			{
				const int yFloor = getYHitFloor(x, z);

				// Decide which color the pixel will be. Do nothing if ground (y == 0), because
				// the ground color is transparent.
				if (yFloor == -1)
				{
					// Water/lava.
					drawSquare(x, z, AutomapWater.toARGB());
				}
				else if (yFloor > 0)
				{
					// Wall.
					drawSquare(x, z, AutomapWall.toARGB());
				}

				// To do: Other types eventually (red doors, blue/green stairs, ...). Some
				// voxels have priority even if they are between voxels in a column, so this
				// drawing loop will eventually need to find the highest priority color instead 
				// of the first hit.
			}
		}

		// Lambda for drawing the player's arrow in the automap. It's drawn differently 
		// depending on their direction.
		auto drawPlayer = [surface](int x, int z, const Double2 &direction)
		{
			const CardinalDirectionName cardinalDirection =
				CardinalDirection::getDirectionName(direction);

			const int surfaceX = z * 3;
			const int surfaceY = surface->h - 3 - (x * 3);

			uint32_t *pixels = static_cast<uint32_t*>(surface->pixels);

			// Draw the player's arrow within the 3x3 map pixel.
			const std::vector<Int2> &offsets = AutomapPlayerArrowPatterns.at(cardinalDirection);
			for (const auto &offset : offsets)
			{
				const int index = (surfaceX + offset.x) +
					((surfaceY + offset.y) * surface->w);
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

		auto &renderer = this->getGame()->getRenderer();
		Texture texture(renderer.createTextureFromSurface(surface));
		SDL_FreeSurface(surface);

		return texture;
	}();
}

AutomapPanel::~AutomapPanel()
{

}

void AutomapPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame()->getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool nPressed = inputManager.keyPressed(e, SDLK_n);

	if (escapePressed || nPressed)
	{
		this->backToGameButton->click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
			.nativePointToOriginal(mousePosition);

		// Check if "Exit" was clicked.
		if (this->backToGameButton->contains(mouseOriginalPoint))
		{
			this->backToGameButton->click(this->getGame());
		}
	}
}

void AutomapPanel::handleMouse(double dt)
{
	static_cast<void>(dt);

	const auto &inputManager = this->getGame()->getInputManager();
	const bool leftClick = inputManager.mouseButtonIsDown(SDL_BUTTON_LEFT);

	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
		.nativePointToOriginal(mousePosition);

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
	const Font &font = this->getGame()->getFontManager().getFont(FontName::D);

	Texture tooltip(Panel::createTooltip(text, font, renderer));

	const auto &inputManager = this->getGame()->getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativePointToOriginal(mousePosition);
	const int mouseX = originalPosition.x;
	const int mouseY = originalPosition.y;
	const int x = ((mouseX + 8 + tooltip.getWidth()) < Renderer::ORIGINAL_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < Renderer::ORIGINAL_HEIGHT) ?
		(mouseY - 1) : (mouseY - tooltip.getHeight());

	renderer.drawToOriginal(tooltip.get(), x, y);
}

void AutomapPanel::tick(double dt)
{
	this->handleMouse(dt);
}

void AutomapPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGame()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw automap background.
	const auto &automapBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::Automap),
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawToOriginal(automapBackground.get());

	// Only draw the part of the automap within the drawing area.
	renderer.setClipRect(DrawingArea.getRect());

	// Draw automap. Remember that +X is north and +Z is east (aliased as Y), and that
	// the map texture is scaled by 3 (for the 3x3 player pixel).
	const int offsetX = static_cast<int>(std::floor(this->automapOffset.y * 3.0));
	const int offsetY = static_cast<int>(std::floor(this->automapOffset.x * 3.0));
	const int mapX = (DrawingArea.getLeft() + (DrawingArea.getWidth() / 2)) - offsetX;
	const int mapY = (DrawingArea.getTop() + (DrawingArea.getHeight() / 2)) + offsetY - 
		this->mapTexture.getHeight();
	renderer.drawToOriginal(this->mapTexture.get(), mapX, mapY);

	// Reset renderer clipping to normal.
	renderer.setClipRect(nullptr);

	// Draw text: title.
	renderer.drawToOriginal(this->locationTextBox->getShadowTexture(),
		this->locationTextBox->getX() + 2, this->locationTextBox->getY() + 2);
	renderer.drawToOriginal(this->locationTextBox->getTexture(),
		this->locationTextBox->getX(), this->locationTextBox->getY());

	// Check if the mouse is over the compass directions for tooltips.
	const auto &inputManager = this->getGame()->getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativePointToOriginal(mousePosition);

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

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();

	// Draw quill cursor. This one uses a different point for blitting because 
	// the tip of the cursor is at the bottom left, not the top left.
	const auto &cursor = textureManager.getTexture(
		TextureFile::fromName(TextureName::QuillCursor),
		TextureFile::fromName(TextureName::Automap));
	const auto &options = this->getGame()->getOptions();
	const int cursorYOffset = static_cast<int>(
		static_cast<double>(cursor.getHeight()) * options.getCursorScale());
	renderer.drawToNative(cursor.get(),
		mousePosition.x,
		mousePosition.y - cursorYOffset,
		static_cast<int>(cursor.getWidth() * options.getCursorScale()),
		static_cast<int>(cursor.getHeight() * options.getCursorScale()));
}
