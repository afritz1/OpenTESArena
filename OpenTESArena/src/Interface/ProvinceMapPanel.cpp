#include <algorithm>
#include <array>
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
#include "Surface.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "TextSubPanel.h"
#include "Texture.h"
#include "WorldMapPanel.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/ExeData.h"
#include "../Assets/IMGFile.h"
#include "../Assets/MIFFile.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/MathUtils.h"
#include "../Math/Random.h"
#include "../Math/Rect.h"
#include "../Media/FontLibrary.h"
#include "../Media/FontName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../World/LocationDefinition.h"
#include "../World/LocationInstance.h"
#include "../World/LocationUtils.h"
#include "../World/WeatherType.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace
{
	// .CIF palette indices for staff dungeon outlines.
	constexpr uint8_t BackgroundPaletteIndex = 220;
	constexpr uint8_t YellowPaletteIndex = 194;
	constexpr uint8_t RedPaletteIndex = 223;

	const std::unordered_map<ProvinceButtonName, std::string> ProvinceButtonTooltips =
	{
		{ ProvinceButtonName::Search, "Search" },
		{ ProvinceButtonName::Travel, "Travel" },
		{ ProvinceButtonName::BackToWorldMap, "Back to World Map" }
	};

	const std::unordered_map<ProvinceButtonName, Rect> ProvinceButtonClickAreas =
	{
		{ ProvinceButtonName::Search, Rect(34, ArenaRenderUtils::SCREEN_HEIGHT - 32, 18, 27) },
		{ ProvinceButtonName::Travel, Rect(53, ArenaRenderUtils::SCREEN_HEIGHT - 32, 18, 27) },
		{ ProvinceButtonName::BackToWorldMap, Rect(72, ArenaRenderUtils::SCREEN_HEIGHT - 32, 18, 27) }
	};
}

ProvinceMapPanel::TravelData::TravelData(int locationID, int provinceID, int travelDays)
{
	this->locationID = locationID;
	this->provinceID = provinceID;
	this->travelDays = travelDays;
}

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
				const auto &exeData = game.getBinaryAssetLibrary().getExeData();
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
	const std::string backgroundFilename = this->getBackgroundFilename();
	if (!IMGFile::tryExtractPalette(backgroundFilename.c_str(), this->provinceMapPalette))
	{
		DebugCrash("Could not extract palette from \"" + backgroundFilename + "\".");
	}

	// If displaying a province that contains a staff dungeon, get the staff dungeon icon's
	// raw palette indices (for yellow and red color swapping).
	const bool hasStaffDungeon = provinceID != LocationUtils::CENTER_PROVINCE_ID;
	if (hasStaffDungeon)
	{
		const std::string &cifName = ArenaTextureName::StaffDungeonIcons;
		this->staffDungeonCif = CIFFile();
		if (!this->staffDungeonCif.init(cifName.c_str()))
		{
			DebugCrash("Could not init .CIF file \"" + cifName + "\".");
		}
	}
}

