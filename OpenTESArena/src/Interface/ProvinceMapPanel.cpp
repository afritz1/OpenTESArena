#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <unordered_map>

#include "SDL.h"

#include "CursorAlignment.h"
#include "FastTravelSubPanel.h"
#include "GameWorldPanel.h"
#include "ProvinceButtonName.h"
#include "ProvinceMapPanel.h"
#include "ProvinceSearchSubPanel.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "TextSubPanel.h"
#include "WorldMapPanel.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/ExeData.h"
#include "../Assets/IMGFile.h"
#include "../Assets/MIFFile.h"
#include "../Assets/MiscAssets.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Random.h"
#include "../Math/Rect.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/MusicName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"
#include "../Rendering/Texture.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"
#include "../World/Location.h"
#include "../World/LocationDataType.h"
#include "../World/WeatherType.h"

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
		{ ProvinceButtonName::Search, "Search" },
		{ ProvinceButtonName::Travel, "Travel" },
		{ ProvinceButtonName::BackToWorldMap, "Back to World Map" }
	};

	const std::unordered_map<ProvinceButtonName, Rect> ProvinceButtonClickAreas =
	{
		{ ProvinceButtonName::Search, Rect(34, Renderer::ORIGINAL_HEIGHT - 32, 18, 27) },
		{ ProvinceButtonName::Travel, Rect(53, Renderer::ORIGINAL_HEIGHT - 32, 18, 27) },
		{ ProvinceButtonName::BackToWorldMap, Rect(72, Renderer::ORIGINAL_HEIGHT - 32, 18, 27) }
	};
}

ProvinceMapPanel::TravelData::TravelData(int locationID, int provinceID, int travelDays)
{
	this->locationID = locationID;
	this->provinceID = provinceID;
	this->travelDays = travelDays;
}

const double ProvinceMapPanel::BLINK_PERIOD = 1.0 / 5.0;
const double ProvinceMapPanel::BLINK_PERIOD_PERCENT_ON = 0.75;

ProvinceMapPanel::ProvinceMapPanel(Game &game, int provinceID,
	std::unique_ptr<ProvinceMapPanel::TravelData> travelData)
	: Panel(game), travelData(std::move(travelData))
{
	this->searchButton = []()
	{
		const Rect &clickArea = ProvinceButtonClickAreas.at(ProvinceButtonName::Search);
		int x = clickArea.getLeft();
		int y = clickArea.getTop();
		int width = clickArea.getWidth();
		int height = clickArea.getHeight();
		auto function = [](Game &game, ProvinceMapPanel &panel)
		{
			// Push text entry sub-panel for location searching.
			game.pushSubPanel<ProvinceSearchSubPanel>(game, panel, panel.provinceID);
		};
		return Button<Game&, ProvinceMapPanel&>(x, y, width, height, function);
	}();

	this->travelButton = []()
	{
		const Rect &clickArea = ProvinceButtonClickAreas.at(ProvinceButtonName::Travel);
		int x = clickArea.getLeft();
		int y = clickArea.getTop();
		int width = clickArea.getWidth();
		int height = clickArea.getHeight();
		auto function = [](Game &game, ProvinceMapPanel &panel)
		{
			if (panel.travelData.get() != nullptr)
			{
				// Fast travel to the selected destination.
				panel.handleFastTravel();
			}
			else
			{
				// Display error message about no selected destination.
				const auto &exeData = game.getMiscAssets().getExeData();
				const std::string errorText = [&exeData]()
				{
					std::string text = exeData.travel.noDestination;

					// Remove carriage return at end.
					text.pop_back();

					// Replace carriage returns with newlines.
					text = String::replace(text, '\r', '\n');

					return text;
				}();

				std::unique_ptr<Panel> textPopUp = panel.makeTextPopUp(errorText);
				game.pushSubPanel(std::move(textPopUp));
			}
		};
		return Button<Game&, ProvinceMapPanel&>(x, y, width, height, function);
	}();

	this->backToWorldMapButton = []()
	{
		const Rect &clickArea = ProvinceButtonClickAreas.at(ProvinceButtonName::BackToWorldMap);
		int x = clickArea.getLeft();
		int y = clickArea.getTop();
		int width = clickArea.getWidth();
		int height = clickArea.getHeight();
		auto function = [](Game &game, std::unique_ptr<ProvinceMapPanel::TravelData> travelData)
		{
			game.setPanel<WorldMapPanel>(game, std::move(travelData));
		};
		return Button<Game&, std::unique_ptr<ProvinceMapPanel::TravelData>>(
			x, y, width, height, function);
	}();

	this->provinceID = provinceID;
	this->blinkTimer = 0.0;

	// Get the palette for the background image.
	this->provinceMapPalette = IMGFile::extractPalette(this->getBackgroundFilename());

	// If displaying a province that contains a staff dungeon, get the staff dungeon icon's
	// raw palette indices (for yellow and red color swapping).
	if (provinceID != Location::CENTER_PROVINCE_ID)
	{
		const std::string &cifName = TextureFile::fromName(TextureName::StaffDungeonIcons);
		this->staffDungeonCif = std::make_unique<CIFFile>(cifName);
	}
}

