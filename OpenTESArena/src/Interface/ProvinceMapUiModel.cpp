#include "ProvinceMapUiController.h"
#include "ProvinceMapUiModel.h"
#include "ProvinceMapUiView.h"
#include "TextSubPanel.h"
#include "../Game/Game.h"
#include "../WorldMap/LocationUtils.h"

#include "components/utilities/String.h"

ProvinceMapUiModel::TravelData::TravelData(int locationID, int provinceID, int travelDays)
{
	this->locationID = locationID;
	this->provinceID = provinceID;
	this->travelDays = travelDays;
}

std::string ProvinceMapUiModel::makeAlreadyAtLocationText(Game &game, const std::string &locationName)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
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
	auto &gameState = game.getGameState();
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
	const RichTextString richText(
		text,
		ProvinceMapUiView::TextPopUpFontName,
		ProvinceMapUiView::TextPopUpTextColor,
		ProvinceMapUiView::TextPopUpTextAlignment,
		ProvinceMapUiView::TextPopUpLineSpacing,
		game.getFontLibrary());

	Texture texture = TextureUtils::generate(
		ProvinceMapUiView::TextPopUpTexturePatternType,
		ProvinceMapUiView::getTextPopUpTextureWidth(richText.getDimensions().x),
		ProvinceMapUiView::getTextPopUpTextureHeight(richText.getDimensions().y),
		game.getTextureManager(),
		game.getRenderer());

	return std::make_unique<TextSubPanel>(
		game,
		ProvinceMapUiView::TextPopUpCenterPoint,
		richText,
		ProvinceMapUiController::onTextPopUpSelected,
		std::move(texture),
		ProvinceMapUiView::TextPopUpTextureCenterPoint);
}

std::string ProvinceMapUiModel::makeTravelText(Game &game, int srcProvinceIndex, const LocationDefinition &srcLocationDef,
	const ProvinceDefinition &srcProvinceDef, int dstLocationIndex, const TravelData &travelData)
{
	auto &gameState = game.getGameState();
	const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
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
			if (srcProvinceIndex != LocationUtils::CENTER_PROVINCE_ID)
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

	const int travelDistance = [&srcLocationDef, &srcProvinceDef, &worldMapDef, &dstProvinceDef, &dstLocationDef]()
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

	return locationFormatText + startDateString + "\n\n" + dayString + distanceString + arrivalDateString;
}
