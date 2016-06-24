#include <cassert>

#include "SDL.h"

#include "WorldMapPanel.h"

#include "Button.h"
#include "GameWorldPanel.h"
#include "ProvinceMapPanel.h"
#include "TextBox.h"
#include "../Game/GameState.h"
#include "../Math/Constants.h"
#include "../Math/Int2.h"
#include "../Math/Rect.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../World/Province.h"
#include "../World/ProvinceName.h"

WorldMapPanel::WorldMapPanel(GameState *gameState)
	: Panel(gameState)
{
	this->backToGameButton = [gameState]()
	{
		auto center = Int2(ORIGINAL_WIDTH - 22, ORIGINAL_HEIGHT - 7);
		int width = 36;
		int height = 9;
		auto function = [gameState]()
		{
			auto gamePanel = std::unique_ptr<Panel>(new GameWorldPanel(gameState));
			gameState->setPanel(std::move(gamePanel));
		};
		return std::unique_ptr<Button>(new Button(center, width, height, function));
	}();

	this->provinceButton = [this, gameState]()
	{
		auto function = [this, gameState]()
		{
			auto provincePanel = std::unique_ptr<Panel>(new ProvinceMapPanel(
				gameState, Province(*this->provinceName.get())));
			gameState->setPanel(std::move(provincePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->provinceName = nullptr;
}

WorldMapPanel::~WorldMapPanel()
{

}

void WorldMapPanel::handleEvents(bool &running)
{
	auto mousePosition = this->getMousePosition();
	auto mouseOriginalPoint = this->nativePointToOriginal(mousePosition);

	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		bool applicationExit = (e.type == SDL_QUIT);
		bool resized = (e.type == SDL_WINDOWEVENT) &&
			(e.window.event == SDL_WINDOWEVENT_RESIZED);
		bool escapePressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_ESCAPE);
		bool mPressed = (e.type == SDL_KEYDOWN) && (e.key.keysym.sym == SDLK_m);

		if (applicationExit)
		{
			running = false;
		}
		if (resized)
		{
			int width = e.window.data1;
			int height = e.window.data2;
			this->getGameState()->resizeWindow(width, height);
		}
		if (escapePressed || mPressed)
		{
			this->backToGameButton->click();
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);

		if (leftClick)
		{
			if (this->backToGameButton->containsPoint(mouseOriginalPoint))
			{
				this->backToGameButton->click();
			}

			// Listen for map clicks.
			for (const auto &provinceName : Province::getAllProvinceNames())
			{
				Province province(provinceName);
				const Rect &clickArea = province.getWorldMapClickArea();

				if (clickArea.contains(mouseOriginalPoint))
				{
					// Save the clicked province's name.
					this->provinceName = std::unique_ptr<ProvinceName>(new ProvinceName(
						provinceName));

					// Go to the province panel.
					this->provinceButton->click();
					break;
				}
			}
		}
	}
}

void WorldMapPanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void WorldMapPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void WorldMapPanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
}

void WorldMapPanel::render(SDL_Renderer *renderer, const SDL_Rect *letterbox)
{
	assert(this->getGameState()->gameDataIsActive());

	// Clear full screen.
	this->clearScreen(renderer);

	// Draw world map background. This one has "Exit" at the bottom right.
	const auto *mapBackground = this->getGameState()->getTextureManager()
		.getTexture(TextureFile::fromName(TextureName::WorldMap));
	this->drawScaledToNative(mapBackground, renderer);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, renderer);
}
