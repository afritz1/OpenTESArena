#include <algorithm>
#include <sstream>

#include "TextSubPanel.h"
#include "WorldMapUiController.h"
#include "WorldMapUiModel.h"
#include "WorldMapUiView.h"
#include "../Game/ArenaDateUtils.h"
#include "../Game/Game.h"
#include "../Math/Random.h"
#include "../UI/Surface.h"
#include "../UI/TextBox.h"
#include "../WorldMap/LocationType.h"
#include "../WorldMap/LocationUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

std::string WorldMapUiModel::getProvinceNameOffsetFilename()
{
	return "OUTPROV.CIF";
}

const WorldMapMask &WorldMapUiModel::getMask(const Game &game, int maskID)
{
	const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
	const auto &worldMapMasks = binaryAssetLibrary.getWorldMapMasks();

	DebugAssertIndex(worldMapMasks, maskID);
	return worldMapMasks[maskID];
}

std::optional<int> WorldMapUiModel::getMaskID(Game &game, const Int2 &mousePosition, bool ignoreCenterProvince,
	bool ignoreExitButton)
{
	const Int2 classicPosition = game.getRenderer().nativeToOriginal(mousePosition);
	const auto &worldMapMasks = game.getBinaryAssetLibrary().getWorldMapMasks();
	const int maskCount = static_cast<int>(worldMapMasks.size());
	for (int maskID = 0; maskID < maskCount; maskID++)
	{
		constexpr int centerProvinceID = LocationUtils::CENTER_PROVINCE_ID;
		constexpr int exitButtonID = WorldMapUiModel::EXIT_BUTTON_MASK_ID;
		if ((ignoreCenterProvince && (maskID == centerProvinceID)) ||
			(ignoreExitButton && (maskID == exitButtonID)))
		{
			continue;
		}

		DebugAssertIndex(worldMapMasks, maskID);
		const WorldMapMask &mapMask = worldMapMasks[maskID];
		const Rect &maskRect = mapMask.getRect();

		if (maskRect.contains(classicPosition))
		{
			// See if the pixel is set in the bitmask.
			const bool success = mapMask.get(classicPosition.x, classicPosition.y);

			if (success)
			{
				return maskID;
			}
		}
	}

	// No province/button found at the given pixel.
	return std::nullopt;
}

void WorldMapUiModel::tickTravelTime(Game &game, int travelDays)
{
	auto &gameState = game.getGameState();
	auto &random = game.getRandom();

	// Tick the game date by the number of travel days.
	auto &date = gameState.getDate();
	for (int i = 0; i < travelDays; i++)
	{
		date.incrementDay();
	}

	// Add between 0 and 22 random hours to the clock time.
	auto &clock = gameState.getClock();
	const int randomHours = random.next(23);
	for (int i = 0; i < randomHours; i++)
	{
		clock.incrementHour();

		// Increment day if the clock loops around.
		if (clock.getHours24() == 0)
		{
			date.incrementDay();
		}
	}
}

