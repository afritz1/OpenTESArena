#include <algorithm>

#include "ProvinceMapUiController.h"
#include "ProvinceMapUiModel.h"
#include "ProvinceMapUiView.h"
#include "TextSubPanel.h"
#include "../Game/Game.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextBox.h"
#include "../UI/TextRenderUtils.h"
#include "../WorldMap/ArenaLocationUtils.h"

#include "components/utilities/String.h"

ProvinceMapUiModel::TravelData::TravelData(int locationID, int provinceID, int travelDays)
{
	this->locationID = locationID;
	this->provinceID = provinceID;
	this->travelDays = travelDays;
}

std::string ProvinceMapUiModel::makeAlreadyAtLocationText(Game &game, const std::string &locationName)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	std::string text = exeData.travel.alreadyAtDestination;

	// Remove carriage return at end.
	text.pop_back();

	// Replace carriage returns with newlines.
	text = String::replace(text, '\r', '\n');

	// Replace %s with location name.
	size_t index = text.find("%s");
	text.replace(index, 2, locationName);

	return text;
}

std::string ProvinceMapUiModel::getLocationName(Game &game, int provinceID, int locationID)
{
	auto &gameState = game.gameState;
	const WorldMapInstance &worldMapInst = gameState.getWorldMapInstance();
	const ProvinceInstance &provinceInst = worldMapInst.getProvinceInstance(provinceID);
	const int provinceDefIndex = provinceInst.getProvinceDefIndex();
	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceDefIndex);
	const LocationInstance &locationInst = provinceInst.getLocationInstance(locationID);
	const int locationDefIndex = locationInst.getLocationDefIndex();
	const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);
	return locationInst.getName(locationDef);
}

std::unique_ptr<Panel> ProvinceMapUiModel::makeTextPopUp(Game &game, const std::string &text)
{
	auto &renderer = game.renderer;
	const auto &fontLibrary = FontLibrary::getInstance();

	const std::string &fontName = ProvinceMapUiView::TextPopUpFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	constexpr int lineSpacing = ProvinceMapUiView::TextPopUpLineSpacing;
	const TextRenderUtils::TextureGenInfo textBoxTextureGenInfo =
		TextRenderUtils::makeTextureGenInfo(text, fontDef, std::nullopt, lineSpacing);
	const TextBox::InitInfo textBoxInitInfo = TextBox::InitInfo::makeWithCenter(
		text,
		ProvinceMapUiView::TextPopUpCenterPoint,
		fontName,
		ProvinceMapUiView::TextPopUpTextColor,
		ProvinceMapUiView::TextPopUpTextAlignment,
		std::nullopt,
		lineSpacing,
		fontLibrary);

	auto &textureManager = game.textureManager;
	const UiTextureID textureID = ProvinceMapUiView::allocTextPopUpTexture(
		textBoxTextureGenInfo.width, textBoxTextureGenInfo.height, textureManager, renderer);
	ScopedUiTextureRef textureRef(textureID, renderer);

	std::unique_ptr<TextSubPanel> subPanel = std::make_unique<TextSubPanel>(game);
	if (!subPanel->init(textBoxInitInfo, text, ProvinceMapUiController::onTextPopUpSelected,
		std::move(textureRef), ProvinceMapUiView::TextPopUpTextureCenterPoint))
	{
		DebugCrash("Couldn't init province map text sub-panel.");
	}

	return subPanel;
}

