#include <cstring>

#include "ArenaLocationUtils.h"
#include "LocationDefinition.h"
#include "../Assets/BinaryAssetLibrary.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

void LocationCityDefinition::MainQuestTempleOverride::init(int modelIndex, int suffixIndex, int menuNamesIndex)
{
	this->modelIndex = modelIndex;
	this->suffixIndex = suffixIndex;
	this->menuNamesIndex = menuNamesIndex;
}

void LocationCityDefinition::init(ArenaTypes::CityType type, const char *typeDisplayName,
	const char *mapFilename, uint32_t citySeed, uint32_t wildSeed, uint32_t provinceSeed,
	uint32_t rulerSeed, uint32_t skySeed, ArenaTypes::ClimateType climateType,
	const std::vector<uint8_t> *reservedBlocks, WEInt blockStartPosX, SNInt blockStartPosY,
	const MainQuestTempleOverride *mainQuestTempleOverride, int cityBlocksPerSide, bool coastal,
	bool premade, bool rulerIsMale, bool palaceIsMainQuestDungeon)
{
	this->type = type;
	std::snprintf(this->typeDisplayName, std::size(this->typeDisplayName), "%s", typeDisplayName);
	std::snprintf(this->mapFilename, std::size(this->mapFilename), "%s", mapFilename);

	this->citySeed = citySeed;
	this->wildSeed = wildSeed;
	this->provinceSeed = provinceSeed;
	this->rulerSeed = rulerSeed;
	this->skySeed = skySeed;
	this->climateType = climateType;
	this->reservedBlocks = reservedBlocks;
	this->blockStartPosX = blockStartPosX;
	this->blockStartPosY = blockStartPosY;

	if (mainQuestTempleOverride != nullptr)
	{
		this->hasMainQuestTempleOverride = true;
		this->mainQuestTempleOverride = *mainQuestTempleOverride;
	}
	else
	{
		this->hasMainQuestTempleOverride = false;
	}

	this->cityBlocksPerSide = cityBlocksPerSide;
	this->coastal = coastal;
	this->premade = premade;
	this->rulerIsMale = rulerIsMale;
	this->palaceIsMainQuestDungeon = palaceIsMainQuestDungeon;
}

uint32_t LocationCityDefinition::getWildDungeonSeed(int wildBlockX, int wildBlockY) const
{
	return (this->provinceSeed + (((wildBlockY << 6) + wildBlockX) & 0xFFFF)) & 0xFFFFFFFF;
}

void LocationDungeonDefinition::init(uint32_t dungeonSeed, int widthChunkCount,
	int heightChunkCount)
{
	this->dungeonSeed = dungeonSeed;
	this->widthChunkCount = widthChunkCount;
	this->heightChunkCount = heightChunkCount;
}

void LocationMainQuestDungeonDefinition::init(LocationMainQuestDungeonDefinitionType type, const char *mapFilename)
{
	this->type = type;
	std::snprintf(this->mapFilename, std::size(this->mapFilename), "%s", mapFilename);
}

void LocationDefinition::init(LocationDefinitionType type, const std::string &name,
	int x, int y, double latitude)
{
	this->name = name;
	this->x = x;
	this->y = y;
	this->latitude = latitude;
	this->visibleByDefault = (type == LocationDefinitionType::City) && (name.size() > 0);
	this->type = type;
}