std::string WorldMapUiModel::getCityArrivalMessage(Game &game, int targetProvinceID,
	int targetLocationID, int travelDays)
{
	auto &gameState = game.getGameState();
	const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
	const auto &exeData = binaryAssetLibrary.getExeData();

	// @todo: change these to 'index' so it's clear they don't rely on original game's 0-32, etc..
	const int provinceID = targetProvinceID;
	const int localCityID = targetLocationID;

	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceID);
	const LocationDefinition &locationDef = provinceDef.getLocationDef(localCityID);

	const std::string locationString = [travelDays, &gameState, &exeData, provinceID,
		localCityID, &provinceDef, &locationDef]()
	{
		if (provinceID != LocationUtils::CENTER_PROVINCE_ID)
		{
			// The <city type> of <city name> in <province> Province.
			// Replace first %s with location type.
			const std::string &locationTypeName = [&exeData, localCityID]()
			{
				if (localCityID < 8)
				{
					// City.
					return exeData.locations.locationTypes.front();
				}
				else if (localCityID < 16)
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

			std::string text = exeData.travel.locationFormatTexts.at(2);

			// Replace first %s with location type name.
			size_t index = text.find("%s");
			text.replace(index, 2, locationTypeName);

			// Replace second %s with location name.
			index = text.find("%s", index);
			text.replace(index, 2, locationDef.getName());

			// Replace third %s with province name.
			index = text.find("%s", index);
			text.replace(index, 2, provinceDef.getName());

			return exeData.travel.arrivalPopUpLocation + text;
		}
		else
		{
			// Center province displays only the city name.
			return exeData.travel.arrivalPopUpLocation + exeData.travel.arrivalCenterProvinceLocation;
		}
	}();

	const std::string dateString = exeData.travel.arrivalPopUpDate +
		ArenaDateUtils::makeDateString(gameState.getDate(), exeData);

	const std::string daysString = [travelDays, &exeData]()
	{
		std::string text = exeData.travel.arrivalPopUpDays;

		// Replace %d with travel days.
		size_t index = text.find("%d");
		text.replace(index, 2, std::to_string(travelDays));

		return text;
	}();

	const auto &textAssetLibrary = game.getTextAssetLibrary();
	const std::string locationDescriptionString = [&game, &gameState, &binaryAssetLibrary,
		&textAssetLibrary, &exeData, provinceID, localCityID, &locationDef]()
	{
		const LocationType locationType = LocationUtils::getCityType(localCityID);

		// Get the description for the local location. If it's a town or village, choose
		// one of the three substrings randomly. Otherwise, get the city description text
		// directly.
		const std::string description = [&gameState, &binaryAssetLibrary, &textAssetLibrary,
			&exeData, provinceID, localCityID, &locationDef, locationType]()
		{
			// City descriptions start at #0600. The three town descriptions are at #1422,
			// and the three village descriptions are at #1423.
			const std::vector<std::string> &templateDatTexts = [&binaryAssetLibrary, &textAssetLibrary,
				provinceID, localCityID, locationType]()
			{
				// Get the key that maps into TEMPLATE.DAT.
				const int key = [provinceID, localCityID, locationType]()
				{
					if (locationType == LocationType::CityState)
					{
						return 600 + localCityID + (8 * provinceID);
					}
					else if (locationType == LocationType::Town)
					{
						return 1422;
					}
					else if (locationType == LocationType::Village)
					{
						return 1423;
					}
					else
					{
						DebugUnhandledReturnMsg(int,
							std::to_string(static_cast<int>(locationType)));
					}
				}();

				const auto &templateDat = textAssetLibrary.getTemplateDat();
				const auto &entry = templateDat.getEntry(key);
				return entry.values;
			}();

			if (locationType == LocationType::CityState)
			{
				return templateDatTexts.front();
			}
			else
			{
				ArenaRandom &random = gameState.getRandom();
				std::string description = [&random, &templateDatTexts]()
				{
					const int templateDatTextIndex = random.next() % templateDatTexts.size();
					DebugAssertIndex(templateDatTexts, templateDatTextIndex);
					return templateDatTexts[templateDatTextIndex];
				}();

				// Replace %cn with city name.
				size_t index = description.find("%cn");
				description.replace(index, 3, locationDef.getName());

				const auto &cityData = binaryAssetLibrary.getCityDataFile();
				const auto &province = cityData.getProvinceData(provinceID);
				const uint32_t rulerSeed = [localCityID, &cityData, &province]()
				{
					const auto &location = province.getLocationData(localCityID);
					const Int2 localPoint(location.x, location.y);
					return LocationUtils::getRulerSeed(localPoint, province.getGlobalRect());
				}();

				const bool isMale = LocationUtils::isRulerMale(localCityID, province);

				// Replace %t with ruler title (if it exists).
				random.srand(rulerSeed);
				index = description.find("%t");
				if (index != std::string::npos)
				{
					const std::string &rulerTitle = binaryAssetLibrary.getRulerTitle(
						provinceID, locationType, isMale, random);
					description.replace(index, 2, rulerTitle);
				}

				// Replace %rf with ruler first name (if it exists). Make sure to reset
				// the random seed.
				random.srand(rulerSeed);
				index = description.find("%rf");
				if (index != std::string::npos)
				{
					const std::string rulerFirstName = [&textAssetLibrary, provinceID,
						&random, isMale]()
					{
						const std::string fullName = textAssetLibrary.generateNpcName(provinceID, isMale, random);
						const std::vector<std::string> tokens = String::split(fullName, ' ');
						return tokens.front();
					}();

					description.replace(index, 3, rulerFirstName);
				}

				return description;
			}
		}();

		return description;
	}();

	std::string fullText = locationString + dateString + daysString + locationDescriptionString;

	// Replace all line breaks with spaces and compress spaces into one.
	std::string trimmedText = [&fullText]()
	{
		std::string str;
		char prev = -1;
		for (char c : fullText)
		{
			if (c == '\r')
			{
				c = ' ';
			}

			if (prev != ' ' || c != ' ')
			{
				str += c;
			}

			prev = c;
		}

		return str;
	}();

	// Re-distribute newlines.
	fullText = String::distributeNewlines(trimmedText, 50);

	return fullText;
}

std::unique_ptr<Panel> WorldMapUiModel::makeCityArrivalPopUp(Game &game, int targetProvinceID,
	int targetLocationID, int travelDays)
{
	const std::string text = WorldMapUiModel::getCityArrivalMessage(game, targetProvinceID, targetLocationID, travelDays);
	const TextBox::InitInfo textBoxInitInfo = TextBox::InitInfo::makeWithCenter(
		text,
		WorldMapUiView::getCityArrivalPopUpTextCenterPoint(game),
		WorldMapUiView::CityArrivalFontName,
		WorldMapUiView::CityArrivalTextColor,
		WorldMapUiView::CityArrivalTextAlignment,
		std::nullopt,
		WorldMapUiView::CityArrivalLineSpacing,
		game.getFontLibrary());

	auto &renderer = game.getRenderer();
	const Surface surface = TextureUtils::generate(
		WorldMapUiView::CityArrivalTexturePatternType,
		WorldMapUiView::getCityArrivalPopUpTextureWidth(textBoxInitInfo.rect.getWidth()),
		WorldMapUiView::getCityArrivalPopUpTextureHeight(textBoxInitInfo.rect.getHeight()),
		game.getTextureManager(),
		renderer);
	Texture texture = renderer.createTextureFromSurface(surface);

	std::unique_ptr<TextSubPanel> subPanel = std::make_unique<TextSubPanel>(game);
	if (!subPanel->init(textBoxInitInfo, text, WorldMapUiController::onFastTravelCityArrivalPopUpSelected,
		std::move(texture), WorldMapUiView::getCityArrivalPopUpTextureCenterPoint(game)))
	{
		DebugCrash("Couldn't init sub-panel.");
	}

	return subPanel;
}
