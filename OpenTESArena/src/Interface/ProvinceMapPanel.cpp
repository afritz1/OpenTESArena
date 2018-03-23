#include <algorithm>
#include <cassert>
#include <cmath>
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
#include "../Assets/ExeData.h"
#include "../Assets/IMGFile.h"
#include "../Assets/MiscAssets.h"
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
#include "../World/Location.h"
#include "../World/LocationDataType.h"

namespace std
{
	// Hash specialization, required until GCC 6.1.
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
	// .CIF palette indices for staff dungeon outlines.
	const uint8_t BackgroundPaletteIndex = 220;
	const uint8_t YellowPaletteIndex = 194;
	const uint8_t RedPaletteIndex = 223;

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

ProvinceMapPanel::ProvinceMapPanel(Game &game, int provinceID)
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
		return Button<>(x, y, width, height, function);
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
		return Button<>(x, y, width, height, function);
	}();

	this->backToWorldMapButton = []()
	{
		const Rect &clickArea = ProvinceButtonClickAreas.at(ProvinceButtonName::BackToWorldMap);
		int x = clickArea.getLeft();
		int y = clickArea.getTop();
		int width = clickArea.getWidth();
		int height = clickArea.getHeight();
		auto function = [](Game &game)
		{
			game.setPanel<WorldMapPanel>(game);
		};
		return Button<Game&>(x, y, width, height, function);
	}();

	this->provinceID = provinceID;

	// Get the palette for the background image.
	IMGFile::extractPalette(this->getBackgroundFilename(), this->provinceMapPalette);

	// If displaying a province that contains a staff dungeon, get the staff dungeon icon's
	// raw palette indices (for yellow and red color swapping).
	if (provinceID != 8)
	{
		const std::string &cifName = TextureFile::fromName(TextureName::StaffDungeonIcons);
		this->staffDungeonCif = std::make_unique<CIFFile>(cifName, this->provinceMapPalette);
	}
}

std::pair<SDL_Texture*, CursorAlignment> ProvinceMapPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default), renderer);
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
}

void ProvinceMapPanel::handleEvent(const SDL_Event &e)
{
	// Input will eventually depend on if the location pop-up is displayed, or
	// if a location is selected.
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	if (escapePressed || rightClick)
	{
		this->backToWorldMapButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		if (this->searchButton.contains(mouseOriginalPoint))
		{
			this->searchButton.click();
		}
		else if (this->travelButton.contains(mouseOriginalPoint))
		{
			this->travelButton.click();
		}
		else if (this->backToWorldMapButton.contains(mouseOriginalPoint))
		{
			this->backToWorldMapButton.click(this->getGame());
		}

		// Check locations for clicks...
	}
}

void ProvinceMapPanel::tick(double dt)
{
	// Eventually blink the selected location.
	static_cast<void>(dt);
}

std::string ProvinceMapPanel::getBackgroundFilename() const
{
	const auto &exeData = this->getGame().getMiscAssets().getExeData();
	const std::string &filename =
		exeData.locations.provinceImgFilenames.at(this->provinceID);

	// Set all characters to uppercase because the texture manager expects 
	// extensions to be uppercase, and most filenames in A.EXE are lowercase.
	return String::toUppercase(filename);
}

void ProvinceMapPanel::drawButtonTooltip(ProvinceButtonName buttonName, Renderer &renderer)
{
	const std::string &text = ProvinceButtonTooltips.at(buttonName);

	Texture tooltip(Panel::createTooltip(
		text, FontName::D, this->getGame().getFontManager(), renderer));

	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);
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
		this->getGame().getFontManager());

	const TextBox::ShadowData shadowData(Color(48, 48, 48), Int2(1, 0));
	const TextBox textBox(center - Int2(0, 10), richText, &shadowData, renderer);

	// Clamp to screen edges, with some extra space on the left and right.
	const int x = std::max(std::min(textBox.getX(),
		Renderer::ORIGINAL_WIDTH - textBox.getSurface()->w - 2), 2);
	const int y = std::max(std::min(textBox.getY(),
		Renderer::ORIGINAL_HEIGHT - textBox.getSurface()->h - 2), 2);

	renderer.drawOriginal(textBox.getTexture(), x, y);
}

