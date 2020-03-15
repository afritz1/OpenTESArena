#include "ClimateType.h"
#include "ExteriorWorldData.h"
#include "InteriorWorldData.h"
#include "Location.h"
#include "LocationType.h"
#include "WeatherType.h"
#include "VoxelUtils.h"
#include "WorldType.h"
#include "../Assets/MiscAssets.h"

#include "components/debug/Debug.h"

ExteriorWorldData::InteriorState::InteriorState(InteriorWorldData &&worldData,
	const Int2 &returnVoxel)
	: worldData(std::move(worldData)), returnVoxel(returnVoxel) { }

ExteriorWorldData::ExteriorWorldData(ExteriorLevelData &&levelData, bool isCity)
	: levelData(std::move(levelData))
{
	this->isCity = isCity;
}

ExteriorWorldData::~ExteriorWorldData()
{

}

std::string ExteriorWorldData::generateCityInfName(ClimateType climateType, WeatherType weatherType)
{
	const std::string climateLetter = [climateType]()
	{
		if (climateType == ClimateType::Temperate)
		{
			return "T";
		}
		else if (climateType == ClimateType::Desert)
		{
			return "D";
		}
		else
		{
			return "M";
		}
	}();

	// City/town/village letter. Wilderness is "W".
	const std::string locationLetter = "C";

	const std::string weatherLetter = [climateType, weatherType]()
	{
		if ((weatherType == WeatherType::Clear) ||
			(weatherType == WeatherType::Overcast) ||
			(weatherType == WeatherType::Overcast2))
		{
			return "N";
		}
		else if ((weatherType == WeatherType::Rain) ||
			(weatherType == WeatherType::Rain2))
		{
			return "R";
		}
		else if ((weatherType == WeatherType::Snow) ||
			(weatherType == WeatherType::SnowOvercast) ||
			(weatherType == WeatherType::SnowOvercast2))
		{
			// Deserts can't have snow.
			if (climateType != ClimateType::Desert)
			{
				return "S";
			}
			else
			{
				DebugLogWarning("Deserts do not have snow templates.");
				return "N";
			}
		}
		else
		{
			// Not sure what this letter represents.
			return "W";
		}
	}();

	return climateLetter + locationLetter + weatherLetter + ".INF";
}

std::string ExteriorWorldData::generateWildernessInfName(ClimateType climateType, WeatherType weatherType)
{
	const std::string climateLetter = [climateType]()
	{
		if (climateType == ClimateType::Temperate)
		{
			return "T";
		}
		else if (climateType == ClimateType::Desert)
		{
			return "D";
		}
		else
		{
			return "M";
		}
	}();

	// Wilderness is "W".
	const std::string locationLetter = "W";

	const std::string weatherLetter = [climateType, weatherType]()
	{
		if ((weatherType == WeatherType::Clear) ||
			(weatherType == WeatherType::Overcast) ||
			(weatherType == WeatherType::Overcast2))
		{
			return "N";
		}
		else if ((weatherType == WeatherType::Rain) ||
			(weatherType == WeatherType::Rain2))
		{
			return "R";
		}
		else if ((weatherType == WeatherType::Snow) ||
			(weatherType == WeatherType::SnowOvercast) ||
			(weatherType == WeatherType::SnowOvercast2))
		{
			// Deserts can't have snow.
			if (climateType != ClimateType::Desert)
			{
				return "S";
			}
			else
			{
				DebugLogWarning("Deserts do not have snow templates.");
				return "N";
			}
		}
		else
		{
			// Not sure what this letter represents.
			return "W";
		}
	}();

	return climateLetter + locationLetter + weatherLetter + ".INF";
}

ExteriorWorldData ExteriorWorldData::loadPremadeCity(const MIFFile &mif, ClimateType climateType,
	WeatherType weatherType, int currentDay, int starCount, const MiscAssets &miscAssets,
	TextureManager &textureManager)
{
	// Hardcoded to center province city for now.
	const int localCityID = 0;
	const int provinceID = 8;

	const auto &level = mif.getLevels().front();
	const std::string infName = ExteriorWorldData::generateCityInfName(climateType, weatherType);

	const int gridWidth = mif.getDepth();
	const int gridDepth = mif.getWidth();

	// Generate level data for the city.
	ExteriorLevelData levelData = ExteriorLevelData::loadPremadeCity(localCityID, provinceID,
		level, weatherType, currentDay, starCount, infName, gridWidth, gridDepth, miscAssets,
		textureManager);

	const bool isCity = true;

	// Generate world data from the level data.
	ExteriorWorldData worldData(std::move(levelData), isCity);

	// Convert start points from the old coordinate system to the new one.
	for (const Double2 &point : mif.getStartPoints())
	{
		worldData.startPoints.push_back(VoxelUtils::getTransformedVoxel(
			point, mif.getDepth(), mif.getWidth()));
	}

	worldData.mifName = mif.getName();

	return worldData;
}