void ProvinceMapPanel::trySelectLocation(int selectedLocationID)
{
	auto &game = this->getGame();
	auto &gameData = game.getGameData();

	// Get the current location to compare with.
	const auto &currentLocation = gameData.getLocation();
	const int currentLocationID = [&currentLocation]()
	{
		if (currentLocation.dataType == LocationDataType::City)
		{
			return Location::cityToLocationID(currentLocation.localCityID);
		}
		else if (currentLocation.dataType == LocationDataType::Dungeon)
		{
			return Location::dungeonToLocationID(currentLocation.localDungeonID);
		}
		else if (currentLocation.dataType == LocationDataType::SpecialCase)
		{
			const Location::SpecialCaseType specialCaseType =
				currentLocation.specialCaseType;

			if (specialCaseType == Location::SpecialCaseType::StartDungeon)
			{
				// Technically this shouldn't be allowed, so just use a placeholder.
				return Location::dungeonToLocationID(0);
			}
			else if (specialCaseType == Location::SpecialCaseType::WildDungeon)
			{
				return Location::cityToLocationID(currentLocation.localCityID);
			}
			else
			{
				throw DebugException("Bad special location type \"" +
					std::to_string(static_cast<int>(specialCaseType)) + "\".");
			}
		}
		else
		{
			throw DebugException("Bad location data type \"" +
				std::to_string(static_cast<int>(currentLocation.dataType)) + "\".");
		}
	}();

	// Only continue if the selected location is not the player's current location.
	const bool matchesPlayerLocation =
		(this->provinceID == currentLocation.provinceID) &&
		(currentLocationID == selectedLocationID);

	if (!matchesPlayerLocation)
	{
		// Set the travel data for the selected location and reset the blink timer.
		const auto &miscAssets = game.getMiscAssets();
		const auto &cityData = gameData.getCityDataFile();
		const Date &currentDate = gameData.getDate();
		const int travelDays = cityData.getTravelDays(
			currentLocationID, currentLocation.provinceID,
			selectedLocationID, this->provinceID, currentDate.getMonth(),
			gameData.getWeathersArray(), gameData.getRandom(), miscAssets);
		this->travelData = std::make_unique<TravelData>(
			selectedLocationID, this->provinceID, travelDays);
		this->blinkTimer = 0.0;

		// Create pop-up travel dialog.
		const std::string travelText = this->makeTravelText(currentLocationID,
			currentLocation, selectedLocationID, *this->travelData.get());
		std::unique_ptr<Panel> textPopUp = this->makeTextPopUp(travelText);
		game.pushSubPanel(std::move(textPopUp));
	}
	else
	{
		// Cannot travel to the player's current location. Create an error pop-up.
		const std::string errorText = [&game, &gameData, &currentLocation]()
		{
			const auto &cityData = gameData.getCityDataFile();
			const auto &exeData = game.getMiscAssets().getExeData();
			std::string text = exeData.travel.alreadyAtDestination;

			// Remove carriage return at end.
			text.pop_back();

			// Replace carriage returns with newlines.
			text = String::replace(text, '\r', '\n');

			// Replace %s with location name.
			size_t index = text.find("%s");
			text.replace(index, 2, currentLocation.getName(cityData, exeData));

			return text;
		}();

		std::unique_ptr<Panel> textPopUp = this->makeTextPopUp(errorText);
		game.pushSubPanel(std::move(textPopUp));
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
	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	if (escapePressed || rightClick)
	{
		this->backToWorldMapButton.click(game, std::move(this->travelData));
	}

	const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPosition = game.getRenderer()
			.nativeToOriginal(mousePosition);

		if (this->searchButton.contains(originalPosition))
		{
			this->searchButton.click(game, *this);
		}
		else if (this->travelButton.contains(originalPosition))
		{
			this->travelButton.click(game, *this);
		}
		else if (this->backToWorldMapButton.contains(originalPosition))
		{
			this->backToWorldMapButton.click(game, std::move(this->travelData));
		}
		else
		{
			// The closest location to the cursor was clicked. See if it can be set as the
			// travel destination (depending on whether the player is already there).
			const int closestLocationID = this->getClosestLocationID(originalPosition);
			this->trySelectLocation(closestLocationID);
		}
	}
}

