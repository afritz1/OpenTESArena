#include <algorithm>
#include <cassert>
#include <limits>
#include <unordered_map>

#include "SDL.h"

#include "CursorAlignment.h"
#include "ProvinceButtonName.h"
#include "ProvinceMapPanel.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "WorldMapPanel.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/ExeStrings.h"
#include "../Assets/TextAssets.h"
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
#include "../Utilities/String.h"

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

std::pair<SDL_Texture*, CursorAlignment> ProvinceMapPanel::getCurrentCursor() const
{
	auto &textureManager = this->getGame()->getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default));
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
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

	Texture tooltip(Panel::createTooltip(
		text, FontName::D, this->getGame()->getFontManager(), renderer));

	const auto &inputManager = this->getGame()->getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativePointToOriginal(mousePosition);
	const int mouseX = originalPosition.x;
	const int mouseY = originalPosition.y;
	const int x = ((mouseX + 8 + tooltip.getWidth()) < Renderer::ORIGINAL_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < Renderer::ORIGINAL_HEIGHT) ?
		mouseY : (mouseY - tooltip.getHeight());

	renderer.drawOriginal(tooltip.get(), x, y);
}

void ProvinceMapPanel::drawLocationName(const std::string &name, const Int2 &center,
	Renderer &renderer)
{
	const RichTextString richText(
		name,
		FontName::Arena,
		Color(158, 0, 0),
		TextAlignment::Center,
		this->getGame()->getFontManager());

	const TextBox textBox(
		center - Int2(0, 10), richText, Color(48, 48, 48), renderer);

	// Clamp to screen edges, with some extra space on the left and right.
	const int x = std::max(std::min(textBox.getX(),
		Renderer::ORIGINAL_WIDTH - textBox.getSurface()->w - 2), 2);
	const int y = std::max(std::min(textBox.getY(),
		Renderer::ORIGINAL_HEIGHT - textBox.getSurface()->h), 0);
	
	renderer.drawOriginal(textBox.getShadowTexture(), x + 1, y);
	renderer.drawOriginal(textBox.getTexture(), x, y);
}

void ProvinceMapPanel::render(Renderer &renderer)
{
	assert(this->getGame()->gameDataIsActive());

	// Clear full screen.
	renderer.clear();

	// Set palette.
	auto &textureManager = this->getGame()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Get the filename of the province map.
	const std::string backgroundFilename = [this]()
	{
		const std::string &filename = this->getGame()->getTextAssets().getAExeStrings().getList(
			ExeStringKey::ProvinceIMGFilenames).at(this->provinceID);

		// Set all characters to uppercase because the texture manager expects 
		// extensions to be uppercase, and most filenames in A.EXE are lowercase.
		return String::toUppercase(filename);
	}();

	// Draw province map background.
	const auto &mapBackground = textureManager.getTexture(
		backgroundFilename, PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawOriginal(mapBackground.get());

	// Draw location icons, and find which place is closest to the mouse cursor.
	// Ignore locations with no name (ones that are zeroed out in the center province).
	const auto &inputManager = this->getGame()->getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = this->getGame()->getRenderer()
		.nativePointToOriginal(mousePosition);

	// Initialize the current closest position to something very far away (watch out for
	// integer multiplication overflow with closest distance).
	Int2 closestPosition(-1000, -1000);

	// Lambda for comparing distances of two location points to the mouse position.
	auto closerThanCurrentClosest = [&originalPosition, &closestPosition](const Int2 &point)
	{
		const int diffX = point.x - originalPosition.x;
		const int diffY = point.y - originalPosition.y;
		const int closestDiffX = closestPosition.x - originalPosition.x;
		const int closestDiffY = closestPosition.y - originalPosition.y;

		const double distance = std::sqrt((diffX * diffX) + (diffY * diffY));
		const double closestDistance = std::sqrt(
			(closestDiffX * closestDiffX) + (closestDiffY * closestDiffY));

		return distance < closestDistance;
	};

	std::string closestName;

	// Lambda for drawing a location icon and refreshing the closest location data.
	auto drawIcon = [&closestPosition, &closestName, &closerThanCurrentClosest, &renderer](
		const CityDataFile::ProvinceData::LocationData &location, const Texture &icon)
	{
		// Only check locations with names.
		if (location.name.front() != '\0')
		{
			const Int2 point(location.x, location.y);

			renderer.drawOriginal(icon.get(),
				point.x - (icon.getWidth() / 2),
				point.y - (icon.getHeight() / 2));

			if (closerThanCurrentClosest(point))
			{
				closestPosition = point;
				closestName = std::string(location.name.data());
			}
		}
	};

	const auto &cityStateIcon = textureManager.getTexture(
		TextureFile::fromName(TextureName::CityStateIcon), backgroundFilename);
	const auto &townIcon = textureManager.getTexture(
		TextureFile::fromName(TextureName::TownIcon), backgroundFilename);
	const auto &villageIcon = textureManager.getTexture(
		TextureFile::fromName(TextureName::VillageIcon), backgroundFilename);

	const auto &province = this->getGame()->getCityDataFile()
		.getProvinceData(this->provinceID);

	// Draw city-state icons.
	for (const auto &cityState : province.cityStates)
	{
		drawIcon(cityState, cityStateIcon);
	}

	// Draw town icons.
	for (const auto &town : province.towns)
	{
		drawIcon(town, townIcon);
	}

	// Draw village icons.
	for (const auto &village : province.villages)
	{
		drawIcon(village, villageIcon);
	}

	// Draw the name of the location closest to the mouse cursor.
	this->drawLocationName(closestName, closestPosition, renderer);

	// Draw a tooltip if the mouse is over a button.
	for (const auto &pair : ProvinceButtonTooltips)
	{
		const Rect &clickArea = ProvinceButtonClickAreas.at(pair.first);

		if (clickArea.contains(originalPosition))
		{
			this->drawButtonTooltip(pair.first, renderer);
		}
	}
}
