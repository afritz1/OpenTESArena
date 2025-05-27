#include <algorithm>
#include <sstream>

#include "TextSubPanel.h"
#include "WorldMapUiController.h"
#include "WorldMapUiModel.h"
#include "WorldMapUiView.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Game/Game.h"
#include "../Math/Random.h"
#include "../Time/ArenaDateUtils.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextBox.h"
#include "../WorldMap/ArenaLocationUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

std::string WorldMapUiModel::getProvinceNameOffsetFilename()
{
	return "OUTPROV.CIF";
}

const WorldMapMask &WorldMapUiModel::getMask(const Game &game, int maskID)
{
	const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const auto &worldMapMasks = binaryAssetLibrary.getWorldMapMasks();

	DebugAssertIndex(worldMapMasks, maskID);
	return worldMapMasks[maskID];
}

std::optional<int> WorldMapUiModel::getMaskID(Game &game, const Int2 &mousePosition, bool ignoreCenterProvince,
	bool ignoreExitButton)
{
	const Int2 classicPosition = game.renderer.nativeToOriginal(mousePosition);
	const auto &worldMapMasks = BinaryAssetLibrary::getInstance().getWorldMapMasks();
	const int maskCount = static_cast<int>(worldMapMasks.size());
	for (int maskID = 0; maskID < maskCount; maskID++)
	{
		constexpr int centerProvinceID = ArenaLocationUtils::CENTER_PROVINCE_ID;
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

void FastTravelUiModel::tickTravelTime(Game &game, int travelDays)
{
	DebugAssert(travelDays >= 0);

	auto &gameState = game.gameState;
	Random &random = game.random;

	Date &date = gameState.getDate();
	for (int i = 0; i < travelDays; i++)
	{
		date.incrementDay();
	}

	Clock &clock = gameState.getClock();
	const int randomHours = random.next(23);
	for (int i = 0; i < randomHours; i++)
	{
		clock.incrementHour();

		if (clock.hours == 0)
		{
			date.incrementDay();
		}
	}
}

std::string FastTravelUiModel::getCityArrivalMessage(Game &game, int targetProvinceID,
	int targetLocationID, int travelDays)
{
	auto &gameState = game.gameState;
	const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
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
		if (provinceID != ArenaLocationUtils::CENTER_PROVINCE_ID)
		{
			// The <city type> of <city name> in <province> Province.
			// Replace first %s with location type.
			const std::string &locationTypeName = [&exeData, localCityID]()
			{
				int locationTypeIndex = -1;
				if (localCityID < 8)
				{
					// City.
					locationTypeIndex = 0;
				}
				else if (localCityID < 16)
				{
					// Town.
					locationTypeIndex = 1;
				}
				else
				{
					// Village.
					locationTypeIndex = 2;
				}

				return exeData.locations.locationTypes[locationTypeIndex];
			}();

			std::string text = exeData.travel.locationFormatTexts[2];

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

	const std::string locationDescriptionString = [&game, &gameState, &binaryAssetLibrary,
		&exeData, provinceID, localCityID, &locationDef]()
	{
		const ArenaTypes::LocationType locationType = ArenaLocationUtils::getCityType(localCityID);

		// Get the description for the local location. If it's a town or village, choose
		// one of the three substrings randomly. Otherwise, get the city description text
		// directly.
		const std::string description = [&game, &binaryAssetLibrary, &exeData, provinceID, localCityID, &locationDef, locationType]()
		{
			// City descriptions start at #0600. The three town descriptions are at #1422,
			// and the three village descriptions are at #1423.
			const int templateDatEntryKey = [provinceID, localCityID, locationType]()
			{
				if (locationType == ArenaTypes::LocationType::CityState)
				{
					return 600 + localCityID + (8 * provinceID);
				}
				else if (locationType == ArenaTypes::LocationType::Town)
				{
					return 1422;
				}
				else if (locationType == ArenaTypes::LocationType::Village)
				{
					return 1423;
				}
				else
				{
					DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(locationType)));
				}
			}();

			const TextAssetLibrary &textAssetLibrary = TextAssetLibrary::getInstance();
			const ArenaTemplateDat &templateDat = textAssetLibrary.templateDat;
			const ArenaTemplateDatEntry &entry = templateDat.getEntry(templateDatEntryKey);
			const BufferView<const std::string> templateDatTexts = entry.values;

			if (locationType == ArenaTypes::LocationType::CityState)
			{
				return templateDatTexts[0];
			}
			else
			{
				ArenaRandom &random = game.arenaRandom;
				std::string description = [&random, &templateDatTexts]()
				{
					const int templateDatTextIndex = random.next() % templateDatTexts.getCount();
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
					return ArenaLocationUtils::getRulerSeed(localPoint, province.getGlobalRect());
				}();

				const bool isMale = ArenaLocationUtils::isRulerMale(localCityID, province);

				// Replace %t with ruler title (if it exists).
				random.srand(rulerSeed);
				index = description.find("%t");
				if (index != std::string::npos)
				{
					const std::string &rulerTitle = binaryAssetLibrary.getRulerTitle(provinceID, locationType, isMale, random);
					description.replace(index, 2, rulerTitle);
				}

				// Replace %rf with ruler first name (if it exists). Make sure to reset the random seed.
				random.srand(rulerSeed);
				index = description.find("%rf");
				if (index != std::string::npos)
				{
					const std::string rulerFirstName = [provinceID, &random, isMale]()
					{
						const auto &textAssetLibrary = TextAssetLibrary::getInstance();
						const std::string fullName = textAssetLibrary.generateNpcName(provinceID, isMale, random);
						const Buffer<std::string> tokens = String::split(fullName, ' ');
						return tokens[0];
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

std::unique_ptr<Panel> FastTravelUiModel::makeCityArrivalPopUp(Game &game, int targetProvinceID,
	int targetLocationID, int travelDays)
{
	const std::string text = FastTravelUiModel::getCityArrivalMessage(game, targetProvinceID, targetLocationID, travelDays);
	const TextBoxInitInfo textBoxInitInfo = TextBoxInitInfo::makeWithCenter(
		text,
		FastTravelUiView::getCityArrivalPopUpTextCenterPoint(game),
		FastTravelUiView::CityArrivalFontName,
		FastTravelUiView::CityArrivalTextColor,
		FastTravelUiView::CityArrivalTextAlignment,
		std::nullopt,
		FastTravelUiView::CityArrivalLineSpacing,
		FontLibrary::getInstance());

	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;
	const UiTextureID textureID = FastTravelUiView::allocCityArrivalPopUpTexture(
		textBoxInitInfo.rect.width, textBoxInitInfo.rect.height, textureManager, renderer);
	ScopedUiTextureRef textureRef(textureID, renderer);

	std::unique_ptr<TextSubPanel> subPanel = std::make_unique<TextSubPanel>(game);
	if (!subPanel->init(textBoxInitInfo, text, FastTravelUiController::onCityArrivalPopUpSelected,
		std::move(textureRef), FastTravelUiView::getCityArrivalPopUpTextureCenterPoint(game)))
	{
		DebugCrash("Couldn't init city arrival sub-panel.");
	}

	return subPanel;
}