void ProvinceMapPanel::trySelectLocation(int selectedLocationID)
{
	auto &game = this->getGame();
	auto &gameData = game.getGameData();
	const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();

	const WorldMapDefinition &worldMapDef = gameData.getWorldMapDefinition();
	const ProvinceDefinition &currentProvinceDef = gameData.getProvinceDefinition();
	const LocationDefinition &currentLocationDef = gameData.getLocationDefinition();

	const ProvinceDefinition &selectedProvinceDef = worldMapDef.getProvinceDef(this->provinceID);
	const LocationDefinition &selectedLocationDef = selectedProvinceDef.getLocationDef(selectedLocationID);

	// Only continue if the selected location is not the player's current location.
	const bool matchesPlayerLocation = selectedProvinceDef.matches(currentProvinceDef) &&
		selectedLocationDef.matches(currentLocationDef);

	if (!matchesPlayerLocation)
	{
		// Set the travel data for the selected location and reset the blink timer.
		const Date &currentDate = gameData.getDate();

		// Use a copy of the RNG so displaying the travel pop-up multiple times doesn't
		// cause different day amounts.
		ArenaRandom tempRandom = gameData.getRandom();

		auto makeGlobalPoint = [](const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef)
		{
			const Int2 localPoint(locationDef.getScreenX(), locationDef.getScreenY());
			return LocationUtils::getGlobalPoint(localPoint, provinceDef.getGlobalRect());
		};

		const Int2 srcGlobalPoint = makeGlobalPoint(currentLocationDef, currentProvinceDef);
		const Int2 dstGlobalPoint = makeGlobalPoint(selectedLocationDef, selectedProvinceDef);
		const int travelDays = LocationUtils::getTravelDays(srcGlobalPoint, dstGlobalPoint,
			currentDate.getMonth(), gameData.getWeathersArray(), tempRandom, binaryAssetLibrary);

		this->travelData = std::make_unique<TravelData>(selectedLocationID, this->provinceID, travelDays);
		this->blinkTimer = 0.0;

		// Create pop-up travel dialog.
		const std::string travelText = this->makeTravelText(currentLocationDef, currentProvinceDef,
			selectedLocationID, *this->travelData.get());
		std::unique_ptr<Panel> textPopUp = this->makeTextPopUp(travelText);
		game.pushSubPanel(std::move(textPopUp));
	}
	else
	{
		// Cannot travel to the player's current location. Create an error pop-up.
		const std::string errorText = [&gameData, &binaryAssetLibrary, &currentLocationDef]()
		{
			const std::string &currentLocationName = [&gameData, &currentLocationDef]() -> const std::string&
			{
				const LocationInstance &currentLocationInst = gameData.getLocationInstance();
				return currentLocationInst.getName(currentLocationDef);
			}();

			const auto &exeData = binaryAssetLibrary.getExeData();
			std::string text = exeData.travel.alreadyAtDestination;

			// Remove carriage return at end.
			text.pop_back();

			// Replace carriage returns with newlines.
			text = String::replace(text, '\r', '\n');

			// Replace %s with location name.
			size_t index = text.find("%s");
			text.replace(index, 2, currentLocationName);

			return text;
		}();

		std::unique_ptr<Panel> textPopUp = this->makeTextPopUp(errorText);
		game.pushSubPanel(std::move(textPopUp));
	}
}

std::optional<Panel::CursorData> ProvinceMapPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
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
	const auto &exeData = this->getGame().getBinaryAssetLibrary().getExeData();
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
		// @todo: change to distance squared
		const double distance = std::sqrt((diff.x * diff.x) + (diff.y * diff.y));
		const double closestDistance = std::sqrt(
			(closestDiff.x * closestDiff.x) + (closestDiff.y * closestDiff.y));
		return distance < closestDistance;
	};

	// Look through all visible locations to find the one closest to the mouse.
	int closestIndex = -1;
	auto &game = this->getGame();
	auto &gameData = game.getGameData();
	const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();

	const WorldMapInstance &worldMapInst = gameData.getWorldMapInstance();
	const ProvinceInstance &provinceInst = worldMapInst.getProvinceInstance(this->provinceID);
	const int provinceDefIndex = provinceInst.getProvinceDefIndex();
	const WorldMapDefinition &worldMapDef = gameData.getWorldMapDefinition();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceDefIndex);

	for (int i = 0; i < provinceInst.getLocationCount(); i++)
	{
		const LocationInstance &locationInst = provinceInst.getLocationInstance(i);
		if (locationInst.isVisible())
		{
			const int locationDefIndex = locationInst.getLocationDefIndex();
			const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);
			const Int2 point(locationDef.getScreenX(), locationDef.getScreenY());

			if (closerThanCurrentClosest(point))
			{
				closestPosition = point;
				closestIndex = i;
			}
		}
	}

	DebugAssertMsg(closestIndex >= 0, "No closest location ID found.");
	return closestIndex;
}