std::string ProvinceMapPanel::getBackgroundFilename() const
{
	const auto &exeData = this->getGame().getMiscAssets().getExeData();
	const auto &provinceImgFilenames = exeData.locations.provinceImgFilenames;
	const std::string &filename = provinceImgFilenames.at(this->provinceID);

	// Set all characters to uppercase because the texture manager expects 
	// extensions to be uppercase, and most filenames in A.EXE are lowercase.
	return String::toUppercase(filename);
}

int ProvinceMapPanel::getClosestLocationID(const Int2 &originalPosition) const
{
	// Initialize the current closest position to something very far away (watch out for
	// integer multiplication overflow with closest distance).
	Int2 closestPosition(-1000, -1000);

	// Lambda for comparing distances of two location points to the mouse position.
	auto closerThanCurrentClosest = [&originalPosition, &closestPosition](const Int2 &point)
	{
		const Int2 diff = point - originalPosition;
		const Int2 closestDiff = closestPosition - originalPosition;
		const double distance = std::sqrt((diff.x * diff.x) + (diff.y * diff.y));
		const double closestDistance = std::sqrt(
			(closestDiff.x * closestDiff.x) + (closestDiff.y * closestDiff.y));
		return distance < closestDistance;
	};

	// Look through all visible locations to find the one closest to the mouse.
	int closestID = -1;
	const auto &cityData = this->getGame().getGameData().getCityDataFile();
	const auto &provinceData = cityData.getProvinceData(this->provinceID);
	for (int i = 0; i < 48; i++)
	{
		const auto &locationData = provinceData.getLocationData(i);

		if (locationData.isVisible())
		{
			const Int2 point(locationData.x, locationData.y);

			if (closerThanCurrentClosest(point))
			{
				closestPosition = point;
				closestID = i;
			}
		}
	}

	DebugAssert(closestID >= 0, "No closest location ID found.");
	return closestID;
}