std::string ProvinceMapUiModel::makeTravelText(Game &game, int srcProvinceIndex, const LocationDefinition &srcLocationDef,
	const ProvinceDefinition &srcProvinceDef, int dstLocationIndex)
{
	auto &gameState = game.gameState;
	const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const auto &exeData = binaryAssetLibrary.getExeData();
	const WorldMapInstance &worldMapInst = gameState.getWorldMapInstance();
	const ProvinceInstance &dstProvinceInst = worldMapInst.getProvinceInstance(srcProvinceIndex);
	const LocationInstance &dstLocationInst = dstProvinceInst.getLocationInstance(dstLocationIndex);

	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
	const int dstProvinceDefIndex = dstProvinceInst.getProvinceDefIndex();
	const ProvinceDefinition &dstProvinceDef = worldMapDef.getProvinceDef(dstProvinceDefIndex);
	const int dstLocationDefIndex = dstLocationInst.getLocationDefIndex();
	const LocationDefinition &dstLocationDef = dstProvinceDef.getLocationDef(dstLocationDefIndex);
	const std::string &dstLocationName = dstLocationInst.getName(dstLocationDef);

	const Date &currentDate = gameState.getDate();
	const ProvinceMapUiModel::TravelData *travelDataPtr = gameState.getTravelData();
	DebugAssert(travelDataPtr != nullptr);
	const ProvinceMapUiModel::TravelData &travelData = *travelDataPtr;

	const Date destinationDate = [&currentDate, &travelData]()
	{
		Date newDate = currentDate;
		for (int i = 0; i < travelData.travelDays; i++)
		{
			newDate.incrementDay();
		}

		return newDate;
	}();

	const std::string locationFormatText = [srcProvinceIndex, dstLocationIndex, &exeData, &dstProvinceDef,
		&dstLocationDef, &dstLocationName]()
	{
		std::string formatText = [srcProvinceIndex, dstLocationIndex, &exeData, &dstProvinceDef,
			&dstLocationDef, &dstLocationName]()
		{
			const auto &locationFormatTexts = exeData.travel.locationFormatTexts;

			// Decide the format based on whether it's the center province.
			if (srcProvinceIndex != ArenaLocationUtils::CENTER_PROVINCE_ID)
			{
				// Determine whether to use the city format or dungeon format.
				if (dstLocationDef.getType() == LocationDefinitionType::City)
				{
					// City format.
					const int locationFormatTextsIndex = 2;
					DebugAssertIndex(locationFormatTexts, locationFormatTextsIndex);
					std::string text = locationFormatTexts[locationFormatTextsIndex];

					// Replace first %s with location type.
					const LocationCityDefinition &cityDef = dstLocationDef.getCityDefinition();
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
				else if ((dstLocationDef.getType() == LocationDefinitionType::Dungeon) ||
					(dstLocationDef.getType() == LocationDefinitionType::MainQuestDungeon))
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
		const int weekday = date.getWeekday();
		DebugAssertIndex(exeData.calendar.weekdayNames, weekday);
		const std::string &weekdayString = exeData.calendar.weekdayNames[weekday];
		size_t index = text.find("%s");
		text.replace(index, 2, weekdayString);

		// Replace %u%s with day and ordinal suffix.
		const std::string dayString = date.getOrdinalDay();
		index = text.find("%u%s");
		text.replace(index, 4, dayString);

		// Replace third %s with month.
		const int month = date.getMonth();
		DebugAssertIndex(exeData.calendar.monthNames, month);
		const std::string &monthString = exeData.calendar.monthNames[month];
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
		const std::string &dayStringPrefix = exeData.travel.dayPrediction[0];
		const std::string dayStringBody = [&exeData, &travelData]()
		{
			const int dayPredictionIndex = std::size(exeData.travel.dayPrediction) - 1;
			DebugAssertIndex(exeData.travel.dayPrediction, dayPredictionIndex);
			std::string text = exeData.travel.dayPrediction[dayPredictionIndex];

			// Replace %d with travel days.
			const size_t index = text.find("%d");
			text.replace(index, 2, std::to_string(travelData.travelDays));

			return text;
		}();

		// Replace carriage returns with newlines.
		return String::replace(dayStringPrefix + dayStringBody, '\r', '\n');
	}();

	const int travelDistance = [&srcLocationDef, &srcProvinceDef, &worldMapDef, &dstProvinceDef, &dstLocationDef]()
	{
		const Rect srcProvinceRect = srcProvinceDef.getGlobalRect();
		const Rect dstProvinceRect = dstProvinceDef.getGlobalRect();
		const Int2 srcLocationGlobalPoint = ArenaLocationUtils::getGlobalPoint(
			Int2(srcLocationDef.getScreenX(), srcLocationDef.getScreenY()), srcProvinceRect);
		const Int2 dstLocationGlobalPoint = ArenaLocationUtils::getGlobalPoint(
			Int2(dstLocationDef.getScreenX(), dstLocationDef.getScreenY()), dstProvinceRect);
		return ArenaLocationUtils::getMapDistance(srcLocationGlobalPoint, dstLocationGlobalPoint);
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

	return locationFormatText + startDateString + "\n\n" + dayString + distanceString + arrivalDateString;
}

bool ProvinceSearchUiModel::isCharAllowed(char c)
{
	// Letters, numbers, spaces, and symbols are allowed.
	return (c >= ' ') && (c < 127);
}

std::string ProvinceSearchUiModel::getTitleText(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	return exeData.travel.searchTitleText;
}

std::vector<int> ProvinceSearchUiModel::getMatchingLocations(Game &game, const std::string &locationName,
	int provinceIndex, const int **exactLocationIndex)
{
	auto &gameState = game.gameState;
	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
	const WorldMapInstance &worldMapInst = gameState.getWorldMapInstance();

	const ProvinceInstance &provinceInst = worldMapInst.getProvinceInstance(provinceIndex);
	const int provinceDefIndex = provinceInst.getProvinceDefIndex();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceDefIndex);

	// Iterate through all locations in the province. If any visible location's name has
	// a match with the one entered, then add the location to the matching IDs.
	std::vector<int> locationIndices;
	for (int i = 0; i < provinceInst.getLocationCount(); i++)
	{
		const LocationInstance &locationInst = provinceInst.getLocationInstance(i);

		// Only check visible locations.
		if (locationInst.isVisible())
		{
			const int locationDefIndex = locationInst.getLocationDefIndex();
			const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);
			const std::string &curLocationName = locationInst.getName(locationDef);

			// See if the location names are an exact match.
			const bool isExactMatch = String::caseInsensitiveEquals(locationName, curLocationName);

			if (isExactMatch)
			{
				locationIndices.emplace_back(i);
				*exactLocationIndex = &locationIndices.back();
				break;
			}
			else
			{
				// Approximate match behavior. If the given location name is a case-insensitive
				// substring of the current location, it's a match.
				const std::string locNameLower = String::toLowercase(locationName);
				const std::string locDataNameLower = String::toLowercase(curLocationName);
				const bool isApproxMatch = locDataNameLower.find(locNameLower) != std::string::npos;

				if (isApproxMatch)
				{
					locationIndices.emplace_back(i);
				}
			}
		}
	}

	// If no exact or approximate matches, just fill the list with all visible location IDs.
	if (locationIndices.empty())
	{
		for (int i = 0; i < provinceInst.getLocationCount(); i++)
		{
			const LocationInstance &locationInst = provinceInst.getLocationInstance(i);
			if (locationInst.isVisible())
			{
				locationIndices.emplace_back(i);
			}
		}
	}

	// If one approximate match was found and no exact match was found, treat the approximate
	// match as the nearest.
	if ((locationIndices.size() == 1) && (*exactLocationIndex == nullptr))
	{
		*exactLocationIndex = &locationIndices.front();
	}

	// The original game orders locations by their location ID, but that's hardly helpful for the
	// player because they memorize places by name. Therefore, this feature will deviate from
	// the original behavior for the sake of convenience. If the list isn't sorted alphabetically,
	// then it takes the player linear time to find a location in it, which essentially isn't any
	// faster than hovering over each location individually.
	std::sort(locationIndices.begin(), locationIndices.end(),
		[&provinceInst, &provinceDef](int a, int b)
	{
		const LocationInstance &locationInstA = provinceInst.getLocationInstance(a);
		const LocationInstance &locationInstB = provinceInst.getLocationInstance(b);
		const int locationDefIndexA = locationInstA.getLocationDefIndex();
		const int locationDefIndexB = locationInstB.getLocationDefIndex();
		const LocationDefinition &locationDefA = provinceDef.getLocationDef(locationDefIndexA);
		const LocationDefinition &locationDefB = provinceDef.getLocationDef(locationDefIndexB);

		const std::string &aName = locationInstA.getName(locationDefA);
		const std::string &bName = locationInstB.getName(locationDefB);
		return aName.compare(bName) < 0;
	});

	return locationIndices;
}