std::string ProvinceMapPanel::makeTravelText(const LocationDefinition &srcLocationDef,
	const ProvinceDefinition &srcProvinceDef, int dstLocationIndex,
	const ProvinceMapPanel::TravelData &travelData) const
{
	auto &game = this->getGame();
	auto &gameData = this->getGame().getGameData();
	const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
	const auto &exeData = binaryAssetLibrary.getExeData();
	const WorldMapInstance &worldMapInst = gameData.getWorldMapInstance();
	const ProvinceInstance &dstProvinceInst = worldMapInst.getProvinceInstance(this->provinceID);
	const LocationInstance &dstLocationInst = dstProvinceInst.getLocationInstance(dstLocationIndex);

	const WorldMapDefinition &worldMapDef = gameData.getWorldMapDefinition();
	const int dstProvinceDefIndex = dstProvinceInst.getProvinceDefIndex();
	const ProvinceDefinition &dstProvinceDef = worldMapDef.getProvinceDef(dstProvinceDefIndex);
	const int dstLocationDefIndex = dstLocationInst.getLocationDefIndex();
	const LocationDefinition &dstLocationDef = dstProvinceDef.getLocationDef(dstLocationDefIndex);
	const std::string &dstLocationName = dstLocationInst.getName(dstLocationDef);

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

	const std::string locationFormatText = [this, &exeData, &dstProvinceDef,
		dstLocationIndex, &dstLocationDef, &dstLocationName]()
	{
		std::string formatText = [this, &exeData, &dstProvinceDef, dstLocationIndex,
			&dstLocationDef, &dstLocationName]()
		{
			const auto &locationFormatTexts = exeData.travel.locationFormatTexts;

			// Decide the format based on whether it's the center province.
			if (this->provinceID != LocationUtils::CENTER_PROVINCE_ID)
			{
				// Determine whether to use the city format or dungeon format.
				if (dstLocationDef.getType() == LocationDefinition::Type::City)
				{
					// City format.
					const int locationFormatTextsIndex = 2;
					DebugAssertIndex(locationFormatTexts, locationFormatTextsIndex);
					std::string text = locationFormatTexts[locationFormatTextsIndex];

					// Replace first %s with location type.
					const LocationDefinition::CityDefinition &cityDef = dstLocationDef.getCityDefinition();
					const std::string_view locationTypeName = cityDef.typeDisplayName;

					size_t index = text.find("%s");
					text.replace(index, 2, locationTypeName);

					// Replace second %s with location name.
					index = text.find("%s", index);
					text.replace(index, 2, dstLocationName);

					// Replace third %s with province name.
					index = text.find("%s", index);
					text.replace(index, 2, dstProvinceDef.getName());

					return text;
				}
				else if ((dstLocationDef.getType() == LocationDefinition::Type::Dungeon) ||
					(dstLocationDef.getType() == LocationDefinition::Type::MainQuestDungeon))
				{
					// Dungeon format.
					const int locationFormatTextsIndex = 0;
					DebugAssertIndex(locationFormatTexts, locationFormatTextsIndex);
					std::string text = locationFormatTexts[locationFormatTextsIndex];

					// Replace first %s with dungeon name.
					size_t index = text.find("%s");
					text.replace(index, 2, dstLocationName);

					// Replace second %s with province name.
					index = text.find("%s", index);
					text.replace(index, 2, dstProvinceDef.getName());

					return text;
				}
				else
				{
					DebugUnhandledReturnMsg(std::string,
						std::to_string(static_cast<int>(dstLocationDef.getType())));
				}
			}
			else
			{
				// Center province format (always the center city).
				const int locationFormatTextsIndex = 1;
				DebugAssertIndex(locationFormatTexts, locationFormatTextsIndex);
				std::string text = locationFormatTexts[locationFormatTextsIndex];

				// Replace first %s with center province city name.
				size_t index = text.find("%s");
				text.replace(index, 2, dstLocationName);

				// Replace second %s with center province name.
				index = text.find("%s", index);
				text.replace(index, 2, dstProvinceDef.getName());

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

	const int travelDistance = [this, &srcLocationDef, &srcProvinceDef, &worldMapDef,
		&dstProvinceDef, &dstLocationDef]()
	{
		const Rect srcProvinceRect = srcProvinceDef.getGlobalRect();
		const Rect dstProvinceRect = dstProvinceDef.getGlobalRect();
		const Int2 srcLocationGlobalPoint = LocationUtils::getGlobalPoint(
			Int2(srcLocationDef.getScreenX(), srcLocationDef.getScreenY()), srcProvinceRect);
		const Int2 dstLocationGlobalPoint = LocationUtils::getGlobalPoint(
			Int2(dstLocationDef.getScreenX(), dstLocationDef.getScreenY()), dstProvinceRect);
		return LocationUtils::getMapDistance(srcLocationGlobalPoint, dstLocationGlobalPoint);
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

	const Int2 center(ArenaRenderUtils::SCREEN_WIDTH / 2, 98);
	const Color color(52, 24, 8);
	const int lineSpacing = 1;

	const RichTextString richText(
		text,
		FontName::Arena,
		color,
		TextAlignment::Center,
		lineSpacing,
		game.getFontLibrary());

	// Parchment minimum height is 40 pixels.
	const int parchmentHeight = std::max(richText.getDimensions().y + 16, 40);

	Texture texture = TextureUtils::generate(TextureUtils::PatternType::Parchment,
		richText.getDimensions().x + 20, parchmentHeight,
		game.getTextureManager(), game.getRenderer());

	const Int2 textureCenter(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);

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

void ProvinceMapPanel::drawCenteredIcon(const Texture &texture, const Int2 &point, Renderer &renderer)
{
	const int x = point.x - (texture.getWidth() / 2);
	const int y = point.y - (texture.getHeight() / 2);
	renderer.drawOriginal(texture, x, y);
}

void ProvinceMapPanel::drawCenteredIcon(TextureBuilderID textureBuilderID, PaletteID paletteID,
	const Int2 &point, Renderer &renderer)
{
	const auto &textureManager = this->getGame().getTextureManager();
	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(textureBuilderID);
	const int x = point.x - (textureBuilder.getWidth() / 2);
	const int y = point.y - (textureBuilder.getHeight() / 2);
	renderer.drawOriginal(textureBuilderID, paletteID, x, y, textureManager);
}

void ProvinceMapPanel::drawVisibleLocations(const std::string &backgroundFilename,
	TextureManager &textureManager, Renderer &renderer)
{
	const std::optional<PaletteID> backgroundPaletteID = textureManager.tryGetPaletteID(backgroundFilename.c_str());
	if (!backgroundPaletteID.has_value())
	{
		DebugLogError("Couldn't get background palette ID for \"" + backgroundFilename + "\".");
		return;
	}

	const std::string &cityStateIconFilename = ArenaTextureName::CityStateIcon;
	const std::string &townIconFilename = ArenaTextureName::TownIcon;
	const std::string &villageIconFilename = ArenaTextureName::VillageIcon;
	const std::string &dungeonIconFilename = ArenaTextureName::DungeonIcon;
	const std::optional<TextureBuilderID> cityStateIconTextureBuilderID =
		textureManager.tryGetTextureBuilderID(cityStateIconFilename.c_str());
	const std::optional<TextureBuilderID> townIconTextureBuilderID =
		textureManager.tryGetTextureBuilderID(townIconFilename.c_str());
	const std::optional<TextureBuilderID> villageIconTextureBuilderID =
		textureManager.tryGetTextureBuilderID(villageIconFilename.c_str());
	const std::optional<TextureBuilderID> dungeonIconTextureBuilderID =
		textureManager.tryGetTextureBuilderID(dungeonIconFilename.c_str());
	DebugAssert(cityStateIconTextureBuilderID.has_value());
	DebugAssert(townIconTextureBuilderID.has_value());
	DebugAssert(villageIconTextureBuilderID.has_value());
	DebugAssert(dungeonIconTextureBuilderID.has_value());

	auto &game = this->getGame();
	auto &gameData = game.getGameData();
	const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
	const WorldMapInstance &worldMapInst = gameData.getWorldMapInstance();
	const ProvinceInstance &provinceInst = worldMapInst.getProvinceInstance(this->provinceID);
	const int provinceDefIndex = provinceInst.getProvinceDefIndex();
	const WorldMapDefinition &worldMapDef = gameData.getWorldMapDefinition();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceDefIndex);

	// Gets the displayed icon texture ID for a location.
	auto getLocationIconTextureID = [this, &backgroundFilename, &textureManager, &renderer,
		&cityStateIconTextureBuilderID, &townIconTextureBuilderID, &villageIconTextureBuilderID,
		&dungeonIconTextureBuilderID](const LocationDefinition &locationDef) -> TextureBuilderID
	{
		switch (locationDef.getType())
		{
		case LocationDefinition::Type::City:
		{
			switch (locationDef.getCityDefinition().type)
			{
			case ArenaTypes::CityType::CityState:
				return *cityStateIconTextureBuilderID;
			case ArenaTypes::CityType::Town:
				return *townIconTextureBuilderID;
			case ArenaTypes::CityType::Village:
				return *villageIconTextureBuilderID;
			default:
				throw DebugException(std::to_string(
					static_cast<int>(locationDef.getCityDefinition().type)));
			}
		}
		case LocationDefinition::Type::Dungeon:
			return *dungeonIconTextureBuilderID;
		case LocationDefinition::Type::MainQuestDungeon:
		{
			const LocationDefinition::MainQuestDungeonDefinition::Type mainQuestDungeonType =
				locationDef.getMainQuestDungeonDefinition().type;

			if (mainQuestDungeonType == LocationDefinition::MainQuestDungeonDefinition::Type::Staff)
			{
				const std::string &staffDungeonIconFilename = ArenaTextureName::StaffDungeonIcons;
				const std::optional<TextureBuilderIdGroup> staffDungeonIconTextureBuilderIDs =
					textureManager.tryGetTextureBuilderIDs(staffDungeonIconFilename.c_str());
				if (!staffDungeonIconTextureBuilderIDs.has_value())
				{
					DebugCrash("Couldn't get staff dungeon icon texture builder IDs for \"" + staffDungeonIconFilename + "\".");
				}

				return staffDungeonIconTextureBuilderIDs->getID(this->provinceID);
			}
			else
			{
				return *dungeonIconTextureBuilderID;
			}
		}
		default:
			throw DebugException(std::to_string(static_cast<int>(locationDef.getType())));
		}
	};

	auto drawIconIfVisible = [this, &textureManager, &renderer, &backgroundPaletteID, &provinceDef,
		&getLocationIconTextureID](const LocationInstance &locationInst)
	{
		if (locationInst.isVisible())
		{
			const int locationDefIndex = locationInst.getLocationDefIndex();
			const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);
			const Int2 point(locationDef.getScreenX(), locationDef.getScreenY());
			const TextureBuilderID iconTextureBuilderID = getLocationIconTextureID(locationDef);
			this->drawCenteredIcon(iconTextureBuilderID, *backgroundPaletteID, point, renderer);
		}
	};

	// Draw all visible location icons in the province.
	for (int i = 0; i < provinceInst.getLocationCount(); i++)
	{
		const LocationInstance &locationInst = provinceInst.getLocationInstance(i);
		drawIconIfVisible(locationInst);
	}
}

void ProvinceMapPanel::drawLocationHighlight(const LocationDefinition &locationDef,
	LocationHighlightType highlightType, const std::string &backgroundFilename,
	TextureManager &textureManager, Renderer &renderer)
{
	const std::string &highlightPaletteFilename = backgroundFilename;
	const std::optional<PaletteID> highlightPaletteID =
		textureManager.tryGetPaletteID(highlightPaletteFilename.c_str());
	if (!highlightPaletteID.has_value())
	{
		DebugLogError("Couldn't get highlight palette ID for \"" + highlightPaletteFilename + "\".");
		return;
	}

	auto drawHighlightTextureBuilderID = [this, &locationDef, &renderer, &highlightPaletteID](
		TextureBuilderID highlightTextureBuilderID)
	{
		const Int2 point(locationDef.getScreenX(), locationDef.getScreenY());
		this->drawCenteredIcon(highlightTextureBuilderID, *highlightPaletteID, point, renderer);
	};

	auto drawHighlightTexture = [this, &locationDef, &renderer](const Texture &highlight)
	{
		const Int2 point(locationDef.getScreenX(), locationDef.getScreenY());
		this->drawCenteredIcon(highlight, point, renderer);
	};

	// Generic highlights (city, town, village, and dungeon).
	const std::string &outlinesFilename = (highlightType == ProvinceMapPanel::LocationHighlightType::Current) ?
		ArenaTextureName::MapIconOutlines : ArenaTextureName::MapIconOutlinesBlinking;

	const std::optional<TextureBuilderIdGroup> highlightTextureBuilderIDs =
		textureManager.tryGetTextureBuilderIDs(outlinesFilename.c_str());
	if (!highlightTextureBuilderIDs.has_value())
	{
		DebugLogError("Couldn't get highlight texture builder IDs for \"" + outlinesFilename + "\".");
		return;
	}

	auto handleCityHighlight = [&textureManager, &renderer, &locationDef, &drawHighlightTextureBuilderID,
		&highlightTextureBuilderIDs]()
	{
		const int highlightIndex = [&locationDef, &highlightTextureBuilderIDs]()
		{
			const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();

			switch (cityDef.type)
			{
			case ArenaTypes::CityType::CityState:
				return 0;
			case ArenaTypes::CityType::Town:
				return 1;
			case ArenaTypes::CityType::Village:
				return 2;
			default:
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(cityDef.type)));
			}
		}();

		const TextureBuilderID highlightTextureBuilderID = highlightTextureBuilderIDs->getID(highlightIndex);
		drawHighlightTextureBuilderID(highlightTextureBuilderID);
	};

	auto handleDungeonHighlight = [this, &textureManager, &drawHighlightTextureBuilderID,
		&highlightTextureBuilderIDs]()
	{
		// Named dungeon (they all use the same icon).
		constexpr int highlightIndex = 3;
		const TextureBuilderID highlightTextureBuilderID = highlightTextureBuilderIDs->getID(highlightIndex);
		drawHighlightTextureBuilderID(highlightTextureBuilderID);
	};

	auto handleMainQuestDungeonHighlight = [this, &locationDef, highlightType, &backgroundFilename, &textureManager,
		&renderer, &drawHighlightTextureBuilderID, &drawHighlightTexture, &highlightTextureBuilderIDs]()
	{
		const LocationDefinition::MainQuestDungeonDefinition &mainQuestDungeonDef =
			locationDef.getMainQuestDungeonDefinition();

		if (mainQuestDungeonDef.type == LocationDefinition::MainQuestDungeonDefinition::Type::Start)
		{
			// Start dungeon is not drawn.
		}
		else if (mainQuestDungeonDef.type == LocationDefinition::MainQuestDungeonDefinition::Type::Map)
		{
			// Staff map dungeon.
			constexpr int highlightIndex = 3;
			const TextureBuilderID highlightTextureBuilderID = highlightTextureBuilderIDs->getID(highlightIndex);
			drawHighlightTextureBuilderID(highlightTextureBuilderID);
		}
		else if (mainQuestDungeonDef.type == LocationDefinition::MainQuestDungeonDefinition::Type::Staff)
		{
			// Staff dungeon. It changes a value in the palette to give the icon's background
			// its yellow color (since there are no highlight icons for staff dungeons).
			const Texture highlightTexture = [this, highlightType, &renderer]()
			{
				// Get the palette indices associated with the staff dungeon icon.
				const CIFFile &iconCif = this->staffDungeonCif;
				const int cifWidth = iconCif.getWidth(this->provinceID);
				const int cifHeight = iconCif.getHeight(this->provinceID);

				// Make a copy of the staff dungeon icon with changes based on which
				// pixels should be highlighted.
				Surface surface = Surface::createWithFormat(cifWidth, cifHeight,
					Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);

				auto getColorFromIndex = [this, highlightType, &surface](int paletteIndex)
				{
					const auto &palette = this->provinceMapPalette;
					DebugAssertIndex(palette, paletteIndex);
					const Color &color = palette[paletteIndex];
					return surface.mapRGBA(color.r, color.g, color.b, color.a);
				};

				const uint32_t highlightColor = getColorFromIndex(
					(highlightType == ProvinceMapPanel::LocationHighlightType::Current) ?
					YellowPaletteIndex : RedPaletteIndex);

				// Convert each palette index to its equivalent 32-bit color, changing 
				// background indices to highlight indices as they are found.
				const uint8_t *srcPixels = iconCif.getPixels(this->provinceID);
				uint32_t *dstPixels = static_cast<uint32_t*>(surface.get()->pixels);
				const int pixelCount = surface.getWidth() * surface.getHeight();
				std::transform(srcPixels, srcPixels + pixelCount, dstPixels,
					[&getColorFromIndex, highlightColor](uint8_t srcPixel)
				{
					return (srcPixel == BackgroundPaletteIndex) ?
						highlightColor : getColorFromIndex(srcPixel);
				});

				Texture texture = renderer.createTextureFromSurface(surface);
				return texture;
			}();

			drawHighlightTexture(highlightTexture);
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(mainQuestDungeonDef.type)));
		}
	};

	// Decide how to highlight the location.
	switch (locationDef.getType())
	{
	case LocationDefinition::Type::City:
		handleCityHighlight();
		break;
	case LocationDefinition::Type::Dungeon:
		handleDungeonHighlight();
		break;
	case LocationDefinition::Type::MainQuestDungeon:
		handleMainQuestDungeonHighlight();
		break;
	default:
		DebugNotImplementedMsg(std::to_string(static_cast<int>(locationDef.getType())));
	}
}

