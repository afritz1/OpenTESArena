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
#include "../Rendering/Surface.h"
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

void ProvinceMapPanel::handleEvent(const SDL_Event &e)
{
	// Input will eventually depend on if the location pop-up is displayed, or
	// if a location is selected.
	bool escapePressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_ESCAPE);
	bool rightClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_RIGHT);

	if (escapePressed || rightClick)
	{
		this->backToWorldMapButton->click(this->getGameState());
	}

	bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = this->getMousePosition();
		const Int2 mouseOriginalPoint = this->getGameState()->getRenderer()
			.nativePointToOriginal(mousePosition);

		if (this->searchButton->contains(mouseOriginalPoint))
		{
			this->searchButton->click(this->getGameState());
		}
		else if (this->travelButton->contains(mouseOriginalPoint))
		{
			this->travelButton->click(this->getGameState());
		}
		else if (this->backToWorldMapButton->contains(mouseOriginalPoint))
		{
			this->backToWorldMapButton->click(this->getGameState());
		}

		// Check locations for clicks...
	}
}

void ProvinceMapPanel::tick(double dt)
{
	// Eventually blink the selected location.
	static_cast<void>(dt);
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

	Texture tooltipBackground = [&renderer, &tooltip, tooltipWidth, tooltipHeight]()
	{
		SDL_Surface *temp = Surface::createSurfaceWithFormat(
			tooltipWidth, tooltipHeight, Renderer::DEFAULT_BPP,
			Renderer::DEFAULT_PIXELFORMAT);
		SDL_FillRect(temp, nullptr, SDL_MapRGBA(temp->format, 32, 32, 32, 192));
		SDL_Texture *background = renderer.createTextureFromSurface(temp);
		SDL_SetTextureBlendMode(background, SDL_BLENDMODE_BLEND);
		SDL_FreeSurface(temp);
		return Texture(background);
	}();

	const int tooltipX = tooltip->getX();
	const int tooltipY = tooltip->getY();
	const int x = ((tooltipX + 8 + tooltipWidth) < Renderer::ORIGINAL_WIDTH) ?
		(tooltipX + 8) : (tooltipX - tooltipWidth);
	const int y = ((tooltipY + tooltipHeight) < Renderer::ORIGINAL_HEIGHT) ?
		tooltipY : (tooltipY - tooltipHeight);

	renderer.drawToOriginal(tooltipBackground.get(), x, y - 1, 
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
