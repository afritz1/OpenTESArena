#include <algorithm>

#include "ArenaCityUtils.h"
#include "ArenaWildUtils.h"
#include "MapType.h"
#include "../Assets/MIFFile.h"
#include "../Assets/RMDFile.h"
#include "../Math/Random.h"
#include "../Voxels/ArenaVoxelUtils.h"
#include "../Weather/ArenaWeatherUtils.h"
#include "../Weather/WeatherDefinition.h"

#include "components/debug/Debug.h"

std::string ArenaWildUtils::generateInfName(ArenaTypes::ClimateType climateType, const WeatherDefinition &weatherDef)
{
	const char climateLetter = [climateType]()
	{
		if (climateType == ArenaTypes::ClimateType::Temperate)
		{
			return 'T';
		}
		else if (climateType == ArenaTypes::ClimateType::Desert)
		{
			return 'D';
		}
		else if (climateType == ArenaTypes::ClimateType::Mountain)
		{
			return 'M';
		}
		else
		{
			DebugUnhandledReturnMsg(char, std::to_string(static_cast<int>(climateType)));
		}
	}();

	// Wilderness is "W".
	constexpr char locationLetter = 'W';

	const char weatherLetter = [climateType, &weatherDef]()
	{
		const WeatherDefinition::Type weatherDefType = weatherDef.getType();
		if ((weatherDefType == WeatherDefinition::Type::Clear) || (weatherDefType == WeatherDefinition::Type::Overcast))
		{
			return 'N';
		}
		else if (weatherDefType == WeatherDefinition::Type::Rain)
		{
			return 'R';
		}
		else if (weatherDefType == WeatherDefinition::Type::Snow)
		{
			// Deserts can't have snow.
			if (climateType != ArenaTypes::ClimateType::Desert)
			{
				return 'S';
			}
			else
			{
				DebugLogWarning("Deserts do not have snow templates.");
				return 'N';
			}
		}
		else
		{
			// Not sure what this means.
			return 'W';
		}
	}();

	return std::string { climateLetter, locationLetter, weatherLetter } + ".INF";
}

uint32_t ArenaWildUtils::makeWildChunkSeed(int wildX, int wildY)
{
	return (wildY << 16) + wildX;
}

Buffer2D<ArenaWildUtils::WildBlockID> ArenaWildUtils::generateWildernessIndices(uint32_t wildSeed,
	const ExeData::Wilderness &wildData)
{
	Buffer2D<ArenaWildUtils::WildBlockID> indices(ArenaWildUtils::WILD_WIDTH, ArenaWildUtils::WILD_HEIGHT);
	ArenaRandom random(wildSeed);

	// Generate a random wilderness .MIF index for each wilderness chunk.
	std::generate(indices.get(), indices.get() + (indices.getWidth() * indices.getHeight()),
		[&wildData, &random]()
	{
		// Determine the wilderness block list to draw from.
		const auto &blockList = [&wildData, &random]() -> const std::vector<WildBlockID>&
		{
			constexpr uint16_t normalVal = 0x6666;
			constexpr uint16_t villageVal = 0x4000;
			constexpr uint16_t dungeonVal = 0x2666;
			constexpr uint16_t tavernVal = 0x1999;
			int randVal = random.next();

			if (randVal < normalVal)
			{
				return wildData.normalBlocks;
			}
			else
			{
				randVal -= normalVal;
				if (randVal < villageVal)
				{
					return wildData.villageBlocks;
				}
				else
				{
					randVal -= villageVal;
					if (randVal < dungeonVal)
					{
						return wildData.dungeonBlocks;
					}
					else
					{
						randVal -= dungeonVal;
						if (randVal < tavernVal)
						{
							return wildData.tavernBlocks;
						}
						else
						{
							return wildData.templeBlocks;
						}
					}
				}
			}
		}();

		const int blockListIndex = (random.next() & 0xFF) % blockList.size();
		return blockList[blockListIndex];
	});

	// City indices in the center of the wilderness (WILD001.MIF, etc.).
	static_assert(ArenaWildUtils::WILD_WIDTH >= 2, "Can't fit city tiles in wild width.");
	static_assert(ArenaWildUtils::WILD_HEIGHT >= 2, "Can't fit city tiles in wild height.");
	constexpr WEInt cityX = (ArenaWildUtils::WILD_WIDTH / 2) - 1;
	constexpr SNInt cityY = (ArenaWildUtils::WILD_HEIGHT / 2) - 1;
	indices.set(cityX, cityY, 1);
	indices.set(cityX + 1, cityY, 2);
	indices.set(cityX, cityY + 1, 3);
	indices.set(cityX + 1, cityY + 1, 4);

	return indices;
}