void ProvinceMapPanel::render(Renderer &renderer)
{
	assert(this->getGame().gameDataIsActive());

	// Clear full screen.
	renderer.clear();

	// Set palette.
	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Get the filename of the province map.
	auto &gameData = this->getGame().getGameData();
	const std::string backgroundFilename = this->getBackgroundFilename();

	// Draw province map background.
	const auto &mapBackground = textureManager.getTexture(
		backgroundFilename, PaletteFile::fromName(PaletteName::BuiltIn), renderer);
	renderer.drawOriginal(mapBackground.get());

	// Draw location icons, and find which place is closest to the mouse cursor.
	// Ignore locations with no name (ones that are zeroed out in the center province).
	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = this->getGame().getRenderer()
		.nativeToOriginal(mousePosition);

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
		TextureFile::fromName(TextureName::CityStateIcon), backgroundFilename, renderer);
	const auto &townIcon = textureManager.getTexture(
		TextureFile::fromName(TextureName::TownIcon), backgroundFilename, renderer);
	const auto &villageIcon = textureManager.getTexture(
		TextureFile::fromName(TextureName::VillageIcon), backgroundFilename, renderer);
	const auto &dungeonIcon = textureManager.getTexture(
		TextureFile::fromName(TextureName::DungeonIcon), backgroundFilename, renderer);

	const auto &cityData = this->getGame().getMiscAssets().getCityDataFile();
	const auto &province = cityData.getProvinceData(this->provinceID);

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

	// Draw dungeon icons.
	drawIcon(province.firstDungeon, dungeonIcon);

	if (this->provinceID != 8)
	{
		// Only draw staff dungeon if not the center province.
		const auto &staffDungeonIcon = textureManager.getTextures(
			TextureFile::fromName(TextureName::StaffDungeonIcons),
			backgroundFilename, renderer).at(this->provinceID);
		drawIcon(province.secondDungeon, staffDungeonIcon);
	}

	for (const auto &dungeon : province.randomDungeons)
	{
		drawIcon(dungeon, dungeonIcon);
	}

	// If the player is in the current province, highlight their current location.
	const auto &location = gameData.getLocation();
	if (this->provinceID == location.provinceID)
	{
		auto drawHighlight = [&renderer](
			const CityDataFile::ProvinceData::LocationData &location, const Texture &highlight)
		{
			const Int2 point(location.x, location.y);
			renderer.drawOriginal(highlight.get(),
				point.x - (highlight.getWidth() / 2),
				point.y - (highlight.getHeight() / 2));
		};

		// Generic highlights (city, town, village, and dungeon).
		const auto &highlights = textureManager.getTextures(
			TextureFile::fromName(TextureName::MapIconOutlines), backgroundFilename, renderer);

		auto handleCityHighlight = [&renderer, &province, &location,
			&drawHighlight, &highlights]()
		{
			const int localCityID = location.localCityID;

			if (localCityID < 8)
			{
				// City.
				const auto &highlight = highlights.front();
				const auto &locationData = province.cityStates.at(localCityID);
				drawHighlight(locationData, highlight);
			}
			else if (localCityID < 16)
			{
				// Town.
				const auto &highlight = highlights.at(1);
				const auto &locationData = province.towns.at(localCityID - 8);
				drawHighlight(locationData, highlight);
			}
			else
			{
				// Village.
				const auto &highlight = highlights.at(2);
				const auto &locationData = province.villages.at(localCityID - 16);
				drawHighlight(locationData, highlight);
			}
		};

		auto handleDungeonHighlight = [this, &renderer, &textureManager, &backgroundFilename,
			&province, &location, &drawHighlight, &highlights]()
		{
			const int localDungeonID = location.localDungeonID;

			if (localDungeonID == 0)
			{
				// Staff dungeon. It changes a value in the palette to give the icon's background
				// its yellow color (since there are no highlight icons for staff dungeons).
				const auto &locationData = province.secondDungeon;
				const Texture highlight = [this, &renderer]()
				{
					// Get the palette indices associated with the staff dungeon icon.
					const CIFFile &iconCif = *this->staffDungeonCif.get();
					const int cifWidth = iconCif.getWidth(this->provinceID);
					const int cifHeight = iconCif.getHeight(this->provinceID);

					// Make a copy of the staff dungeon icon with changes based on which
					// pixels should be highlighted.
					SDL_Surface *surface = Surface::createSurfaceWithFormat(
						cifWidth, cifHeight, Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);

					auto getColorFromIndex = [this, surface](int paletteIndex)
					{
						const Color &color = this->provinceMapPalette.get().at(paletteIndex);
						return SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);
					};

					const uint32_t highlightColor = getColorFromIndex(YellowPaletteIndex);

					// Convert each palette index to its equivalent 32-bit color, changing 
					// background indices to highlight indices as they are found.
					const uint8_t *srcPixels = iconCif.getRawPixels(this->provinceID);
					uint32_t *dstPixels = static_cast<uint32_t*>(surface->pixels);
					const int pixelCount = surface->w * surface->h;
					std::transform(srcPixels, srcPixels + pixelCount, dstPixels,
						[&getColorFromIndex, highlightColor](const uint8_t &srcPixel)
					{
						return (srcPixel == BackgroundPaletteIndex) ?
							highlightColor : getColorFromIndex(srcPixel);
					});

					Texture texture(renderer.createTextureFromSurface(surface));
					SDL_FreeSurface(surface);
					return texture;
				}();

				drawHighlight(locationData, highlight);
			}
			else if (localDungeonID == 1)
			{
				// Staff map dungeon.
				const auto &highlight = highlights.at(3);
				const auto &locationData = province.firstDungeon;
				drawHighlight(locationData, highlight);
			}
			else
			{
				// Named dungeon.
				const auto &highlight = highlights.at(3);
				const auto &locationData = province.randomDungeons.at(localDungeonID - 2);
				drawHighlight(locationData, highlight);
			}
		};

		// Decide how to highlight the location.
		if (location.dataType == LocationDataType::City)
		{
			handleCityHighlight();
		}
		else if (location.dataType == LocationDataType::Dungeon)
		{
			handleDungeonHighlight();
		}
		else if (location.dataType == LocationDataType::SpecialCase)
		{
			if (location.specialCaseType == Location::SpecialCaseType::StartDungeon)
			{
				// The starting dungeon is not technically on the world map (and the original
				// game doesn't allow the world map to open then, either).
			}
			else if (location.specialCaseType == Location::SpecialCaseType::WildDungeon)
			{
				// Draw the highlight for the city the wild dungeon is in.
				handleCityHighlight();
			}
			else
			{
				throw std::runtime_error("Bad special location type \"" +
					std::to_string(static_cast<int>(location.specialCaseType)) + "\".");
			}
		}
		else
		{
			throw std::runtime_error("Bad location data type \"" +
				std::to_string(static_cast<int>(location.dataType)) + "\".");
		}
	}

	// Draw the name of the location closest to the mouse cursor.
	this->drawLocationName(closestName, closestPosition, renderer);
}

void ProvinceMapPanel::renderSecondary(Renderer &renderer)
{
	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = this->getGame().getRenderer()
		.nativeToOriginal(mousePosition);

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
