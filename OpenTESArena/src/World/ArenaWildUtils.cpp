#include <algorithm>

#include "ArenaCityUtils.h"
#include "ArenaVoxelUtils.h"
#include "ArenaWildUtils.h"
#include "ClimateType.h"
#include "LocationDefinition.h"
#include "MapType.h"
#include "VoxelDefinition.h"
#include "VoxelGrid.h"
#include "WeatherUtils.h"
#include "../Assets/MIFFile.h"
#include "../Assets/RMDFile.h"
#include "../Math/Random.h"

#include "components/debug/Debug.h"

std::string ArenaWildUtils::generateInfName(ClimateType climateType, WeatherType weatherType)
{
	const char climateLetter = [climateType]()
	{
		if (climateType == ClimateType::Temperate)
		{
			return 'T';
		}
		else if (climateType == ClimateType::Desert)
		{
			return 'D';
		}
		else if (climateType == ClimateType::Mountain)
		{
			return 'M';
		}
		else
		{
			DebugUnhandledReturnMsg(char, std::to_string(static_cast<int>(climateType)));
		}
	}();

	// Wilderness is "W".
	const char locationLetter = 'W';

	const char weatherLetter = [climateType, weatherType]()
	{
		if (WeatherUtils::isClear(weatherType) || WeatherUtils::isOvercast(weatherType))
		{
			return 'N';
		}
		else if (WeatherUtils::isRain(weatherType))
		{
			return 'R';
		}
		else if (WeatherUtils::isSnow(weatherType))
		{
			// Deserts can't have snow.
			if (climateType != ClimateType::Desert)
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

void ArenaWildUtils::reviseWildernessCity(const LocationDefinition &locationDef,
	Buffer2D<uint16_t> &flor, Buffer2D<uint16_t> &map1, Buffer2D<uint16_t> &map2,
	const BinaryAssetLibrary &binaryAssetLibrary)
{
	// For now, assume the given buffers are for the entire 4096x4096 wilderness.
	// @todo: change to only care about 128x128 layers.
	DebugAssert(flor.getWidth() == (ArenaWildUtils::WILD_WIDTH * RMDFile::WIDTH));
	DebugAssert(flor.getWidth() == flor.getHeight());
	DebugAssert(flor.getWidth() == map1.getWidth());
	DebugAssert(flor.getWidth() == map2.getWidth());

	// Clear all placeholder city blocks.
	constexpr int placeholderWidth = RMDFile::WIDTH * 2;
	constexpr int placeholderDepth = RMDFile::DEPTH * 2;

	// @todo: change to only care about 128x128 floors -- these should both be removed.
	constexpr WEInt xOffset = RMDFile::WIDTH * 31;
	constexpr SNInt zOffset = RMDFile::DEPTH * 31;

	for (WEInt x = 0; x < placeholderWidth; x++)
	{
		const int startIndex = zOffset + ((x + xOffset) * flor.getHeight());

		auto clearRow = [placeholderDepth, startIndex](Buffer2D<uint16_t> &dst)
		{
			const auto dstBegin = dst.get() + startIndex;
			const auto dstEnd = dstBegin + placeholderDepth;
			DebugAssert(dstEnd <= dst.end());
			std::fill(dstBegin, dstEnd, 0);
		};

		clearRow(flor);
		clearRow(map1);
		clearRow(map2);
	}

	// Get city generation info -- the .MIF filename to load for the city skeleton.
	const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
	const std::string mifName = cityDef.mapFilename;
	MIFFile mif;
	if (!mif.init(mifName.c_str()))
	{
		DebugLogError("Couldn't init .MIF file \"" + mifName + "\".");
		return;
	}

	const MIFFile::Level &level = mif.getLevel(0);

	// Buffers for the city data. Copy the .MIF data into them.
	Buffer2D<uint16_t> cityFlor(mif.getWidth(), mif.getDepth());
	Buffer2D<uint16_t> cityMap1(mif.getWidth(), mif.getDepth());
	Buffer2D<uint16_t> cityMap2(mif.getWidth(), mif.getDepth());
	BufferView2D<uint16_t> cityFlorView(cityFlor.get(), cityFlor.getWidth(), cityFlor.getHeight());
	BufferView2D<uint16_t> cityMap1View(cityMap1.get(), cityMap1.getWidth(), cityMap1.getHeight());
	BufferView2D<uint16_t> cityMap2View(cityMap2.get(), cityMap2.getWidth(), cityMap2.getHeight());
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

	// Write city buffers into the wilderness.
	for (SNInt z = 0; z < mif.getDepth(); z++)
	{
		for (WEInt x = 0; x < mif.getWidth(); x++)
		{
			const uint16_t srcFlorVoxel = cityFlor.get(x, z);
			const uint16_t srcMap1Voxel = cityMap1.get(x, z);
			const uint16_t srcMap2Voxel = cityMap2.get(x, z);
			const WEInt dstX = xOffset + x;
			const SNInt dstZ = zOffset + z;
			flor.set(dstX, dstZ, srcFlorVoxel);
			map1.set(dstX, dstZ, srcMap1Voxel);
			map2.set(dstX, dstZ, srcMap2Voxel);
		}
	}
}

OriginalInt2 ArenaWildUtils::getRelativeWildOrigin(const Int2 &voxel)
{
	return OriginalInt2(
		voxel.x - (voxel.x % (RMDFile::WIDTH * 2)),
		voxel.y - (voxel.y % (RMDFile::DEPTH * 2)));
}

NewInt2 ArenaWildUtils::getCenteredWildOrigin(const NewInt2 &voxel)
{
	return NewInt2(
		(std::max(voxel.x - 32, 0) / RMDFile::WIDTH) * RMDFile::WIDTH,
		(std::max(voxel.y - 32, 0) / RMDFile::DEPTH) * RMDFile::DEPTH);
}

bool ArenaWildUtils::menuIsDisplayedInWildAutomap(int menuIndex)
{
	return (menuIndex != 0) && (menuIndex != 2) && (menuIndex != 3) && (menuIndex != 4) &&
		(menuIndex != 6) && (menuIndex != 7);
}