bool ArenaWildUtils::isWildCityBlock(ArenaWildUtils::WildBlockID wildBlockID)
{
	return (wildBlockID >= 1) && (wildBlockID <= 4);
}

void ArenaWildUtils::reviseWildCityBlock(ArenaWildUtils::WildBlockID wildBlockID,
	BufferView2D<ArenaTypes::VoxelID> &flor, BufferView2D<ArenaTypes::VoxelID> &map1,
	BufferView2D<ArenaTypes::VoxelID> &map2, const LocationDefinition::CityDefinition &cityDef,
	const BinaryAssetLibrary &binaryAssetLibrary)
{
	DebugAssert(ArenaWildUtils::isWildCityBlock(wildBlockID));

	// Get city generation info -- the .MIF filename to load for the city skeleton.
	const std::string mifName = cityDef.mapFilename;
	MIFFile mif;
	if (!mif.init(mifName.c_str()))
	{
		DebugLogError("Couldn't init .MIF file \"" + mifName + "\".");
		return;
	}

	const MIFFile::Level &level = mif.getLevel(0);

	// Buffers for the city data. Copy the .MIF data into them.
	Buffer2D<ArenaTypes::VoxelID> cityFlor(mif.getWidth(), mif.getDepth());
	Buffer2D<ArenaTypes::VoxelID> cityMap1(mif.getWidth(), mif.getDepth());
	Buffer2D<ArenaTypes::VoxelID> cityMap2(mif.getWidth(), mif.getDepth());
	BufferView2D<ArenaTypes::VoxelID> cityFlorView(cityFlor.get(), cityFlor.getWidth(), cityFlor.getHeight());
	BufferView2D<ArenaTypes::VoxelID> cityMap1View(cityMap1.get(), cityMap1.getWidth(), cityMap1.getHeight());
	BufferView2D<ArenaTypes::VoxelID> cityMap2View(cityMap2.get(), cityMap2.getWidth(), cityMap2.getHeight());
	ArenaCityUtils::writeSkeleton(level, cityFlorView, cityMap1View, cityMap2View);

	// Run city generation if it's not a premade city. The center province's city does not have
	// any special generation -- the .MIF buffers are simply used as-is (with some simple palace
	// gate revisions done afterwards).
	const bool isPremadeCity = cityDef.premade;
	if (!isPremadeCity)
	{
		const int cityBlocksPerSide = cityDef.cityBlocksPerSide;
		const BufferView<const uint8_t> reservedBlocks(cityDef.reservedBlocks->data(),
			static_cast<int>(cityDef.reservedBlocks->size()));
		const OriginalInt2 blockStartPosition(cityDef.blockStartPosX, cityDef.blockStartPosY);
		const uint32_t citySeed = cityDef.citySeed;
		ArenaRandom random(citySeed);

		// Write generated city data into the temp city buffers.
		ArenaCityUtils::generateCity(citySeed, cityBlocksPerSide, mif.getWidth(), reservedBlocks,
			blockStartPosition, random, binaryAssetLibrary, cityFlor, cityMap1, cityMap2);
	}

	// Transform city voxels based on the wilderness rules.
	for (WEInt x = 0; x < mif.getWidth(); x++)
	{
		for (SNInt z = 0; z < mif.getDepth(); z++)
		{
			uint16_t &map1Voxel = cityMap1.get(x, z);
			uint16_t &map2Voxel = cityMap2.get(x, z);

			if ((map1Voxel & 0x8000) != 0)
			{
				map1Voxel = 0;
				map2Voxel = 0;
			}
			else
			{
				const bool isWall = (map1Voxel == 0x2F2F) || (map1Voxel == 0x2D2D) || (map1Voxel == 0x2E2E);
				if (!isWall)
				{
					map1Voxel = 0;
					map2Voxel = 0;
				}
				else
				{
					// Replace solid walls.
					if (map1Voxel == 0x2F2F)
					{
						map1Voxel = 0x3030;
						map2Voxel = 0x3030 | (map2Voxel & 0x8080);
					}
					else if (map1Voxel == 0x2D2D)
					{
						map1Voxel = 0x2F2F;
						map2Voxel = 0x3030 | (map2Voxel & 0x8080);
					}
					else if (map1Voxel == 0x2E2E)
					{
						map2Voxel = 0x3030 | (map2Voxel & 0x8080);
					}
				}
			}
		}
	}
	
	DebugAssert(flor.getWidth() == RMDFile::WIDTH);
	DebugAssert(flor.getWidth() == flor.getHeight());
	DebugAssert(flor.getWidth() == map1.getWidth());
	DebugAssert(flor.getWidth() == map2.getWidth());

	// Clear all voxels in the wild chunk.
	flor.fill(0);
	map1.fill(0);
	map2.fill(0);

	// Write city buffers into the wilderness. The city is most likely bigger than the wild chunk so this will
	// only write part of the city. Wild blocks are ordered like this from a top down view:
	// 2 1
	// 4 3
	const WEInt cityStartX = ((wildBlockID == 1) || (wildBlockID == 3)) ? 0 : RMDFile::WIDTH;
	const WEInt cityEndX = cityStartX + std::min(RMDFile::WIDTH, mif.getWidth() - cityStartX);
	const SNInt cityStartZ = ((wildBlockID == 1) || (wildBlockID == 2)) ? 0 : RMDFile::DEPTH;
	const SNInt cityEndZ = cityStartZ + std::min(RMDFile::DEPTH, mif.getDepth() - cityStartZ);
	for (SNInt cityZ = cityStartZ; cityZ < cityEndZ; cityZ++)
	{
		for (WEInt cityX = cityStartX; cityX < cityEndX; cityX++)
		{
			const WEInt chunkVoxelX = cityX - cityStartX;
			const SNInt chunkVoxelZ = cityZ - cityStartZ;
			const ArenaTypes::VoxelID cityFlorVoxel = cityFlor.get(cityX, cityZ);
			const ArenaTypes::VoxelID cityMap1Voxel = cityMap1.get(cityX, cityZ);
			const ArenaTypes::VoxelID cityMap2Voxel = cityMap2.get(cityX, cityZ);
			flor.set(chunkVoxelX, chunkVoxelZ, cityFlorVoxel);
			map1.set(chunkVoxelX, chunkVoxelZ, cityMap1Voxel);
			map2.set(chunkVoxelX, chunkVoxelZ, cityMap2Voxel);
		}
	}
}

OriginalInt2 ArenaWildUtils::getRelativeWildOrigin(const Int2 &voxel)
{
	return OriginalInt2(
		voxel.x - (voxel.x % (RMDFile::WIDTH * 2)),
		voxel.y - (voxel.y % (RMDFile::DEPTH * 2)));
}

WorldInt2 ArenaWildUtils::getCenteredWildOrigin(const WorldInt2 &voxel)
{
	return WorldInt2(
		(std::max(voxel.x - 32, 0) / RMDFile::WIDTH) * RMDFile::WIDTH,
		(std::max(voxel.y - 32, 0) / RMDFile::DEPTH) * RMDFile::DEPTH);
}

bool ArenaWildUtils::menuIsDisplayedInWildAutomap(int menuIndex)
{
	return (menuIndex != 0) && (menuIndex != 2) && (menuIndex != 3) && (menuIndex != 4) &&
		(menuIndex != 6) && (menuIndex != 7);
}