std::string ProvinceMapPanel::makeTravelText(int currentLocationID,
	const Location &currentLocation, int closestLocationID,
	const ProvinceMapPanel::TravelData &travelData) const
{
	auto &game = this->getGame();
	auto &gameData = this->getGame().getGameData();
	const auto &miscAssets = game.getMiscAssets();
	const auto &exeData = miscAssets.getExeData();
	const auto &cityData = gameData.getCityDataFile();
	const auto &closestProvinceData = cityData.getProvinceData(this->provinceID);
	const auto &closestLocationData = closestProvinceData.getLocationData(closestLocationID);
	const Date &currentDate = gameData.getDate();
	const Date destinationDate = [&currentDate, &travelData]()
	{
		Date newDate = currentDate;
		for (int i = 0; i < travelData.travelDays; i++)
		{
			newDate.incrementDay();
		}

		return newDate;
	}();

	const std::string locationFormatText = [this, &exeData, &closestProvinceData,
		closestLocationID, &closestLocationData]()
	{
		std::string formatText = [this, &exeData, &closestProvinceData,
			closestLocationID, &closestLocationData]()
		{
			// Decide the format based on whether it's the center province.
			if (this->provinceID != Location::CENTER_PROVINCE_ID)
			{
				// Determine whether to use the city format or dungeon format.
				if (closestLocationID < 32)
				{
					// City format.
					std::string text = exeData.travel.locationFormatTexts.at(2);

					// Replace first %s with location type.
					const std::string &locationTypeName = [&exeData, closestLocationID]()
					{
						if (closestLocationID < 8)
						{
							// City.
							return exeData.locations.locationTypes.front();
						}
						else if (closestLocationID < 16)
						{
							// Town.
							return exeData.locations.locationTypes.at(1);
						}
						else
						{
							// Village.
							return exeData.locations.locationTypes.at(2);
						}
					}();

					size_t index = text.find("%s");
					text.replace(index, 2, locationTypeName);

					// Replace second %s with location name.
					index = text.find("%s", index);
					text.replace(index, 2, closestLocationData.name);

					// Replace third %s with province name.
					index = text.find("%s", index);
					text.replace(index, 2, closestProvinceData.name);

					return text;
				}
				else
				{
					// Dungeon format.
					std::string text = exeData.travel.locationFormatTexts.at(0);

					// Replace first %s with dungeon name.
					size_t index = text.find("%s");
					text.replace(index, 2, closestLocationData.name);

					// Replace second %s with province name.
					index = text.find("%s", index);
					text.replace(index, 2, closestProvinceData.name);

					return text;
				}
			}
			else
			{
				// Center province format (always the center city).
				std::string text = exeData.travel.locationFormatTexts.at(1);

				// Replace first %s with center province city name.
				size_t index = text.find("%s");
				text.replace(index, 2, closestLocationData.name);

				// Replace second %s with center province name.
				index = text.find("%s", index);
				text.replace(index, 2, closestProvinceData.name);

				return text;
			}
		}();

		// Replace carriage returns with newlines.
		formatText = String::replace(formatText, '\r', '\n');
		return formatText;
	}();

	// Lambda for getting the date string for a given date.
	auto getDateString = [&exeData](const Date &date)
	{
		std::string text = exeData.status.date;

		// Replace carriage returns with newlines.
		text = String::replace(text, '\r', '\n');

		// Remove newline at end.
		text.pop_back();

		// Replace first %s with weekday.
		const std::string &weekdayString =
			exeData.calendar.weekdayNames.at(date.getWeekday());
		size_t index = text.find("%s");
		text.replace(index, 2, weekdayString);

		// Replace %u%s with day and ordinal suffix.
		const std::string dayString = date.getOrdinalDay();
		index = text.find("%u%s");
		text.replace(index, 4, dayString);

		// Replace third %s with month.
		const std::string &monthString =
			exeData.calendar.monthNames.at(date.getMonth());
		index = text.find("%s");
		text.replace(index, 2, monthString);

		// Replace %d with year.
		index = text.find("%d");
		text.replace(index, 2, std::to_string(date.getYear()));

		return text;
	};

	const std::string startDateString = [&exeData, &getDateString, &currentDate]()
	{
		// The date prefix is shared between the province map pop-up and the arrival pop-up.
		const std::string datePrefix = exeData.travel.arrivalPopUpDate;

		// Replace carriage returns with newlines.
		return String::replace(datePrefix + getDateString(currentDate), '\r', '\n');
	}();

	const std::string dayString = [&exeData, &travelData]()
	{
		const std::string &dayStringPrefix = exeData.travel.dayPrediction.front();
		const std::string dayStringBody = [&exeData, &travelData]()
		{
			std::string text = exeData.travel.dayPrediction.back();

			// Replace %d with travel days.
			const size_t index = text.find("%d");
			text.replace(index, 2, std::to_string(travelData.travelDays));

			return text;
		}();

		// Replace carriage returns with newlines.
		return String::replace(dayStringPrefix + dayStringBody, '\r', '\n');
	}();

	const int travelDistance = [this, &cityData, &currentLocation,
		currentLocationID, &closestProvinceData, &closestLocationData]()
	{
		const auto &currentProvinceData =
			cityData.getProvinceData(currentLocation.provinceID);
		const auto &currentLocationData =
			currentProvinceData.getLocationData(currentLocationID);
		const Rect currentProvinceRect = currentProvinceData.getGlobalRect();
		const Rect closestProvinceRect = closestProvinceData.getGlobalRect();
		const Int2 currentLocationGlobalPoint = cityData.localPointToGlobal(
			Int2(currentLocationData.x, currentLocationData.y),
			currentProvinceRect);
		const Int2 closestLocationGlobalPoint = cityData.localPointToGlobal(
			Int2(closestLocationData.x, closestLocationData.y),
			closestProvinceRect);

		return cityData.getDistance(
			currentLocationGlobalPoint, closestLocationGlobalPoint);
	}();

	const std::string distanceString = [&exeData, travelDistance]()
	{
		std::string text = exeData.travel.distancePrediction;

		// Replace %d with travel distance.
		const size_t index = text.find("%d");
		text.replace(index, 2, std::to_string(travelDistance * 20));

		// Replace carriage returns with newlines.
		return String::replace(text, '\r', '\n');
	}();

	const std::string arrivalDateString = [&exeData, &getDateString, &destinationDate]()
	{
		const std::string text = exeData.travel.arrivalDatePrediction;

		// Replace carriage returns with newlines.
		return String::replace(text + getDateString(destinationDate), '\r', '\n');
	}();

	return locationFormatText + startDateString + "\n\n" + dayString +
		distanceString + arrivalDateString;
}

