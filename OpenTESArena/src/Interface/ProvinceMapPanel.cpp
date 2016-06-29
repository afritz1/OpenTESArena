#include <cassert>
#include <map>

#include "SDL.h"

#include "ProvinceMapPanel.h"

#include "Button.h"
#include "ProvinceButtonName.h"
#include "TextBox.h"
#include "WorldMapPanel.h"
#include "../Game/GameState.h"
#include "../Math/Constants.h"
#include "../Math/Int2.h"
#include "../Math/Rect.h"
#include "../Media/FontName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../World/Province.h"
#include "../World/ProvinceName.h"

namespace
{
	const std::map<ProvinceButtonName, std::string> ProvinceButtonTooltips =
	{
		{ ProvinceButtonName::Search, "Search (not implemented)" },
		{ ProvinceButtonName::Travel, "Travel (not implemented)" },
		{ ProvinceButtonName::BackToWorldMap, "Back to World Map" }
	};

	const std::map<ProvinceButtonName, Rect> ProvinceButtonClickAreas =
	{
		{ ProvinceButtonName::Search, Rect(34, ORIGINAL_HEIGHT - 32, 18, 27) },
		{ ProvinceButtonName::Travel, Rect(53, ORIGINAL_HEIGHT - 32, 18, 27) },
		{ ProvinceButtonName::BackToWorldMap, Rect(72, ORIGINAL_HEIGHT - 32, 18, 27) }
	};

	const std::map<ProvinceName, TextureName> ProvinceMapTextureNames =
	{
		{ ProvinceName::BlackMarsh, TextureName::BlackMarshMap },
		{ ProvinceName::Elsweyr, TextureName::ElsweyrMap },
		{ ProvinceName::Hammerfell, TextureName::HammerfellMap },
		{ ProvinceName::HighRock, TextureName::HighRockMap },
		{ ProvinceName::ImperialProvince, TextureName::ImperialProvinceMap },
		{ ProvinceName::Morrowind, TextureName::MorrowindMap },
		{ ProvinceName::Skyrim, TextureName::SkyrimMap },
		{ ProvinceName::SummersetIsle, TextureName::SummersetIsleMap },
		{ ProvinceName::Valenwood, TextureName::ValenwoodMap }
	};
}

ProvinceMapPanel::ProvinceMapPanel(GameState *gameState, const Province &province)
	: Panel(gameState)
{
	this->searchButton = [gameState]()
	{
		const auto &clickArea = ProvinceButtonClickAreas.at(ProvinceButtonName::Search);
		int x = clickArea.getLeft();
		int y = clickArea.getTop();
		int width = clickArea.getWidth();
		int height = clickArea.getHeight();
		auto function = [gameState]()
		{
			// Nothing yet.
		};
		return std::unique_ptr<Button>(new Button(x, y, width, height, function));
	}();

	this->travelButton = [gameState]()
	{
		const auto &clickArea = ProvinceButtonClickAreas.at(ProvinceButtonName::Travel);
		int x = clickArea.getLeft();
		int y = clickArea.getTop();
		int width = clickArea.getWidth();
		int height = clickArea.getHeight();
		auto function = [gameState]()
		{
			// Nothing yet.
		};
		return std::unique_ptr<Button>(new Button(x, y, width, height, function));
	}();

	this->backToWorldMapButton = [gameState]()
	{
		const auto &clickArea = ProvinceButtonClickAreas.at(ProvinceButtonName::BackToWorldMap);
		int x = clickArea.getLeft();
		int y = clickArea.getTop();
		int width = clickArea.getWidth();
		int height = clickArea.getHeight();
		auto function = [gameState]()
		{
			std::unique_ptr<Panel> gamePanel(new WorldMapPanel(gameState));
			gameState->setPanel(std::move(gamePanel));
		};
		return std::unique_ptr<Button>(new Button(x, y, width, height, function));
	}();

	this->province = std::unique_ptr<Province>(new Province(province));
}

ProvinceMapPanel::~ProvinceMapPanel()
{

}

void ProvinceMapPanel::handleEvents(bool &running)
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
		if (escapePressed)
		{
			this->backToWorldMapButton->click();
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);
		bool rightClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_RIGHT);

		// Input will eventually depend on if the location pop-up is displayed, or
		// if a location is selected.
		if (rightClick)
		{
			this->backToWorldMapButton->click();
		}
		else if (leftClick)
		{
			if (this->searchButton->containsPoint(mouseOriginalPoint))
			{
				this->searchButton->click();
			}

			else if (this->travelButton->containsPoint(mouseOriginalPoint))
			{
				this->travelButton->click();
			}

			else if (this->backToWorldMapButton->containsPoint(mouseOriginalPoint))
			{
				this->backToWorldMapButton->click();
			}

			// Check locations for clicks...
		}
	}
}

void ProvinceMapPanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void ProvinceMapPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void ProvinceMapPanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
}

void ProvinceMapPanel::drawButtonTooltip(ProvinceButtonName buttonName, 
	SDL_Renderer *renderer)
{
	auto mouseOriginalPosition = this->nativePointToOriginal(this->getMousePosition());
	std::unique_ptr<TextBox> tooltip(new TextBox(
		mouseOriginalPosition.getX(),
		mouseOriginalPosition.getY(),
		Color::White,
		ProvinceButtonTooltips.at(buttonName),
		FontName::A,
		this->getGameState()->getTextureManager()));
	Surface tooltipBackground(tooltip->getX(), tooltip->getY(),
		tooltip->getWidth(), tooltip->getHeight());
	tooltipBackground.fill(Color(32, 32, 32));

	const int tooltipX = tooltip->getX();
	const int tooltipY = tooltip->getY();
	const int width = tooltip->getWidth() / 2;
	const int height = tooltip->getHeight() / 2;
	const int x = ((tooltipX + width) < ORIGINAL_WIDTH) ? tooltipX : (tooltipX - width);
	const int y = ((tooltipY + height) < ORIGINAL_HEIGHT) ? tooltipY : (tooltipY - height);

	this->drawScaledToNative(tooltipBackground, x, y - 1, width, height + 2, renderer);
	this->drawScaledToNative(*tooltip.get(), x, y, width, height, renderer);
}

void ProvinceMapPanel::render(SDL_Renderer *renderer, const SDL_Rect *letterbox)
{
	assert(this->getGameState()->gameDataIsActive());

	// Clear full screen.
	this->clearScreen(renderer);

	// Get the texture name for this province.
	auto provinceTextureName = ProvinceMapTextureNames.at(
		this->province->getProvinceName());

	// Draw province map background.
	const auto *mapBackground = this->getGameState()->getTextureManager()
		.getTexture(TextureFile::fromName(provinceTextureName));
	this->drawScaledToNative(mapBackground, renderer);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, renderer);

	// Draw tooltip if the mouse is over a button.
	auto mouseOriginalPosition = this->nativePointToOriginal(this->getMousePosition());
	for (const auto &pair : ProvinceButtonTooltips)
	{
		const Rect &clickArea = ProvinceButtonClickAreas.at(pair.first);

		if (clickArea.contains(mouseOriginalPosition))
		{
			this->drawButtonTooltip(pair.first, renderer);
		}
	}
}