void LocationDefinition::initCity(int localCityID, int provinceID, bool coastal, bool premade,
	ArenaTypes::CityType type, const BinaryAssetLibrary &binaryAssetLibrary)
{
	const auto &cityData = binaryAssetLibrary.getCityDataFile();
	const auto &provinceData = cityData.getProvinceData(provinceID);
	const auto &locationData = provinceData.getLocationData(localCityID);
	const Int2 localPoint(locationData.x, locationData.y);
	const Rect provinceRect = provinceData.getGlobalRect();
	const double latitude = [&provinceData, &locationData, &localPoint, &provinceRect]()
	{
		const Int2 globalPoint = ArenaLocationUtils::getGlobalPoint(localPoint, provinceRect);
		return ArenaLocationUtils::getLatitude(globalPoint);
	}();

	this->init(LocationDefinitionType::City, locationData.name,
		locationData.x, locationData.y, latitude);

	const auto &exeData = binaryAssetLibrary.getExeData();
	const std::string &typeDisplayName = [type, &exeData]() -> const std::string&
	{
		const int typeNameIndex = [type]()
		{
			switch (type)
			{
			case ArenaTypes::CityType::CityState:
				return 0;
			case ArenaTypes::CityType::Town:
				return 1;
			case ArenaTypes::CityType::Village:
				return 2;
			default:
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(type)));
			}
		}();

		const auto &locationTypeNames = exeData.locations.locationTypes;
		DebugAssertIndex(locationTypeNames, typeNameIndex);
		return locationTypeNames[typeNameIndex];
	}();

	const int globalCityID = ArenaLocationUtils::getGlobalCityID(localCityID, provinceID);
	const bool isCityState = type == ArenaTypes::CityType::CityState;
	const int templateCount = ArenaLocationUtils::getCityTemplateCount(coastal, isCityState);
	const int templateID = globalCityID % templateCount;

	// @todo: deprecate LocationType in favor of CityDefinition::Type.
	const ArenaTypes::LocationType locationType = [type]()
	{
		switch (type)
		{
		case ArenaTypes::CityType::CityState:
			return ArenaTypes::LocationType::CityState;
		case ArenaTypes::CityType::Town:
			return ArenaTypes::LocationType::Town;
		case ArenaTypes::CityType::Village:
			return ArenaTypes::LocationType::Village;
		default:
			DebugUnhandledReturnMsg(ArenaTypes::LocationType, std::to_string(static_cast<int>(type)));
		}
	}();

	const std::string mapFilename = [coastal, premade, &exeData, templateID, locationType]()
	{
		if (premade)
		{
			// @todo: don't rely on center province city's .MIF name.
			return String::toUppercase(exeData.locations.centerProvinceCityMifName);
		}
		else
		{
			// Get the index into the template names array (town%d.mif, ..., cityw%d.mif).
			const int nameIndex = ArenaLocationUtils::getCityTemplateNameIndex(locationType, coastal);

			// Get the template name associated with the city ID.
			const auto &templateFilenames = exeData.cityGen.templateFilenames;
			DebugAssertIndex(templateFilenames, nameIndex);
			std::string templateName = templateFilenames[nameIndex];
			templateName = String::replace(templateName, "%d", std::to_string(templateID + 1));
			templateName = String::toUppercase(templateName);

			return templateName;
		}
	}();

	const uint32_t citySeed = ArenaLocationUtils::getCitySeed(localCityID, provinceData);
	const uint32_t wildSeed = ArenaLocationUtils::getWildernessSeed(localCityID, provinceData);
	const uint32_t provinceSeed = ArenaLocationUtils::getProvinceSeed(provinceID, provinceData);
	const uint32_t rulerSeed = ArenaLocationUtils::getRulerSeed(localPoint, provinceRect);
	const uint32_t skySeed = ArenaLocationUtils::getSkySeed(localPoint, provinceID, provinceRect);
	const ArenaTypes::ClimateType climateType = ArenaLocationUtils::getCityClimateType(localCityID, provinceID, binaryAssetLibrary);

	const auto &cityGen = exeData.cityGen;
	const std::vector<uint8_t> *reservedBlocks = [coastal, templateID, &cityGen]()
	{
		const int index = ArenaLocationUtils::getCityReservedBlockListIndex(coastal, templateID);
		DebugAssertIndex(cityGen.reservedBlockLists, index);
		return &cityGen.reservedBlockLists[index];
	}();

	const OriginalInt2 blockStartPosition = [coastal, templateID, locationType, &cityGen]()
	{
		const int index = ArenaLocationUtils::getCityStartingPositionIndex(locationType, coastal, templateID);
		DebugAssertIndex(cityGen.startingPositions, index);
		const auto &pair = cityGen.startingPositions[index];
		return OriginalInt2(pair.first, pair.second);
	}();

	const int cityBlocksPerSide = [type]()
	{
		switch (type)
		{
		case ArenaTypes::CityType::CityState:
			return 6;
		case ArenaTypes::CityType::Town:
			return 5;
		case ArenaTypes::CityType::Village:
			return 4;
		default:
			DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(type)));
		}
	}();

	LocationCityDefinition::MainQuestTempleOverride mainQuestTempleOverride;
	const LocationCityDefinition::MainQuestTempleOverride *mainQuestTempleOverridePtr = &mainQuestTempleOverride;
	if (globalCityID == 2)
	{
		mainQuestTempleOverride.init(1, 7, 23);
	}
	else if (globalCityID == 224)
	{
		mainQuestTempleOverride.init(2, 8, 32);
	}
	else
	{
		mainQuestTempleOverridePtr = nullptr;
	}

	const bool rulerIsMale = (rulerSeed & 0x3) != 0;
	const bool palaceIsMainQuestDungeon =
		(provinceID == ArenaLocationUtils::CENTER_PROVINCE_ID) && (localCityID == 0);

	this->city.init(type, typeDisplayName.c_str(), mapFilename.c_str(), citySeed, wildSeed,
		provinceSeed, rulerSeed, skySeed, climateType, reservedBlocks, blockStartPosition.x,
		blockStartPosition.y, mainQuestTempleOverridePtr, cityBlocksPerSide, coastal, premade,
		rulerIsMale, palaceIsMainQuestDungeon);
}

void LocationDefinition::initDungeon(int localDungeonID, int provinceID,
	const CityDataFile::ProvinceData::LocationData &locationData,
	const CityDataFile::ProvinceData &provinceData)
{
	DebugAssert((localDungeonID >= 2) && (localDungeonID < 16));

	const double latitude = [&locationData, &provinceData]()
	{
		const Int2 globalPoint = ArenaLocationUtils::getGlobalPoint(
			Int2(locationData.x, locationData.y), provinceData.getGlobalRect());
		return ArenaLocationUtils::getLatitude(globalPoint);
	}();

	this->init(LocationDefinitionType::Dungeon, locationData.name,
		locationData.x, locationData.y, latitude);

	const uint32_t dungeonSeed = ArenaLocationUtils::getDungeonSeed(localDungeonID, provinceID, provinceData);
	const int widthChunkCount = 2;
	const int heightChunkCount = 1;

	this->dungeon.init(dungeonSeed, widthChunkCount, heightChunkCount);
}