std::unique_ptr<Panel> ProvinceMapPanel::makeTextPopUp(const std::string &text) const
{
	auto &game = this->getGame();
	auto &gameData = game.getGameData();

	const Int2 center(Renderer::ORIGINAL_WIDTH / 2, 98);
	const Color color(52, 24, 8);
	const int lineSpacing = 1;

	const RichTextString richText(
		text,
		FontName::Arena,
		color,
		TextAlignment::Center,
		lineSpacing,
		game.getFontManager());

	// Parchment minimum height is 40 pixels.
	const int parchmentHeight = std::max(richText.getDimensions().y + 16, 40);

	Texture texture(Texture::generate(Texture::PatternType::Parchment,
		richText.getDimensions().x + 20, parchmentHeight,
		game.getTextureManager(), game.getRenderer()));

	const Int2 textureCenter(
		(Renderer::ORIGINAL_WIDTH / 2) - 1,
		(Renderer::ORIGINAL_HEIGHT / 2) - 1);

	auto function = [](Game &game)
	{
		game.popSubPanel();
	};

	return std::make_unique<TextSubPanel>(game, center, richText, function,
		std::move(texture), textureCenter);
}

void ProvinceMapPanel::tick(double dt)
{
	// Update the blink timer. Depending on which interval it's in, this will make the
	// currently selected location (if any) have a red highlight.
	this->blinkTimer += dt;
}

void ProvinceMapPanel::handleFastTravel()
{
	// Switch to world map and push fast travel sub-panel on top of it.
	auto &game = this->getGame();
	game.pushSubPanel<FastTravelSubPanel>(game, *this->travelData.get());
	game.setPanel<WorldMapPanel>(game, std::move(this->travelData));
}

void ProvinceMapPanel::drawCenteredIcon(const Texture &texture,
	const Int2 &point, Renderer &renderer)
{
	renderer.drawOriginal(texture.get(),
		point.x - (texture.getWidth() / 2),
		point.y - (texture.getHeight() / 2));
}

