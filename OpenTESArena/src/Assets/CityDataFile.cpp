#include <cstring>

#include "CityDataFile.h"

#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

const int CityDataFile::PROVINCE_COUNT = 9;

CityDataFile::CityDataFile(const std::string &filename)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Size of each province definition in bytes.
	const size_t provinceDataSize = 1228;

	// Size of each location definition in bytes.
	const size_t locationDataSize = 24;

	// Iterate over each province and initialize the location data.
	for (size_t i = 0; i < this->provinces.size(); i++)
	{
		const size_t startOffset = provinceDataSize * i;
		auto &province = this->provinces.at(i);

		// Read the province header.
		const uint8_t *provinceNamePtr = srcData.data() + startOffset;
		std::memcpy(province.name.data(), provinceNamePtr, province.name.size());
		std::memcpy(province.unknown.data(), 
			provinceNamePtr + province.name.size(), province.unknown.size());

		const uint8_t *locationPtr = provinceNamePtr + province.name.size() + 
			province.unknown.size();

		for (auto &cityState : province.cityStates)
		{
			// Read the city-state data (name, x, y).
			std::memcpy(cityState.name.data(), locationPtr, cityState.name.size());
			cityState.x = Bytes::getLE16(locationPtr + cityState.name.size());
			cityState.y = Bytes::getLE16(locationPtr + cityState.name.size() + sizeof(cityState.x));
			locationPtr += locationDataSize + 1; // Add 1 for null-terminator.
		}

		for (auto &town : province.towns)
		{
			// Read the town data (name, x, y).
			std::memcpy(town.name.data(), locationPtr, town.name.size());
			town.x = Bytes::getLE16(locationPtr + town.name.size());
			town.y = Bytes::getLE16(locationPtr + town.name.size() + sizeof(town.x));
			locationPtr += locationDataSize + 1;
		}

		for (auto &village : province.villages)
		{
			// Read the village data (name, x, y).
			std::memcpy(village.name.data(), locationPtr, village.name.size());
			village.x = Bytes::getLE16(locationPtr + village.name.size());
			village.y = Bytes::getLE16(locationPtr + village.name.size() + sizeof(village.x));
			locationPtr += locationDataSize + 1;
		}

		// Read the dungeon data (name, x, y). The second dungeon is listed first.
		std::memcpy(province.secondDungeon.name.data(), locationPtr, 
			province.secondDungeon.name.size());
		province.secondDungeon.x = Bytes::getLE16(locationPtr + province.secondDungeon.name.size());
		province.secondDungeon.y = Bytes::getLE16(locationPtr + 
			province.secondDungeon.name.size() + sizeof(province.secondDungeon.x));
		locationPtr += locationDataSize + 1;

		std::memcpy(province.firstDungeon.name.data(), locationPtr, 
			province.firstDungeon.name.size());
		province.firstDungeon.x = Bytes::getLE16(locationPtr + province.firstDungeon.name.size());
		province.firstDungeon.y = Bytes::getLE16(locationPtr +
			province.firstDungeon.name.size() + sizeof(province.firstDungeon.x));
		locationPtr += locationDataSize + 1;

		// Read unknown data.
		std::memcpy(province.unknownData.data.data(), locationPtr, province.unknownData.data.size());
	}
}

CityDataFile::~CityDataFile()
{

}

const CityDataFile::ProvinceData &CityDataFile::getProvinceData(int index) const
{
	DebugAssert(index < CityDataFile::PROVINCE_COUNT, "Province index out of range.");

	return this->provinces.at(index);
}
