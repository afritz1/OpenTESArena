#include <cassert>

#include "SDL.h"

#include "WorldMapPanel.h"

#include "Button.h"
#include "GameWorldPanel.h"
#include "ProvinceMapPanel.h"
#include "TextBox.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Math/Rect.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"
#include "../World/Province.h"
#include "../World/ProvinceName.h"

WorldMapPanel::WorldMapPanel(GameState *gameState)
	: Panel(gameState)
{
	this->backToGameButton = []()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH - 22, Renderer::ORIGINAL_HEIGHT - 7);
		int width = 36;
		int height = 9;
		auto function = [](GameState *gameState)
		{
			std::unique_ptr<Panel> gamePanel(new GameWorldPanel(gameState));
			gameState->setPanel(std::move(gamePanel));
		};
		return std::unique_ptr<Button>(new Button(center, width, height, function));
	}();

	this->provinceButton = [this]()
	{
		auto function = [this](GameState *gameState)
		{
			std::unique_ptr<Panel> provincePanel(new ProvinceMapPanel(
				gameState, Province(*this->provinceName.get())));
			gameState->setPanel(std::move(provincePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	// Leave province name null until one is selected.
	this->provinceName = nullptr;
}

WorldMapPanel::~WorldMapPanel()
{

}

void WorldMapPanel::handleEvent(const SDL_Event &e)
{
	const Int2 mousePosition = this->getMousePosition();
	const Int2 mouseOriginalPoint = this->getGameState()->getRenderer()
		.nativePointToOriginal(mousePosition);

	bool escapePressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_ESCAPE);
	bool mPressed = (e.type == SDL_KEYDOWN) && (e.key.keysym.sym == SDLK_m);

	if (escapePressed || mPressed)
	{
		this->backToGameButton->click(this->getGameState());
	}

	bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_LEFT);

	if (leftClick)
	{
		if (this->backToGameButton->contains(mouseOriginalPoint))
		{
			this->backToGameButton->click(this->getGameState());
		}

		// Listen for map clicks.
		for (const auto provinceName : Province::getAllProvinceNames())
		{
			Province province(provinceName);
			const Rect &clickArea = province.getWorldMapClickArea();

			if (clickArea.contains(mouseOriginalPoint))
			{
				// Save the clicked province's name.
				this->provinceName = std::unique_ptr<ProvinceName>(new ProvinceName(
					provinceName));

				// Go to the province panel.
				this->provinceButton->click(this->getGameState());
				break;
			}
		}
	}	
}

void WorldMapPanel::render(Renderer &renderer)
{
	assert(this->getGameState()->gameDataIsActive());

	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGameState()->getTextureManager();
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
		mousePosition.getX(), mousePosition.getY(),
		static_cast<int>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int>(cursor.getHeight() * this->getCursorScale()));
}
