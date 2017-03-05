#include <cassert>
#include <unordered_map>

#include "SDL.h"

#include "ProvinceMapPanel.h"

#include "Button.h"
#include "ProvinceButtonName.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "WorldMapPanel.h"
#include "../Game/Game.h"
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
#include "../World/ProvinceName.h"

namespace std
{
	// Hash specializations, since GCC doesn't support enum classes used as keys
	// in unordered_maps before C++14.
	template <>
	struct hash<ProvinceButtonName>
	{
		size_t operator()(const ProvinceButtonName &x) const
		{
			return static_cast<size_t>(x);
		}
	};

	template <>
	struct hash<ProvinceName>
	{
		size_t operator()(const ProvinceName &x) const
		{
			return static_cast<size_t>(x);
		}
	};
}

namespace
{
	const std::unordered_map<ProvinceButtonName, std::string> ProvinceButtonTooltips =
	{
		{ ProvinceButtonName::Search, "Search (not implemented)" },
		{ ProvinceButtonName::Travel, "Travel (not implemented)" },
		{ ProvinceButtonName::BackToWorldMap, "Back to World Map" }
	};

	const std::unordered_map<ProvinceButtonName, Rect> ProvinceButtonClickAreas =
	{
		{ ProvinceButtonName::Search, Rect(34, Renderer::ORIGINAL_HEIGHT - 32, 18, 27) },
		{ ProvinceButtonName::Travel, Rect(53, Renderer::ORIGINAL_HEIGHT - 32, 18, 27) },
		{ ProvinceButtonName::BackToWorldMap, Rect(72, Renderer::ORIGINAL_HEIGHT - 32, 18, 27) }
	};

	const std::unordered_map<ProvinceName, TextureName> ProvinceMapTextureNames =
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

ProvinceMapPanel::ProvinceMapPanel(Game *game, const Province &province)
	: Panel(game), province(province)
{
	this->searchButton = []()
	{
		const auto &clickArea = ProvinceButtonClickAreas.at(ProvinceButtonName::Search);
		int x = clickArea.getLeft();
		int y = clickArea.getTop();
		int width = clickArea.getWidth();
		int height = clickArea.getHeight();
		auto function = [](Game *game)
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
		auto function = [](Game *game)
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
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> gamePanel(new WorldMapPanel(game));
			game->setPanel(std::move(gamePanel));
		};
		return std::unique_ptr<Button>(new Button(x, y, width, height, function));
	}();
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
		this->backToWorldMapButton->click(this->getGame());
	}

	bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = this->getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
			.nativePointToOriginal(mousePosition);

		if (this->searchButton->contains(mouseOriginalPoint))
		{
			this->searchButton->click(this->getGame());
		}
		else if (this->travelButton->contains(mouseOriginalPoint))
		{
			this->travelButton->click(this->getGame());
		}
		else if (this->backToWorldMapButton->contains(mouseOriginalPoint))
		{
			this->backToWorldMapButton->click(this->getGame());
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
	const std::string &text = ProvinceButtonTooltips.at(buttonName);
	const Font &font = this->getGame()->getFontManager().getFont(FontName::D);

	Texture tooltip(Panel::createTooltip(text, font, renderer));

	const Int2 mousePosition = this->getMousePosition();
	const Int2 originalPosition = renderer.nativePointToOriginal(mousePosition);
	const int mouseX = originalPosition.x;
	const int mouseY = originalPosition.y;
	const int x = ((mouseX + 8 + tooltip.getWidth()) < Renderer::ORIGINAL_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < Renderer::ORIGINAL_HEIGHT) ?
		mouseY : (mouseY - tooltip.getHeight());

	renderer.drawToOriginal(tooltip.get(), x, y);
}

void ProvinceMapPanel::render(Renderer &renderer)
{
	assert(this->getGame()->gameDataIsActive());

	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGame()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Get the texture name for this province.
	auto provinceTextureName = ProvinceMapTextureNames.at(
		this->province.getProvinceName());

	// Draw province map background.
	const auto &mapBackground = textureManager.getTexture(
		TextureFile::fromName(provinceTextureName), 
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawToOriginal(mapBackground.get());

	// Draw tooltip if the mouse is over a button.
	auto mouseOriginalPosition = this->getGame()->getRenderer()
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
		mousePosition.x, mousePosition.y,
		static_cast<int>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int>(cursor.getHeight() * this->getCursorScale()));
}
