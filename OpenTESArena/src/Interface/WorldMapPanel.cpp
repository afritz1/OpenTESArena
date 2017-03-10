#include <cassert>

#include "SDL.h"

#include "WorldMapPanel.h"

#include "GameWorldPanel.h"
#include "ProvinceMapPanel.h"
#include "TextBox.h"
#include "../Game/Game.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"

namespace
{
	// Mouse click areas for the world map, ordered by how Arena originally
	// indexes them (read top left to bottom right on world map, center province 
	// is last).
	// - Eventually replace this with an index into the IMG file.
	const std::vector<Rect> ProvinceClickAreas =
	{
		Rect(52, 51, 44, 11),
		Rect(72, 75, 50, 11),
		Rect(142, 44, 34, 11),
		Rect(222, 84, 52, 11),
		Rect(37, 149, 49, 19),
		Rect(106, 147, 49, 10),
		Rect(148, 127, 37, 11),
		Rect(216, 144, 55, 12),
		Rect(133, 105, 83, 11)
	};
}

WorldMapPanel::WorldMapPanel(Game *game)
	: Panel(game)
{
	this->backToGameButton = []()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH - 22, Renderer::ORIGINAL_HEIGHT - 7);
		int width = 36;
		int height = 9;
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> gamePanel(new GameWorldPanel(game));
			game->setPanel(std::move(gamePanel));
		};
		return std::unique_ptr<Button<>>(new Button<>(center, width, height, function));
	}();

	this->provinceButton = [this]()
	{
		auto function = [this](Game *game, int provinceID)
		{
			std::unique_ptr<Panel> provincePanel(new ProvinceMapPanel(game, provinceID));
			game->setPanel(std::move(provincePanel));
		};
		return std::unique_ptr<Button<int>>(new Button<int>(function));
	}();
}

WorldMapPanel::~WorldMapPanel()
{

}

void WorldMapPanel::handleEvent(const SDL_Event &e)
{
	const Int2 mousePosition = this->getMousePosition();
	const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
		.nativePointToOriginal(mousePosition);

	bool escapePressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_ESCAPE);
	bool mPressed = (e.type == SDL_KEYDOWN) && (e.key.keysym.sym == SDLK_m);

	if (escapePressed || mPressed)
	{
		this->backToGameButton->click(this->getGame());
	}

	bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_LEFT);

	if (leftClick)
	{
		if (this->backToGameButton->contains(mouseOriginalPoint))
		{
			this->backToGameButton->click(this->getGame());
		}

		// Listen for map clicks.
		const int provinceCount = static_cast<int>(ProvinceClickAreas.size());
		for (int provinceID = 0; provinceID < provinceCount; provinceID++)
		{
			const Rect &clickArea = ProvinceClickAreas.at(provinceID);

			if (clickArea.contains(mouseOriginalPoint))
			{
				// Go to the province panel.
				this->provinceButton->click(this->getGame(), provinceID);
				break;
			}
		}
	}	
}

void WorldMapPanel::render(Renderer &renderer)
{
	assert(this->getGame()->gameDataIsActive());

	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGame()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw world map background. This one has "Exit" at the bottom right.
	const auto &mapBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::WorldMap), 
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawToOriginal(mapBackground.get());

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();

	// Draw cursor.
	const auto &cursor = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor));
	auto mousePosition = this->getMousePosition();
	renderer.drawToNative(cursor.get(),
		mousePosition.x, mousePosition.y,
		static_cast<int>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int>(cursor.getHeight() * this->getCursorScale()));
}
