#include <cstring>

#include "LocationDefinition.h"
#include "LocationType.h"
#include "LocationUtils.h"
#include "../Assets/BinaryAssetLibrary.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

void LocationDefinition::CityDefinition::MainQuestTempleOverride::init(int modelIndex,
	int suffixIndex, int menuNamesIndex)
{
	this->modelIndex = modelIndex;
	this->suffixIndex = suffixIndex;
	this->menuNamesIndex = menuNamesIndex;
}

void LocationDefinition::CityDefinition::init(ArenaTypes::CityType type, const char *typeDisplayName,
	const char *mapFilename, uint32_t citySeed, uint32_t wildSeed, uint32_t provinceSeed,
	uint32_t rulerSeed, uint32_t distantSkySeed, ClimateType climateType,
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
	this->distantSkySeed = distantSkySeed;
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

uint32_t LocationDefinition::CityDefinition::getWildDungeonSeed(int wildBlockX, int wildBlockY) const
{
	return (this->provinceSeed + (((wildBlockY << 6) + wildBlockX) & 0xFFFF)) & 0xFFFFFFFF;
}

void LocationDefinition::DungeonDefinition::init(uint32_t dungeonSeed, int widthChunkCount,
	int heightChunkCount)
{
	this->dungeonSeed = dungeonSeed;
	this->widthChunkCount = widthChunkCount;
	this->heightChunkCount = heightChunkCount;
}

void LocationDefinition::MainQuestDungeonDefinition::init(MainQuestDungeonDefinition::Type type,
	const char *mapFilename)
{
	this->type = type;
	std::snprintf(this->mapFilename, std::size(this->mapFilename), "%s", mapFilename);
}

void LocationDefinition::init(LocationDefinition::Type type, const std::string &name,
	int x, int y, double latitude)
{
	this->name = name;
	this->x = x;
	this->y = y;
	this->latitude = latitude;
	this->visibleByDefault = (type == LocationDefinition::Type::City) && (name.size() > 0);
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
		const Int2 globalPoint = LocationUtils::getGlobalPoint(localPoint, provinceRect);
		return LocationUtils::getLatitude(globalPoint);
	}();

	this->init(LocationDefinition::Type::City, locationData.name,
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

	const int globalCityID = LocationUtils::getGlobalCityID(localCityID, provinceID);
	const bool isCityState = type == ArenaTypes::CityType::CityState;
	const int templateCount = LocationUtils::getCityTemplateCount(coastal, isCityState);
	const int templateID = globalCityID % templateCount;

	// @todo: deprecate LocationType in favor of CityDefinition::Type.
	const LocationType locationType = [type]()
	{
		switch (type)
		{
		case ArenaTypes::CityType::CityState:
			return LocationType::CityState;
		case ArenaTypes::CityType::Town:
			return LocationType::Town;
		case ArenaTypes::CityType::Village:
			return LocationType::Village;
		default:
			DebugUnhandledReturnMsg(LocationType, std::to_string(static_cast<int>(type)));
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
			const int nameIndex = LocationUtils::getCityTemplateNameIndex(locationType, coastal);

			// Get the template name associated with the city ID.
			const auto &templateFilenames = exeData.cityGen.templateFilenames;
			DebugAssertIndex(templateFilenames, nameIndex);
			std::string templateName = templateFilenames[nameIndex];
			templateName = String::replace(templateName, "%d", std::to_string(templateID + 1));
			templateName = String::toUppercase(templateName);

			return templateName;
		}
	}();

	const uint32_t citySeed = LocationUtils::getCitySeed(localCityID, provinceData);
	const uint32_t wildSeed = LocationUtils::getWildernessSeed(localCityID, provinceData);
	const uint32_t provinceSeed = LocationUtils::getProvinceSeed(provinceID, provinceData);
	const uint32_t rulerSeed = LocationUtils::getRulerSeed(localPoint, provinceRect);
	const uint32_t distantSkySeed = LocationUtils::getDistantSkySeed(
		localPoint, provinceID, provinceRect);
	const ClimateType climateType = LocationUtils::getCityClimateType(
		localCityID, provinceID, binaryAssetLibrary);

	const auto &cityGen = exeData.cityGen;
	const std::vector<uint8_t> *reservedBlocks = [coastal, templateID, &cityGen]()
	{
		const int index = LocationUtils::getCityReservedBlockListIndex(coastal, templateID);
		DebugAssertIndex(cityGen.reservedBlockLists, index);
		return &cityGen.reservedBlockLists[index];
	}();

	const OriginalInt2 blockStartPosition = [coastal, templateID, locationType, &cityGen]()
	{
		const int index = LocationUtils::getCityStartingPositionIndex(locationType, coastal, templateID);
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

	CityDefinition::MainQuestTempleOverride mainQuestTempleOverride;
	const CityDefinition::MainQuestTempleOverride *mainQuestTempleOverridePtr = &mainQuestTempleOverride;
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
		(provinceID == LocationUtils::CENTER_PROVINCE_ID) && (localCityID == 0);

	this->city.init(type, typeDisplayName.c_str(), mapFilename.c_str(), citySeed, wildSeed,
		provinceSeed, rulerSeed, distantSkySeed, climateType, reservedBlocks, blockStartPosition.x,
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
		const Int2 globalPoint = LocationUtils::getGlobalPoint(
			Int2(locationData.x, locationData.y), provinceData.getGlobalRect());
		return LocationUtils::getLatitude(globalPoint);
	}();

	this->init(LocationDefinition::Type::Dungeon, locationData.name,
		locationData.x, locationData.y, latitude);

	const uint32_t dungeonSeed = LocationUtils::getDungeonSeed(localDungeonID, provinceID, provinceData);
	const int widthChunkCount = 2;
	const int heightChunkCount = 1;

	this->dungeon.init(dungeonSeed, widthChunkCount, heightChunkCount);
}

void LocationDefinition::initMainQuestDungeon(const std::optional<int> &optLocalDungeonID,
	int provinceID, MainQuestDungeonDefinition::Type type, const BinaryAssetLibrary &binaryAssetLibrary)
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
		case MainQuestDungeonDefinition::Type::Start:
			return exeData.locations.startDungeonName;
		case MainQuestDungeonDefinition::Type::Map:
		case MainQuestDungeonDefinition::Type::Staff:
		{
			const int locationID = LocationUtils::dungeonToLocationID(*optLocalDungeonID);
			const auto &locationData = provinceData.getLocationData(locationID);
			return locationData.name;
		}
		default:
			DebugUnhandledReturnMsg(std::string, std::to_string(static_cast<int>(type)));
		}
	}();

	int localPointX, localPointY;
	if (type == LocationDefinition::MainQuestDungeonDefinition::Type::Start)
	{
		// Not well-defined in original game.
		localPointX = 0;
		localPointY = 0;
	}
	else
	{
		const int locationID = LocationUtils::dungeonToLocationID(*optLocalDungeonID);
		const auto &locationData = provinceData.getLocationData(locationID);
		localPointX = locationData.x;
		localPointY = locationData.y;
	}

	const double latitude = [type, &optLocalDungeonID, &provinceData]()
	{
		if (type == LocationDefinition::MainQuestDungeonDefinition::Type::Start)
		{
			// Not well-defined in original game.
			return 0.0;
		}
		else
		{
			const int locationID = LocationUtils::dungeonToLocationID(*optLocalDungeonID);
			const auto &locationData = provinceData.getLocationData(locationID);
			const Int2 globalPoint = LocationUtils::getGlobalPoint(
				Int2(locationData.x, locationData.y), provinceData.getGlobalRect());
			return LocationUtils::getLatitude(globalPoint);
		}
	}();

	this->init(LocationDefinition::Type::MainQuestDungeon, std::move(name),
		localPointX, localPointY, latitude);

	const std::string mapFilename = [type, &optLocalDungeonID, provinceID, &cityData,
		&provinceData, &exeData]()
	{
		if (type == LocationDefinition::MainQuestDungeonDefinition::Type::Start)
		{
			return String::toUppercase(exeData.locations.startDungeonMifName);
		}
		else if ((type == LocationDefinition::MainQuestDungeonDefinition::Type::Map) ||
			(type == LocationDefinition::MainQuestDungeonDefinition::Type::Staff))
		{
			const uint32_t dungeonSeed = LocationUtils::getDungeonSeed(*optLocalDungeonID, provinceID, provinceData);
			const std::string mifName = LocationUtils::getMainQuestDungeonMifName(dungeonSeed);
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

LocationDefinition::Type LocationDefinition::getType() const
{
	return this->type;
}

const LocationDefinition::CityDefinition &LocationDefinition::getCityDefinition() const
{
	DebugAssert(this->type == LocationDefinition::Type::City);
	return this->city;
}

const LocationDefinition::DungeonDefinition &LocationDefinition::getDungeonDefinition() const
{
	DebugAssert(this->type == LocationDefinition::Type::Dungeon);
	return this->dungeon;
}

const LocationDefinition::MainQuestDungeonDefinition &LocationDefinition::getMainQuestDungeonDefinition() const
{
	DebugAssert(this->type == LocationDefinition::Type::MainQuestDungeon);
	return this->mainQuest;
}

bool LocationDefinition::matches(const LocationDefinition &other) const
{
	// Can't have two different locations on the same province pixel.
	return (this->name == other.name) && (this->x == other.x) && (this->y == other.y);
}
