#include <cassert>
#include <map>

#include "SDL.h"

#include "ProvinceMapPanel.h"

#include "Button.h"
#include "ProvinceButtonName.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "WorldMapPanel.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Math/Rect.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"
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
		{ ProvinceButtonName::Search, Rect(34, Renderer::ORIGINAL_HEIGHT - 32, 18, 27) },
		{ ProvinceButtonName::Travel, Rect(53, Renderer::ORIGINAL_HEIGHT - 32, 18, 27) },
		{ ProvinceButtonName::BackToWorldMap, Rect(72, Renderer::ORIGINAL_HEIGHT - 32, 18, 27) }
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
	this->searchButton = []()
	{
		const auto &clickArea = ProvinceButtonClickAreas.at(ProvinceButtonName::Search);
		int x = clickArea.getLeft();
		int y = clickArea.getTop();
		int width = clickArea.getWidth();
		int height = clickArea.getHeight();
		auto function = [](GameState *gameState)
		{
			// Nothing yet.
		};
		return std::unique_ptr<Button>(new Button(x, y, width, height, function));
	}();

	this->travelButton = []()
	{
		const auto &clickArea = ProvinceButtonClickAreas.at(ProvinceButtonName::Travel);
		int x = clickArea.getLeft();
		int y = clickArea.getTop();
		int width = clickArea.getWidth();
		int height = clickArea.getHeight();
		auto function = [](GameState *gameState)
		{
			// Nothing yet.
		};
		return std::unique_ptr<Button>(new Button(x, y, width, height, function));
	}();

	this->backToWorldMapButton = []()
	{
		const auto &clickArea = ProvinceButtonClickAreas.at(ProvinceButtonName::BackToWorldMap);
		int x = clickArea.getLeft();
		int y = clickArea.getTop();
		int width = clickArea.getWidth();
		int height = clickArea.getHeight();
		auto function = [](GameState *gameState)
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
	auto mouseOriginalPoint = this->getGameState()->getRenderer()
		.nativePointToOriginal(mousePosition);

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
			this->backToWorldMapButton->click(this->getGameState());
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);
		bool rightClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_RIGHT);

		// Input will eventually depend on if the location pop-up is displayed, or
		// if a location is selected.
		if (rightClick)
		{
			this->backToWorldMapButton->click(this->getGameState());
		}
		else if (leftClick)
		{
			if (this->searchButton->containsPoint(mouseOriginalPoint))
			{
				this->searchButton->click(this->getGameState());
			}

			else if (this->travelButton->containsPoint(mouseOriginalPoint))
			{
				this->travelButton->click(this->getGameState());
			}

			else if (this->backToWorldMapButton->containsPoint(mouseOriginalPoint))
			{
				this->backToWorldMapButton->click(this->getGameState());
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

void ProvinceMapPanel::drawButtonTooltip(ProvinceButtonName buttonName, Renderer &renderer)
{
	auto mouseOriginalPosition = this->getGameState()->getRenderer()
		.nativePointToOriginal(this->getMousePosition());

	std::unique_ptr<TextBox> tooltip(new TextBox(
		mouseOriginalPosition.getX(),
		mouseOriginalPosition.getY(),
		Color::White,
		ProvinceButtonTooltips.at(buttonName),
		this->getGameState()->getFontManager().getFont(FontName::D),
		TextAlignment::Left,
		this->getGameState()->getRenderer()));

	int tooltipWidth, tooltipHeight;
	SDL_QueryTexture(tooltip->getTexture(), nullptr, nullptr, 
		&tooltipWidth, &tooltipHeight);

	Surface tooltipBackground(tooltip->getX(), tooltip->getY(),
		tooltipWidth, tooltipHeight);
	tooltipBackground.fill(Color(32, 32, 32));

	const int tooltipX = tooltip->getX();
	const int tooltipY = tooltip->getY();
	const int x = ((tooltipX + 8 + tooltipWidth) < Renderer::ORIGINAL_WIDTH) ?
		(tooltipX + 8) : (tooltipX - tooltipWidth);
	const int y = ((tooltipY + tooltipHeight) < Renderer::ORIGINAL_HEIGHT) ?
		tooltipY : (tooltipY - tooltipHeight);

	renderer.drawToOriginal(tooltipBackground.getSurface(), x, y - 1, 
		tooltipWidth, tooltipHeight + 2);
	renderer.drawToOriginal(tooltip->getTexture(), x, y, tooltipWidth, tooltipHeight);
}

void ProvinceMapPanel::render(Renderer &renderer)
{
	assert(this->getGameState()->gameDataIsActive());

	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGameState()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Get the texture name for this province.
	auto provinceTextureName = ProvinceMapTextureNames.at(
		this->province->getProvinceName());	

	// Draw province map background.
	const auto &mapBackground = textureManager.getTexture(
		TextureFile::fromName(provinceTextureName), 
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawToOriginal(mapBackground.get());

	// Draw tooltip if the mouse is over a button.
	auto mouseOriginalPosition = this->getGameState()->getRenderer()
		.nativePointToOriginal(this->getMousePosition());

	for (const auto &pair : ProvinceButtonTooltips)
	{
		const Rect &clickArea = ProvinceButtonClickAreas.at(pair.first);

		if (clickArea.contains(mouseOriginalPosition))
		{
			this->drawButtonTooltip(pair.first, renderer);
		}
	}

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