void LocationDefinition::initMainQuestDungeon(const std::optional<int> &optLocalDungeonID,
	int provinceID, LocationMainQuestDungeonDefinitionType type, const BinaryAssetLibrary &binaryAssetLibrary)
{
	const auto &cityData = binaryAssetLibrary.getCityDataFile();
	const auto &provinceData = cityData.getProvinceData(provinceID);

	// Start dungeon doesn't have a well-defined world map location in the original game,
	// so need to carefully handle looking it up here.

	// Start dungeon's display name is custom.
	const auto &exeData = binaryAssetLibrary.getExeData();
	std::string name = [type, &provinceData, &optLocalDungeonID, &exeData]()
	{
		switch (type)
		{
		case LocationMainQuestDungeonDefinitionType::Start:
			return exeData.locations.startDungeonName;
		case LocationMainQuestDungeonDefinitionType::Map:
		case LocationMainQuestDungeonDefinitionType::Staff:
		{
			const int locationID = ArenaLocationUtils::dungeonToLocationID(*optLocalDungeonID);
			const auto &locationData = provinceData.getLocationData(locationID);
			return locationData.name;
		}
		default:
			DebugUnhandledReturnMsg(std::string, std::to_string(static_cast<int>(type)));
		}
	}();

	int localPointX, localPointY;
	if (type == LocationMainQuestDungeonDefinitionType::Start)
	{
		// Not well-defined in original game.
		localPointX = 0;
		localPointY = 0;
	}
	else
	{
		const int locationID = ArenaLocationUtils::dungeonToLocationID(*optLocalDungeonID);
		const auto &locationData = provinceData.getLocationData(locationID);
		localPointX = locationData.x;
		localPointY = locationData.y;
	}

	const double latitude = [type, &optLocalDungeonID, &provinceData]()
	{
		if (type == LocationMainQuestDungeonDefinitionType::Start)
		{
			// Not well-defined in original game.
			return 0.0;
		}
		else
		{
			const int locationID = ArenaLocationUtils::dungeonToLocationID(*optLocalDungeonID);
			const auto &locationData = provinceData.getLocationData(locationID);
			const Int2 globalPoint = ArenaLocationUtils::getGlobalPoint(
				Int2(locationData.x, locationData.y), provinceData.getGlobalRect());
			return ArenaLocationUtils::getLatitude(globalPoint);
		}
	}();

	this->init(LocationDefinitionType::MainQuestDungeon, std::move(name), localPointX, localPointY, latitude);

	const std::string mapFilename = [type, &optLocalDungeonID, provinceID, &cityData,
		&provinceData, &exeData]()
	{
		if (type == LocationMainQuestDungeonDefinitionType::Start)
		{
			return String::toUppercase(exeData.locations.startDungeonMifName);
		}
		else if ((type == LocationMainQuestDungeonDefinitionType::Map) ||
			(type == LocationMainQuestDungeonDefinitionType::Staff))
		{
			const uint32_t dungeonSeed = ArenaLocationUtils::getDungeonSeed(*optLocalDungeonID, provinceID, provinceData);
			const std::string mifName = ArenaLocationUtils::getMainQuestDungeonMifName(dungeonSeed);
			return String::toUppercase(mifName);
		}
		else
		{
			DebugUnhandledReturnMsg(std::string, std::to_string(static_cast<int>(type)));
		}
	}();

	this->mainQuest.init(type, mapFilename.c_str());
}

const std::string &LocationDefinition::getName() const
{
	return this->name;
}

int LocationDefinition::getScreenX() const
{
	return this->x;
}

int LocationDefinition::getScreenY() const
{
	return this->y;
}

double LocationDefinition::getLatitude() const
{
	return this->latitude;
}

bool LocationDefinition::isVisibleByDefault() const
{
	return this->visibleByDefault;
}

LocationDefinitionType LocationDefinition::getType() const
{
	return this->type;
}

const LocationCityDefinition &LocationDefinition::getCityDefinition() const
{
	DebugAssert(this->type == LocationDefinitionType::City);
	return this->city;
}

const LocationDungeonDefinition &LocationDefinition::getDungeonDefinition() const
{
	DebugAssert(this->type == LocationDefinitionType::Dungeon);
	return this->dungeon;
}

const LocationMainQuestDungeonDefinition &LocationDefinition::getMainQuestDungeonDefinition() const
{
	DebugAssert(this->type == LocationDefinitionType::MainQuestDungeon);
	return this->mainQuest;
}

bool LocationDefinition::matches(const LocationDefinition &other) const
{
	// Can't have two different locations on the same province pixel.
	return (this->name == other.name) && (this->x == other.x) && (this->y == other.y);
}
