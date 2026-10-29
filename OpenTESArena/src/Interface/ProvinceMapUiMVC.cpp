#include <algorithm>

#include "ProvinceMapUiMVC.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Game/Game.h"
#include "../UI/FontDefinition.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextRenderUtils.h"
#include "../WorldMap/ArenaLocationUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

ProvinceMapUiTravelData::ProvinceMapUiTravelData(int locationID, ProvinceDefinitionID provinceDefID, int travelDays)
{
	this->locationID = locationID;
	this->provinceDefID = provinceDefID;
	this->travelDays = travelDays;
}

std::string ProvinceMapUiModel::makeAlreadyAtLocationText(Game &game, const std::string &locationName)
{
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
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

std::string ProvinceMapUiModel::getLocationName(Game &game, ProvinceDefinitionID provinceDefID, int locationID)
{
	GameState &gameState = game.gameState;
	const ProvinceLibrary &provinceLibrary = ProvinceLibrary::getInstance();
	const WorldMapInstance &worldMapInst = gameState.getWorldMapInstance();
	const ProvinceInstance &provinceInst = worldMapInst.getProvinceFromDefinition(provinceDefID);
	const ProvinceDefinition &provinceDef = provinceLibrary.getDefinition(provinceDefID);
	const LocationInstance &locationInst = provinceInst.locations[locationID];
	const LocationDefinition &locationDef = provinceDef.locations[locationInst.locationDefIndex];
	return locationInst.getName(locationDef);
}

std::string ProvinceMapUiModel::makeTravelText(Game &game, ProvinceDefinitionID srcProvinceDefID, const LocationDefinition &srcLocationDef,
	const ProvinceDefinition &srcProvinceDef, int dstLocationIndex)
{
	GameState &gameState = game.gameState;
	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const ExeData &exeData = binaryAssetLibrary.getExeData();
	const ProvinceLibrary &provinceLibrary = ProvinceLibrary::getInstance();

	const WorldMapInstance &worldMapInst = gameState.getWorldMapInstance();
	const ProvinceDefinitionID dstProvinceDefID = srcProvinceDefID;
	const ProvinceInstance &dstProvinceInst = worldMapInst.getProvinceFromDefinition(dstProvinceDefID);
	const LocationInstance &dstLocationInst = dstProvinceInst.locations[dstLocationIndex];

	const ProvinceDefinition &dstProvinceDef = provinceLibrary.getDefinition(dstProvinceDefID);
	const LocationDefinition &dstLocationDef = dstProvinceDef.locations[dstLocationInst.locationDefIndex];
	const std::string &dstLocationName = dstLocationInst.getName(dstLocationDef);

	const Date &currentDate = gameState.getDate();
	const ProvinceMapUiTravelData *travelDataPtr = gameState.getTravelData();
	DebugAssert(travelDataPtr != nullptr);
	const ProvinceMapUiTravelData &travelData = *travelDataPtr;

	const Date destinationDate = [&currentDate, &travelData]()
	{
		Date newDate = currentDate;
		for (int i = 0; i < travelData.travelDays; i++)
		{
			newDate.incrementDay();
		}

		return newDate;
	}();

	const std::string locationFormatText = [&srcProvinceDef, dstLocationIndex, &exeData, &dstProvinceDef, &dstLocationDef, &dstLocationName]()
	{
		std::string formatText = [&srcProvinceDef, dstLocationIndex, &exeData, &dstProvinceDef, &dstLocationDef, &dstLocationName]()
		{
			Span<const std::string> locationFormatTexts = exeData.travel.locationFormatTexts;

			if (srcProvinceDef.originalProvinceIndex != ArenaLocationUtils::CENTER_PROVINCE_INDEX)
			{
				// Determine whether to use the city format or dungeon format.
				if (dstLocationDef.type == LocationDefinitionType::City)
				{
					// City format.
					const int locationFormatTextsIndex = 2;
					std::string text = locationFormatTexts[locationFormatTextsIndex];

					// Replace first %s with location type.
					const LocationCityDefinition &cityDef = dstLocationDef.city;
					const std::string_view locationTypeName = cityDef.typeDisplayName;

					size_t index = text.find("%s");
					text.replace(index, 2, locationTypeName);

					// Replace second %s with location name.
					index = text.find("%s", index);
					text.replace(index, 2, dstLocationName);

					// Replace third %s with province name.
					index = text.find("%s", index);
					text.replace(index, 2, dstProvinceDef.name);

					return text;
				}
				else if ((dstLocationDef.type == LocationDefinitionType::Dungeon) ||
					(dstLocationDef.type == LocationDefinitionType::MainQuestDungeon))
				{
					// Dungeon format.
					const int locationFormatTextsIndex = 0;
					std::string text = locationFormatTexts[locationFormatTextsIndex];

					// Replace first %s with dungeon name.
					size_t index = text.find("%s");
					text.replace(index, 2, dstLocationName);

					// Replace second %s with province name.
					index = text.find("%s", index);
					text.replace(index, 2, dstProvinceDef.name);

					return text;
				}
				else
				{
					DebugUnhandledReturnMsg(std::string, std::to_string(static_cast<int>(dstLocationDef.type)));
				}
			}
			else
			{
				// Center province format (always the center city).
				const int locationFormatTextsIndex = 1;
				std::string text = locationFormatTexts[locationFormatTextsIndex];

				// Replace first %s with center province city name.
				size_t index = text.find("%s");
				text.replace(index, 2, dstLocationName);

				// Replace second %s with center province name.
				index = text.find("%s", index);
				text.replace(index, 2, dstProvinceDef.name);

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

	const int travelDistance = [&srcLocationDef, &srcProvinceDef, &dstProvinceDef, &dstLocationDef]()
	{
		const Rect srcProvinceRect = srcProvinceDef.getGlobalRect();
		const Rect dstProvinceRect = dstProvinceDef.getGlobalRect();
		const Int2 srcLocationGlobalPoint = ArenaLocationUtils::getGlobalPoint(
			Int2(srcLocationDef.screenX, srcLocationDef.screenY), srcProvinceRect);
		const Int2 dstLocationGlobalPoint = ArenaLocationUtils::getGlobalPoint(
			Int2(dstLocationDef.screenX, dstLocationDef.screenY), dstProvinceRect);
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
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	return exeData.travel.searchTitleText;
}

std::vector<int> ProvinceSearchUiModel::getMatchingLocations(Game &game, const std::string &locationName,
	ProvinceDefinitionID provinceDefID, const int **exactLocationIndex)
{
	GameState &gameState = game.gameState;
	const WorldMapInstance &worldMapInst = gameState.getWorldMapInstance();
	const ProvinceInstance &provinceInst = worldMapInst.getProvinceFromDefinition(provinceDefID);
	const ProvinceDefinition &provinceDef = ProvinceLibrary::getInstance().getDefinition(provinceDefID);

	// Iterate through all locations in the province. If any visible location's name has
	// a match with the one entered, then add the location to the matching IDs.
	std::vector<int> locationIndices;
	for (int i = 0; i < provinceInst.locations.getCount(); i++)
	{
		const LocationInstance &locationInst = provinceInst.locations[i];

		// Only check visible locations.
		if (locationInst.visible)
		{
			const LocationDefinition &locationDef = provinceDef.locations[locationInst.locationDefIndex];
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
		for (int i = 0; i < provinceInst.locations.getCount(); i++)
		{
			const LocationInstance &locationInst = provinceInst.locations[i];
			if (locationInst.visible)
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
		const LocationInstance &locationInstA = provinceInst.locations[a];
		const LocationInstance &locationInstB = provinceInst.locations[b];
		const LocationDefinition &locationDefA = provinceDef.locations[locationInstA.locationDefIndex];
		const LocationDefinition &locationDefB = provinceDef.locations[locationInstB.locationDefIndex];
		const std::string &aName = locationInstA.getName(locationDefA);
		const std::string &bName = locationInstB.getName(locationDefB);
		return aName.compare(bName) < 0;
	});

	return locationIndices;
}

Int2 ProvinceMapUiView::getLocationCenterPoint(Game &game, ProvinceDefinitionID provinceDefID, int locationID)
{
	const GameState &gameState = game.gameState;
	const ProvinceDefinition &provinceDef = ProvinceLibrary::getInstance().getDefinition(provinceDefID);
	const LocationDefinition &locationDef = provinceDef.locations[locationID];
	return Int2(locationDef.screenX, locationDef.screenY);
}

Int2 ProvinceMapUiView::getLocationTextClampedCenter(const Rect &unclampedRect)
{
	const Int2 unclampedTopLeft = unclampedRect.getTopLeft();
	const Int2 clampedTopLeft(
		std::clamp(unclampedTopLeft.x, 2, ArenaRenderUtils::SCREEN_WIDTH - unclampedRect.width - 2),
		std::clamp(unclampedTopLeft.y, 2, ArenaRenderUtils::SCREEN_HEIGHT - unclampedRect.height - 2));
	return clampedTopLeft + Int2(unclampedRect.width / 2, unclampedRect.height / 2);
}

int ProvinceMapUiView::getTextPopUpTextureWidth(int textWidth)
{
	return textWidth + 20;
}

int ProvinceMapUiView::getTextPopUpTextureHeight(int textHeight)
{
	// Parchment minimum height is 40 pixels.
	return std::max(textHeight + 16, 40);
}

bool ProvinceMapUiView::provinceHasStaffDungeonIcon(ProvinceDefinitionID provinceDefID)
{
	const ProvinceDefinition &provinceDef = ProvinceLibrary::getInstance().getDefinition(provinceDefID);
	return provinceDef.originalProvinceIndex != ArenaLocationUtils::CENTER_PROVINCE_INDEX;
}

TextureAsset ProvinceMapUiView::getBackgroundTextureAsset(ProvinceDefinitionID provinceDefID, const BinaryAssetLibrary &binaryAssetLibrary)
{
	const ProvinceDefinition &provinceDef = ProvinceLibrary::getInstance().getDefinition(provinceDefID);
	const ExeData &exeData = binaryAssetLibrary.getExeData();
	Span<const std::string> provinceImgFilenames = exeData.locations.provinceImgFilenames;
	const std::string &filename = provinceImgFilenames[provinceDef.originalProvinceIndex];
	return TextureAsset(String::toUppercase(filename));
}

TextureAsset ProvinceMapUiView::getBackgroundPaletteTextureAsset(ProvinceDefinitionID provinceDefID, const BinaryAssetLibrary &binaryAssetLibrary)
{
	return TextureAsset(ProvinceMapUiView::getBackgroundTextureAsset(provinceDefID, binaryAssetLibrary));
}

TextureAsset ProvinceMapUiView::getCityStateIconTextureAsset(ProvinceMapUiHighlightType highlightType)
{
	if (highlightType == ProvinceMapUiHighlightType::None)
	{
		return TextureAsset(ArenaTextureName::CityStateIcon);
	}
	else if (highlightType == ProvinceMapUiHighlightType::PlayerLocation)
	{
		return TextureAsset(ArenaTextureName::MapIconOutlines, ProvinceMapUiView::CityStateIconHighlightIndex);
	}
	else if (highlightType == ProvinceMapUiHighlightType::TravelDestination)
	{
		return TextureAsset(ArenaTextureName::MapIconOutlinesBlinking, ProvinceMapUiView::CityStateIconHighlightIndex);
	}
	else
	{
		DebugUnhandledReturnMsg(TextureAsset, std::to_string(static_cast<int>(highlightType)));
	}
}

TextureAsset ProvinceMapUiView::getTownIconTextureAsset(ProvinceMapUiHighlightType highlightType)
{
	if (highlightType == ProvinceMapUiHighlightType::None)
	{
		return TextureAsset(ArenaTextureName::TownIcon);
	}
	else if (highlightType == ProvinceMapUiHighlightType::PlayerLocation)
	{
		return TextureAsset(ArenaTextureName::MapIconOutlines, ProvinceMapUiView::TownIconHighlightIndex);
	}
	else if (highlightType == ProvinceMapUiHighlightType::TravelDestination)
	{
		return TextureAsset(ArenaTextureName::MapIconOutlinesBlinking, ProvinceMapUiView::TownIconHighlightIndex);
	}
	else
	{
		DebugUnhandledReturnMsg(TextureAsset, std::to_string(static_cast<int>(highlightType)));
	}
}

TextureAsset ProvinceMapUiView::getVillageIconTextureAsset(ProvinceMapUiHighlightType highlightType)
{
	if (highlightType == ProvinceMapUiHighlightType::None)
	{
		return TextureAsset(ArenaTextureName::VillageIcon);
	}
	else if (highlightType == ProvinceMapUiHighlightType::PlayerLocation)
	{
		return TextureAsset(ArenaTextureName::MapIconOutlines, ProvinceMapUiView::VillageIconHighlightIndex);
	}
	else if (highlightType == ProvinceMapUiHighlightType::TravelDestination)
	{
		return TextureAsset(ArenaTextureName::MapIconOutlinesBlinking, ProvinceMapUiView::VillageIconHighlightIndex);
	}
	else
	{
		DebugUnhandledReturnMsg(TextureAsset, std::to_string(static_cast<int>(highlightType)));
	}
}

TextureAsset ProvinceMapUiView::getDungeonIconTextureAsset(ProvinceMapUiHighlightType highlightType)
{
	if (highlightType == ProvinceMapUiHighlightType::None)
	{
		return TextureAsset(ArenaTextureName::DungeonIcon);
	}
	else if (highlightType == ProvinceMapUiHighlightType::PlayerLocation)
	{
		return TextureAsset(ArenaTextureName::MapIconOutlines, ProvinceMapUiView::DungeonIconHighlightIndex);
	}
	else if (highlightType == ProvinceMapUiHighlightType::TravelDestination)
	{
		return TextureAsset(ArenaTextureName::MapIconOutlinesBlinking, ProvinceMapUiView::DungeonIconHighlightIndex);
	}
	else
	{
		DebugUnhandledReturnMsg(TextureAsset, std::to_string(static_cast<int>(highlightType)));
	}
}

TextureAsset ProvinceMapUiView::getStaffDungeonIconTextureAsset(ProvinceDefinitionID provinceDefID)
{
	return TextureAsset(ArenaTextureName::StaffDungeonIcons, provinceDefID);
}

UiTextureID ProvinceMapUiView::allocBackgroundTexture(ProvinceDefinitionID provinceDefID, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = ProvinceMapUiView::getBackgroundTextureAsset(provinceDefID, binaryAssetLibrary);
	const TextureAsset paletteTextureAsset = ProvinceMapUiView::getBackgroundPaletteTextureAsset(provinceDefID, binaryAssetLibrary);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't allocate province \"" + std::to_string(provinceDefID) + "\" background texture.");
	}

	return textureID;
}

UiTextureID ProvinceMapUiView::allocCityStateIconTexture(ProvinceMapUiHighlightType highlightType, const TextureAsset &paletteTextureAsset,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = ProvinceMapUiView::getCityStateIconTextureAsset(highlightType);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't allocate city state icon texture.");
	}

	return textureID;
}

UiTextureID ProvinceMapUiView::allocTownIconTexture(ProvinceMapUiHighlightType highlightType, const TextureAsset &paletteTextureAsset,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = ProvinceMapUiView::getTownIconTextureAsset(highlightType);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't allocate town icon texture.");
	}

	return textureID;
}

UiTextureID ProvinceMapUiView::allocVillageIconTexture(ProvinceMapUiHighlightType highlightType, const TextureAsset &paletteTextureAsset,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = ProvinceMapUiView::getVillageIconTextureAsset(highlightType);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't allocate village icon texture.");
	}

	return textureID;
}

UiTextureID ProvinceMapUiView::allocDungeonIconTexture(ProvinceMapUiHighlightType highlightType, const TextureAsset &paletteTextureAsset,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = ProvinceMapUiView::getDungeonIconTextureAsset(highlightType);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't allocate dungeon icon texture.");
	}

	return textureID;
}

UiTextureID ProvinceMapUiView::allocStaffDungeonIconTexture(ProvinceDefinitionID provinceDefID, ProvinceMapUiHighlightType highlightType,
	const TextureAsset &paletteTextureAsset, TextureManager &textureManager, Renderer &renderer)
{
	DebugAssert(ProvinceMapUiView::provinceHasStaffDungeonIcon(provinceDefID));

	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteTextureAsset);
	if (!paletteID.has_value())
	{
		DebugLogErrorFormat("Couldn't get staff dungeon palette ID for \"%s\".", paletteTextureAsset.filename.c_str());
		return -1;
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	const TextureAsset textureAsset = ProvinceMapUiView::getStaffDungeonIconTextureAsset(provinceDefID);
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
	if (!textureBuilderID.has_value())
	{
		DebugLogErrorFormat("Couldn't get staff dungeon texture builder ID for \"%s\".", textureAsset.filename.c_str());
		return -1;
	}

	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
	const UiTextureID textureID = renderer.createUiTexture(textureBuilder.width, textureBuilder.height);
	if (textureID < 0)
	{
		DebugLogErrorFormat("Couldn't create staff dungeon texture for \"%s\".", textureAsset.filename.c_str());
		return -1;
	}

	if (!renderer.populateUiTexture(textureID, textureBuilder.bytes, &palette))
	{
		DebugLogErrorFormat("Couldn't populate staff dungeon texture for \"%s\".", textureAsset.filename.c_str());
	}

	if (highlightType == ProvinceMapUiHighlightType::None)
	{
		return textureID;
	}

	// Modify icon background texels based on the highlight type.
	const uint8_t *srcTexels = textureBuilder.getTexels8().begin();
	LockedTexture lockedTexture = renderer.lockUiTexture(textureID);
	if (!lockedTexture.isValid())
	{
		DebugLogError("Couldn't lock staff dungeon icon texels for highlight modification.");
		return textureID;
	}

	Span2D<uint32_t> dstTexels = lockedTexture.getTexels32();
	uint32_t *dstTexelsPtr = dstTexels.begin();
	const uint8_t highlightColorIndex = (highlightType == ProvinceMapUiHighlightType::PlayerLocation) ? ProvinceMapUiView::YellowPaletteIndex : ProvinceMapUiView::RedPaletteIndex;
	const uint32_t highlightColor = palette[highlightColorIndex].toRGBA();

	const int texelCount = textureBuilder.width * textureBuilder.height;
	for (int i = 0; i < texelCount; i++)
	{
		const uint8_t srcTexel = srcTexels[i];
		if (srcTexel == ProvinceMapUiView::BackgroundPaletteIndex)
		{
			dstTexelsPtr[i] = highlightColor;
		}
	}

	renderer.unlockUiTexture(textureID);
	return textureID;
}

UiTextureID ProvinceMapUiView::allocTextPopUpTexture(int textWidth, int textHeight,
	TextureManager &textureManager, Renderer &renderer)
{
	const Surface surface = TextureUtils::generate(
		ProvinceMapUiView::TextPopUpTexturePatternType,
		ProvinceMapUiView::getTextPopUpTextureWidth(textWidth),
		ProvinceMapUiView::getTextPopUpTextureHeight(textHeight),
		textureManager,
		renderer);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTextureFromSurface(surface, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create text pop-up texture.");
	}

	return textureID;
}

UiListBoxInitInfo ProvinceSearchUiView::makeListBoxProperties()
{
	const FontLibrary &fontLibrary = FontLibrary::getInstance();

	const char *fontName = ArenaFontName::Arena;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName, &fontDefIndex))
	{
		DebugCrash("Couldn't get search UI list box font \"" + std::string(fontName) + "\".");
	}

	constexpr int maxDisplayedItemCount = 6;
	std::string dummyText;
	for (int i = 0; i < maxDisplayedItemCount; i++)
	{
		if (i > 0)
		{
			dummyText += '\n';
		}

		std::string dummyLine(17, TextRenderUtils::LARGEST_CHAR); // Arbitrary worst-case line size.
		dummyText += dummyLine;
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	UiListBoxInitInfo listBoxInitInfo;
	listBoxInitInfo.textureWidth = textureGenInfo.width;
	listBoxInitInfo.textureHeight = textureGenInfo.height;
	listBoxInitInfo.fontName = fontName;
	listBoxInitInfo.defaultTextColor = Color(52, 24, 8);
	listBoxInitInfo.highlightedTextColor = Color(0, 255, 255);
	return listBoxInitInfo;
}