ExteriorWorldData ExteriorWorldData::loadCity(int localCityID, int provinceID, const MIFFile &mif,
	int cityDim, bool isCoastal, const std::vector<uint8_t> &reservedBlocks,
	const Int2 &startPosition, WeatherType weatherType, int currentDay, int starCount,
	const MiscAssets &miscAssets, TextureManager &textureManager)
{
	// Generate level.
	const auto &level = mif.getLevels().front();

	// Obtain climate from city data.
	const ClimateType climateType = Location::getCityClimateType(
		localCityID, provinceID, miscAssets);

	const std::string infName = ExteriorWorldData::generateCityInfName(climateType, weatherType);

	// Generate level data for the city.
	ExteriorLevelData levelData = ExteriorLevelData::loadCity(
		level, localCityID, provinceID, weatherType, currentDay, starCount, cityDim, isCoastal,
		reservedBlocks, startPosition, infName, mif.getDepth(), mif.getWidth(),
		miscAssets, textureManager);

	const bool isCity = true;

	// Generate world data from the level data.
	ExteriorWorldData worldData(std::move(levelData), isCity);

	// Convert start points from the old coordinate system to the new one.
	for (const Double2 &point : mif.getStartPoints())
	{
		worldData.startPoints.push_back(VoxelUtils::getTransformedVoxel(
			point, mif.getDepth(), mif.getWidth()));
	}

	worldData.mifName = mif.getName();

	return worldData;
}

ExteriorWorldData ExteriorWorldData::loadWilderness(int localCityID, int provinceID,
	WeatherType weatherType, int currentDay, int starCount, const MiscAssets &miscAssets,
	TextureManager &textureManager)
{
	// Obtain climate from city data.
	const ClimateType climateType = Location::getCityClimateType(
		localCityID, provinceID, miscAssets);

	const std::string infName =
		ExteriorWorldData::generateWildernessInfName(climateType, weatherType);

	// Load wilderness data (no starting points to load).
	ExteriorLevelData levelData = ExteriorLevelData::loadWilderness(
		localCityID, provinceID, weatherType, currentDay, starCount, infName,
		miscAssets, textureManager);

	const bool isCity = false;

	// Generate world data from the wilderness data.
	ExteriorWorldData worldData(std::move(levelData), isCity);
	worldData.mifName = "WILD.MIF";

	return worldData;
}

InteriorWorldData *ExteriorWorldData::getInterior() const
{
	return (this->interior.get() != nullptr) ? &this->interior->worldData : nullptr;
}

const std::string &ExteriorWorldData::getMifName() const
{
	return (this->interior.get() != nullptr) ?
		this->interior->worldData.getMifName() : this->mifName;
}

WorldType ExteriorWorldData::getBaseWorldType() const
{
	return this->isCity ? WorldType::City : WorldType::Wilderness;
}

WorldType ExteriorWorldData::getActiveWorldType() const
{
	return (this->interior.get() != nullptr) ?
		WorldType::Interior : this->getBaseWorldType();
}

LevelData &ExteriorWorldData::getActiveLevel()
{
	return (this->interior.get() != nullptr) ?
		this->interior->worldData.getActiveLevel() :
		static_cast<LevelData&>(this->levelData);
}

const LevelData &ExteriorWorldData::getActiveLevel() const
{
	return (this->interior.get() != nullptr) ?
		this->interior->worldData.getActiveLevel() :
		static_cast<const LevelData&>(this->levelData);
}

void ExteriorWorldData::enterInterior(InteriorWorldData &&interior, const Int2 &returnVoxel)
{
	DebugAssert(this->interior.get() == nullptr);
	this->interior = std::make_unique<InteriorState>(std::move(interior), returnVoxel);
}

Int2 ExteriorWorldData::leaveInterior()
{
	DebugAssert(this->interior.get() != nullptr);

	const Int2 returnVoxel = this->interior->returnVoxel;
	this->interior = nullptr;

	return returnVoxel;
}