void ProvinceMapPanel::drawLocationName(int locationID, Renderer &renderer)
{
	auto &game = this->getGame();
	auto &gameData = game.getGameData();
	const WorldMapInstance &worldMapInst = gameData.getWorldMapInstance();
	const ProvinceInstance &provinceInst = worldMapInst.getProvinceInstance(this->provinceID);
	const int provinceDefIndex = provinceInst.getProvinceDefIndex();
	const WorldMapDefinition &worldMapDef = gameData.getWorldMapDefinition();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceDefIndex);
	const LocationInstance &locationInst = provinceInst.getLocationInstance(locationID);
	const int locationDefIndex = locationInst.getLocationDefIndex();
	const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);

	const Int2 center(locationDef.getScreenX(), locationDef.getScreenY());
	const std::string &locationName = locationInst.getName(locationDef);

	const auto &fontLibrary = game.getFontLibrary();
	const RichTextString richText(
		locationName,
		FontName::Arena,
		Color(158, 0, 0),
		TextAlignment::Center,
		fontLibrary);

	const TextBox::ShadowData shadowData(Color(48, 48, 48), Int2(1, 0));
	const TextBox textBox(center - Int2(0, 10), richText, &shadowData, fontLibrary, renderer);
	const Surface &textBoxSurface = textBox.getSurface();

	// Clamp to screen edges, with some extra space on the left and right.
	const int x = std::clamp(textBox.getX(),
		2, ArenaRenderUtils::SCREEN_WIDTH - textBoxSurface.getWidth() - 2);
	const int y = std::clamp(textBox.getY(),
		2, ArenaRenderUtils::SCREEN_HEIGHT - textBoxSurface.getHeight() - 2);

	renderer.drawOriginal(textBox.getTexture(), x, y);
}

