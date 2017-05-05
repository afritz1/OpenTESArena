#include <cassert>
#include <cmath>

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
#include "../Rendering/Texture.h"
#include "../World/VoxelData.h"
#include "../World/VoxelGrid.h"

namespace
{
	// Click areas for compass directions.
	const Rect UpRegion(264, 23, 14, 14);
	const Rect DownRegion(264, 60, 14, 14);
	const Rect LeftRegion(245, 41, 14, 14);
	const Rect RightRegion(284, 41, 14, 14);

	// Colors for automap pixels. Ground pixels (y == 0) are transparent.
	const Color AutomapPlayer(247, 255, 0);
	const Color AutomapWall(130, 89, 48);
	const Color AutomapDoor(146, 0, 0);
	const Color AutomapFloorUp(0, 105, 0);
	const Color AutomapFloorDown(0, 0, 255);
	const Color AutomapWater(109, 138, 174);
	const Color AutomapLava(255, 0, 0);
}

AutomapPanel::AutomapPanel(Game *game, const Double2 &playerPosition, 
	const Double2 &playerDirection, const VoxelGrid &voxelGrid, const std::string &locationName)
	: Panel(game), automapCenter(playerPosition)
{
	this->locationTextBox = [game, &locationName]()
	{
		Int2 center(120, 28);
		Color color(56, 16, 12); // Shadow color is (150, 101, 52).
		auto &font = game->getFontManager().getFont(FontName::A);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
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
		// Create scratch surface. For the purposes of the automap, left to right is
		// the Z axis, and up and down is the X axis, because north is +X in-game.
		// - To do: the SDL_Surface should have 3x3 pixels per voxel, since the player arrow is 
		//   drawn with 3 pixels in one automap "pixel". Therefore, it needs to be scaled by 3.
		SDL_Surface *surface = Surface::createSurfaceWithFormat(
			voxelGrid.getDepth(), voxelGrid.getWidth(), Renderer::DEFAULT_BPP,
			Renderer::DEFAULT_PIXELFORMAT);

		// Fill with transparent color.
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

		// For each automap pixel, walk from the highest Y to the lowest. The color depends 
		// on the last height that was reached (y == -1 is water/lava, 0 is ground, etc.).
		uint32_t *pixels = static_cast<uint32_t*>(surface->pixels);
		for (int x = 0; x < surface->h; ++x)
		{
			for (int z = 0; z < surface->w; ++z)
			{
				const int index = z + ((surface->h - 1 - x) * surface->w);
				const int yFloor = getYHitFloor(x, z);

				// Decide which color the pixel will be. Do nothing if ground (y == 0), because
				// the ground color is transparent.
				if (yFloor == -1)
				{
					// Water/lava.
					pixels[index] = AutomapWater.toARGB();
				}
				else if (yFloor > 0)
				{
					// Wall.
					pixels[index] = AutomapWall.toARGB();
				}

				// Other types eventually (doors, stairs, ...).
			}
		}

		// Draw player last. The player arrow is three pixels, dependent on direction.
		const Int2 playerVoxelPosition(
			static_cast<int>(std::floor(playerPosition.x)),
			static_cast<int>(std::floor(playerPosition.y)));
		const CardinalDirectionName cardinalDirection = 
			CardinalDirection::getDirectionName(playerDirection);

		// Verify that the player is within the bounds of the map before drawing. Coordinates 
		// are reversed because +X is north (up) and Z is aliased as Y.
		if ((playerVoxelPosition.x >= 0) && (playerVoxelPosition.x < surface->h) &&
			(playerVoxelPosition.y >= 0) && (playerVoxelPosition.y < surface->w))
		{
			const int index = playerVoxelPosition.y + 
				((surface->h - 1 - playerVoxelPosition.x) * surface->w);
			pixels[index] = AutomapPlayer.toARGB();
		}

		auto &renderer = this->getGame()->getRenderer();
		std::unique_ptr<Texture> texture(new Texture(renderer.createTextureFromSurface(surface)));
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
		// To do: scroll the map relative to delta time.
		if (UpRegion.contains(mouseOriginalPoint))
		{

		}
		else if (DownRegion.contains(mouseOriginalPoint))
		{

		}
		else if (RightRegion.contains(mouseOriginalPoint))
		{

		}
		else if (LeftRegion.contains(mouseOriginalPoint))
		{

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

	// Draw automap.
	renderer.drawToOriginal(this->mapTexture->get(), 25, 40, 
		this->mapTexture->getWidth() * 3, this->mapTexture->getHeight() * 3);

	// Draw text: title.
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