TextureAsset ProvinceSearchUiView::getListTextureAsset()
{
	return TextureAsset(ArenaTextureName::PopUp8);
}

TextureAsset ProvinceSearchUiView::getListPaletteTextureAsset(const BinaryAssetLibrary &binaryAssetLibrary, ProvinceDefinitionID provinceDefID)
{
	const ProvinceDefinition &provinceDef = ProvinceLibrary::getInstance().getDefinition(provinceDefID);

	const ExeData &exeData = binaryAssetLibrary.getExeData();
	Span<const std::string> provinceImgFilenames = exeData.locations.provinceImgFilenames;
	const std::string &filename = provinceImgFilenames[provinceDef.originalProvinceIndex];

	// Set all characters to uppercase because the texture manager expects extensions to be uppercase,
	// and most filenames in A.EXE are lowercase.
	return String::toUppercase(filename);
}

UiTextureID ProvinceSearchUiView::allocParchmentTexture(TextureManager &textureManager, Renderer &renderer)
{
	const int width = ProvinceSearchUiView::TextureWidth;
	const int height = ProvinceSearchUiView::TextureHeight;
	const Surface surface = TextureUtils::generate(ProvinceSearchUiView::TexturePattern, width, height, textureManager, renderer);

	const UiTextureID textureID = renderer.createUiTexture(width, height);
	if (textureID < 0)
	{
		DebugCrashFormat("Couldn't create parchment texture with dims %dx%d.", width, height);
	}

	LockedTexture lockedTexture = renderer.lockUiTexture(textureID);
	if (!lockedTexture.isValid())
	{
		DebugCrash("Couldn't lock parchment texels for writing.");
	}

	Span2D<const uint32_t> srcTexels = surface.getPixels();
	Span2D<uint32_t> dstTexels = lockedTexture.getTexels32();
	std::copy(srcTexels.begin(), srcTexels.end(), dstTexels.begin());
	renderer.unlockUiTexture(textureID);

	return textureID;
}
