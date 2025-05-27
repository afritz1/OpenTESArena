#include <algorithm>
#include <cmath>

#include "CityDataFile.h"
#include "ExeData.h"
#include "../Math/Random.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/utilities/String.h"
#include "components/vfs/manager.hpp"

bool ArenaLocationData::isVisible() const
{
	return (this->visibility & 0x2) != 0;
}

void ArenaLocationData::setVisible(bool visible)
{
	this->visibility = visible ? 0x2 : 0;
}

Rect ArenaProvinceData::getGlobalRect() const
{
	return Rect(this->globalX, this->globalY, this->globalW, this->globalH);
}

const ArenaLocationData &ArenaProvinceData::getLocationData(int locationID) const
{
	if (locationID < 8)
	{
		const int index = locationID;
		DebugAssertIndex(this->cityStates, index);
		return this->cityStates[locationID];
	}
	else if (locationID < 16)
	{
		const int index = locationID - 8;
		DebugAssertIndex(this->towns, index);
		return this->towns[index];
	}
	else if (locationID < 32)
	{
		const int index = locationID - 16;
		DebugAssertIndex(this->villages, index);
		return this->villages[index];
	}
	else if (locationID == 32)
	{
		// Staff dungeon.
		return this->secondDungeon;
	}
	else if (locationID == 33)
	{
		// Staff map dungeon.
		return this->firstDungeon;
	}
	else if (locationID < 48)
	{
		// Named dungeon.
		const int index = locationID - 34;
		DebugAssertIndex(this->randomDungeons, index);
		return this->randomDungeons[index];
	}
	else
	{
		// @todo: change function return so we can use DebugUnhandledReturn().
		throw DebugException("Bad location ID \"" + std::to_string(locationID) + "\".");
	}
}

ArenaProvinceData &CityDataFile::getProvinceData(int index)
{
	DebugAssertIndex(this->provinces, index);
	return this->provinces[index];
}

const ArenaProvinceData &CityDataFile::getProvinceData(int index) const
{
	DebugAssertIndex(this->provinces, index);
	return this->provinces[index];
}

bool CityDataFile::init(const char *filename)
{
	Buffer<std::byte> src;
	if (!VFS::Manager::get().read(filename, &src))
	{
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.begin());

	// Iterate over each province and initialize the location data.
	for (size_t i = 0; i < std::size(this->provinces); i++)
	{
		auto &province = this->provinces[i];

		// Size of each province definition in bytes.
		const size_t provinceDataSize = 1228;
		const size_t startOffset = provinceDataSize * i;

		// Max number of characters in a province/location name (including null terminator).
		const size_t nameSize = 20;

		// Read the province header.
		const char *provinceNamePtr = reinterpret_cast<const char*>(srcPtr + startOffset);
		province.name = std::string(provinceNamePtr);

		const uint8_t *provinceGlobalDimsPtr =
			reinterpret_cast<const uint8_t*>(provinceNamePtr + nameSize);
		province.globalX = Bytes::getLE16(provinceGlobalDimsPtr);
		province.globalY = Bytes::getLE16(provinceGlobalDimsPtr + sizeof(uint16_t));
		province.globalW = Bytes::getLE16(provinceGlobalDimsPtr + (sizeof(uint16_t) * 2));
		province.globalH = Bytes::getLE16(provinceGlobalDimsPtr + (sizeof(uint16_t) * 3));

		const uint8_t *locationPtr = provinceGlobalDimsPtr + (sizeof(uint16_t) * 4);

		// Lambda for initializing the given location and pushing the location pointer forward.
		auto initLocation = [nameSize, &locationPtr](ArenaLocationData &locationData)
		{
			const char *locationNamePtr = reinterpret_cast<const char*>(locationPtr);
			locationData.name = std::string(locationNamePtr);

			const uint8_t *locationDimsPtr = locationPtr + nameSize;
			locationData.x = Bytes::getLE16(locationDimsPtr);
			locationData.y = Bytes::getLE16(locationDimsPtr + sizeof(uint16_t));
			locationData.visibility = *(locationDimsPtr + (sizeof(uint16_t) * 2));

			// Size of each location definition in bytes.
			const size_t locationDataSize = 25;
			locationPtr += locationDataSize;
		};

		for (auto &cityState : province.cityStates)
		{
			// Read the city-state data.
			initLocation(cityState);
		}

		for (auto &town : province.towns)
		{
			// Read the town data.
			initLocation(town);
		}

		for (auto &village : province.villages)
		{
			// Read the village data.
			initLocation(village);
		}

		// Read the dungeon data. The second dungeon is listed first.
		initLocation(province.secondDungeon);
		initLocation(province.firstDungeon);

		// Read random dungeon data.
		for (auto &dungeon : province.randomDungeons)
		{
			// Read the random dungeon data.
			initLocation(dungeon);
		}
	}

	return true;
}