void ProvinceMapPanel::drawButtonTooltip(ProvinceButtonName buttonName, Renderer &renderer)
{
	const std::string &text = ProvinceButtonTooltips.at(buttonName);

	const Texture tooltip = Panel::createTooltip(
		text, FontName::D, this->getGame().getFontLibrary(), renderer);

	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);
	const int mouseX = originalPosition.x;
	const int mouseY = originalPosition.y;
	const int x = ((mouseX + 8 + tooltip.getWidth()) < ArenaRenderUtils::SCREEN_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < ArenaRenderUtils::SCREEN_HEIGHT) ?
		mouseY : (mouseY - tooltip.getHeight());

	renderer.drawOriginal(tooltip, x, y);
}

void ProvinceMapPanel::render(Renderer &renderer)
{
	DebugAssert(this->getGame().gameDataIsActive());

	// Clear full screen.
	renderer.clear();

	// Draw province map background.
	auto &textureManager = this->getGame().getTextureManager();
	const std::string backgroundFilename = this->getBackgroundFilename();
	const std::string &backgroundPaletteName = backgroundFilename;
	const std::optional<PaletteID> backgroundPaletteID = textureManager.tryGetPaletteID(backgroundPaletteName.c_str());
	if (!backgroundPaletteID.has_value())
	{
		DebugLogError("Couldn't get background palette ID for \"" + backgroundPaletteName + "\".");
		return;
	}

	const std::optional<TextureBuilderID> mapBackgroundTextureBuilderID =
		textureManager.tryGetTextureBuilderID(backgroundFilename.c_str());
	if (!mapBackgroundTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get map background texture builder ID for \"" + backgroundFilename + "\".");
		return;
	}

	renderer.drawOriginal(*mapBackgroundTextureBuilderID, *backgroundPaletteID, textureManager);

	// Draw visible location icons.
	this->drawVisibleLocations(backgroundFilename, textureManager, renderer);

	const auto &gameData = this->getGame().getGameData();
	const WorldMapDefinition &worldMapDef = gameData.getWorldMapDefinition();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(this->provinceID);
	const ProvinceDefinition &playerProvinceDef = gameData.getProvinceDefinition();

	// If the player is in the current province, highlight their current location.
	if (provinceDef.matches(playerProvinceDef))
	{
		const LocationDefinition &locationDef = gameData.getLocationDefinition();
		const auto highlightType = ProvinceMapPanel::LocationHighlightType::Current;
		this->drawLocationHighlight(locationDef, highlightType, backgroundFilename,
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
			const LocationDefinition &selectedLocationDef =
				provinceDef.getLocationDef(this->travelData->locationID);
			const auto highlightType = ProvinceMapPanel::LocationHighlightType::Selected;

			this->drawLocationHighlight(selectedLocationDef, highlightType,
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
