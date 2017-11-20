#include <cassert>

#include "SDL.h"

#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "ProvinceMapPanel.h"
#include "TextBox.h"
#include "WorldMapPanel.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
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

WorldMapPanel::WorldMapPanel(Game &game)
	: Panel(game)
{
	this->backToGameButton = []()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH - 22, Renderer::ORIGINAL_HEIGHT - 7);
		int width = 36;
		int height = 9;
		auto function = [](Game &game)
		{
			std::unique_ptr<Panel> gamePanel(new GameWorldPanel(game));
			game.setPanel(std::move(gamePanel));
		};
		return Button<Game&>(center, width, height, function);
	}();

	this->provinceButton = []()
	{
		auto function = [](Game &game, int provinceID)
		{
			std::unique_ptr<Panel> provincePanel(new ProvinceMapPanel(game, provinceID));
			game.setPanel(std::move(provincePanel));
		};
		return Button<Game&, int>(function);
	}();
}

WorldMapPanel::~WorldMapPanel()
{

}

std::pair<SDL_Texture*, CursorAlignment> WorldMapPanel::getCurrentCursor() const
{
	auto &textureManager = this->getGame().getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default));
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
}

void WorldMapPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool mPressed = inputManager.keyPressed(e, SDLK_m);

	if (escapePressed || mPressed)
	{
		this->backToGameButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame().getRenderer()
			.nativePointToOriginal(mousePosition);

		if (this->backToGameButton.contains(mouseOriginalPoint))
		{
			this->backToGameButton.click(this->getGame());
		}

		// Listen for map clicks.
		const int provinceCount = static_cast<int>(ProvinceClickAreas.size());
		for (int provinceID = 0; provinceID < provinceCount; provinceID++)
		{
			const Rect &clickArea = ProvinceClickAreas.at(provinceID);

			if (clickArea.contains(mouseOriginalPoint))
			{
				// Go to the province panel.
				this->provinceButton.click(this->getGame(), provinceID);
				break;
			}
		}
	}	
}

void WorldMapPanel::render(Renderer &renderer)
{
	assert(this->getGame().gameDataIsActive());

	// Clear full screen.
	renderer.clear();

	// Set palette.
	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw world map background. This one has "Exit" at the bottom right.
	const auto &mapBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::WorldMap), 
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawOriginal(mapBackground.get());
}
