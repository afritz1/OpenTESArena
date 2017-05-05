#include <cassert>
#include <unordered_map>

#include "SDL.h"

#include "ProvinceMapPanel.h"

#include "ProvinceButtonName.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "WorldMapPanel.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
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
}

ProvinceMapPanel::ProvinceMapPanel(Game *game, int provinceID)
	: Panel(game)
{
	this->searchButton = []()
	{
		const Rect &clickArea = ProvinceButtonClickAreas.at(ProvinceButtonName::Search);
		int x = clickArea.getLeft();
		int y = clickArea.getTop();
		int width = clickArea.getWidth();
		int height = clickArea.getHeight();
		auto function = []()
		{
			// Nothing yet.
		};
		return std::unique_ptr<Button<>>(new Button<>(x, y, width, height, function));
	}();

	this->travelButton = []()
	{
		const Rect &clickArea = ProvinceButtonClickAreas.at(ProvinceButtonName::Travel);
		int x = clickArea.getLeft();
		int y = clickArea.getTop();
		int width = clickArea.getWidth();
		int height = clickArea.getHeight();
		auto function = []()
		{
			// Nothing yet.
		};
		return std::unique_ptr<Button<>>(new Button<>(x, y, width, height, function));
	}();

	this->backToWorldMapButton = []()
	{
		const Rect &clickArea = ProvinceButtonClickAreas.at(ProvinceButtonName::BackToWorldMap);
		int x = clickArea.getLeft();
		int y = clickArea.getTop();
		int width = clickArea.getWidth();
		int height = clickArea.getHeight();
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> gamePanel(new WorldMapPanel(game));
			game->setPanel(std::move(gamePanel));
		};
		return std::unique_ptr<Button<Game*>>(
			new Button<Game*>(x, y, width, height, function));
	}();

	this->provinceID = provinceID;
}

ProvinceMapPanel::~ProvinceMapPanel()
{

}

void ProvinceMapPanel::handleEvent(const SDL_Event &e)
{
	// Input will eventually depend on if the location pop-up is displayed, or
	// if a location is selected.
	const auto &inputManager = this->getGame()->getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	if (escapePressed || rightClick)
	{
		this->backToWorldMapButton->click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
			.nativePointToOriginal(mousePosition);

		if (this->searchButton->contains(mouseOriginalPoint))
		{
			this->searchButton->click();
		}
		else if (this->travelButton->contains(mouseOriginalPoint))
		{
			this->travelButton->click();
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

	const auto &inputManager = this->getGame()->getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
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
	
	// Draw province map background.
	const auto &mapBackground = textureManager.getTexture(
		TextureFile::provinceMapFromID(this->provinceID), 
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawToOriginal(mapBackground.get());

	// Draw tooltip if the mouse is over a button.
	const auto &inputManager = this->getGame()->getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	auto mouseOriginalPosition = this->getGame()->getRenderer()
		.nativePointToOriginal(mousePosition);

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
	const auto &options = this->getGame()->getOptions();
	renderer.drawToNative(cursor.get(),
		mousePosition.x, mousePosition.y,
		static_cast<int>(cursor.getWidth() * options.getCursorScale()),
		static_cast<int>(cursor.getHeight() * options.getCursorScale()));
}