void ProvinceMapPanel::drawVisibleLocations(const std::string &backgroundFilename,
	TextureManager &textureManager, Renderer &renderer)
{
	// Lambda for drawing a location icon if it's visible.
	auto drawIconIfVisible = [this, &renderer](
		const CityDataFile::ProvinceData::LocationData &location, const Texture &icon)
	{
		// Only draw visible locations.
		if (location.isVisible())
		{
			const Int2 point(location.x, location.y);
			this->drawCenteredIcon(icon, point, renderer);
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

	const auto &cityData = this->getGame().getGameData().getCityDataFile();
	const auto &province = cityData.getProvinceData(this->provinceID);

	// Draw city-state icons.
	for (const auto &cityState : province.cityStates)
	{
		drawIconIfVisible(cityState, cityStateIcon);
	}

	// Draw town icons.
	for (const auto &town : province.towns)
	{
		drawIconIfVisible(town, townIcon);
	}

	// Draw village icons.
	for (const auto &village : province.villages)
	{
		drawIconIfVisible(village, villageIcon);
	}

	// Draw dungeon icons.
	drawIconIfVisible(province.firstDungeon, dungeonIcon);

	if (this->provinceID != Location::CENTER_PROVINCE_ID)
	{
		// Only draw staff dungeon if not the center province.
		const auto &staffDungeonIcon = textureManager.getTextures(
			TextureFile::fromName(TextureName::StaffDungeonIcons),
			backgroundFilename, renderer).at(this->provinceID);
		drawIconIfVisible(province.secondDungeon, staffDungeonIcon);
	}

	for (const auto &dungeon : province.randomDungeons)
	{
		drawIconIfVisible(dungeon, dungeonIcon);
	}
}

void ProvinceMapPanel::drawLocationHighlight(const Location &location,
	LocationHighlightType highlightType, const std::string &backgroundFilename,
	TextureManager &textureManager, Renderer &renderer)
{
	auto drawHighlight = [this, &renderer](
		const CityDataFile::ProvinceData::LocationData &location, const Texture &highlight)
	{
		const Int2 point(location.x, location.y);
		this->drawCenteredIcon(highlight, point, renderer);
	};

	const auto &cityData = this->getGame().getGameData().getCityDataFile();
	const auto &province = cityData.getProvinceData(location.provinceID);

	// Generic highlights (city, town, village, and dungeon).
	const std::string &outlinesFilename = TextureFile::fromName(
		(highlightType == ProvinceMapPanel::LocationHighlightType::Current) ?
		TextureName::MapIconOutlines : TextureName::MapIconOutlinesBlinking);
	const auto &highlights = textureManager.getTextures(
		outlinesFilename, backgroundFilename, renderer);

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
		highlightType, &province, &location, &drawHighlight, &highlights]()
	{
		const int localDungeonID = location.localDungeonID;

		if (localDungeonID == 0)
		{
			// Staff dungeon. It changes a value in the palette to give the icon's background
			// its yellow color (since there are no highlight icons for staff dungeons).
			const auto &locationData = province.secondDungeon;
			const Texture highlight = [this, highlightType, &renderer]()
			{
				// Get the palette indices associated with the staff dungeon icon.
				const CIFFile &iconCif = *this->staffDungeonCif.get();
				const int cifWidth = iconCif.getWidth(this->provinceID);
				const int cifHeight = iconCif.getHeight(this->provinceID);

				// Make a copy of the staff dungeon icon with changes based on which
				// pixels should be highlighted.
				SDL_Surface *surface = Surface::createSurfaceWithFormat(
					cifWidth, cifHeight, Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);

				auto getColorFromIndex = [this, highlightType, surface](int paletteIndex)
				{
					const Color &color = this->provinceMapPalette.get().at(paletteIndex);
					return SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);
				};

				const uint32_t highlightColor = getColorFromIndex(
					(highlightType == ProvinceMapPanel::LocationHighlightType::Current) ?
					YellowPaletteIndex : RedPaletteIndex);

				// Convert each palette index to its equivalent 32-bit color, changing 
				// background indices to highlight indices as they are found.
				const uint8_t *srcPixels = iconCif.getPixels(this->provinceID);
				uint32_t *dstPixels = static_cast<uint32_t*>(surface->pixels);
				const int pixelCount = surface->w * surface->h;
				std::transform(srcPixels, srcPixels + pixelCount, dstPixels,
					[&getColorFromIndex, highlightColor](uint8_t srcPixel)
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
			throw DebugException("Bad special location type \"" +
				std::to_string(static_cast<int>(location.specialCaseType)) + "\".");
		}
	}
	else
	{
		throw DebugException("Bad location data type \"" +
			std::to_string(static_cast<int>(location.dataType)) + "\".");
	}
}

void ProvinceMapPanel::drawLocationName(int locationID, Renderer &renderer)
{
	auto &gameData = this->getGame().getGameData();
	const auto &cityData = gameData.getCityDataFile();
	const auto &province = cityData.getProvinceData(this->provinceID);
	const auto &location = province.getLocationData(locationID);
	const Int2 center(location.x, location.y);

	const RichTextString richText(
		location.name,
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

void ProvinceMapPanel::render(Renderer &renderer)
{
	assert(this->getGame().gameDataIsActive());

	// Clear full screen.
	renderer.clear();

	// Set palette.
	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw province map background.
	const std::string backgroundFilename = this->getBackgroundFilename();
	const auto &mapBackground = textureManager.getTexture(
		backgroundFilename, PaletteFile::fromName(PaletteName::BuiltIn), renderer);
	renderer.drawOriginal(mapBackground.get());

	// Draw visible location icons.
	this->drawVisibleLocations(backgroundFilename, textureManager, renderer);

	// If the player is in the current province, highlight their current location.
	auto &gameData = this->getGame().getGameData();
	const auto &location = gameData.getLocation();
	if (this->provinceID == location.provinceID)
	{
		const auto highlightType = ProvinceMapPanel::LocationHighlightType::Current;
		this->drawLocationHighlight(location, highlightType, backgroundFilename,
			textureManager, renderer);
	}

	// If there is a currently selected location in this province, draw its blinking highlight
	// if within the "blink on" interval.
	if ((this->travelData.get() != nullptr) && (this->travelData->provinceID == this->provinceID))
	{
		const double blinkInterval = std::fmod(this->blinkTimer, ProvinceMapPanel::BLINK_PERIOD);
		const double blinkPeriodPercent = blinkInterval / ProvinceMapPanel::BLINK_PERIOD;

		// See if the blink period percent lies within the "on" percent. Use less-than
		// to compare them so the on-state appears before the off-state.
		if (blinkPeriodPercent < ProvinceMapPanel::BLINK_PERIOD_PERCENT_ON)
		{
			const Location selectedLocation = Location::makeFromLocationID(
				this->travelData->locationID, this->provinceID);

			const auto highlightType = ProvinceMapPanel::LocationHighlightType::Selected;
			this->drawLocationHighlight(selectedLocation, highlightType,
				backgroundFilename, textureManager, renderer);
		}
	}
}

void ProvinceMapPanel::renderSecondary(Renderer &renderer)
{
	// Draw the name of the location closest to the mouse cursor.
	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = this->getGame().getRenderer()
		.nativeToOriginal(mousePosition);
	const int closestLocationID = this->getClosestLocationID(originalPosition);
	this->drawLocationName(closestLocationID, renderer);

	// Draw a tooltip if the mouse is over a button.
	const auto tooltipIter = std::find_if(
		ProvinceButtonTooltips.begin(), ProvinceButtonTooltips.end(),
		[&originalPosition](const std::pair<ProvinceButtonName, std::string> &pair)
	{
		const Rect &clickArea = ProvinceButtonClickAreas.at(pair.first);
		return clickArea.contains(originalPosition);
	});

	if (tooltipIter != ProvinceButtonTooltips.end())
	{
		this->drawButtonTooltip(tooltipIter->first, renderer);
	}
}
