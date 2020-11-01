#include <algorithm>

#include "CityLevelUtils.h"
#include "VoxelDataType.h"
#include "VoxelDefinition.h"
#include "VoxelGrid.h"
#include "WildLevelUtils.h"
#include "../Assets/RMDFile.h"
#include "../Math/Random.h"

Buffer2D<WildBlockID> WildLevelUtils::generateWildernessIndices(uint32_t wildSeed,
	const ExeData::Wilderness &wildData)
{
	Buffer2D<WildBlockID> indices(WildLevelUtils::WILD_WIDTH, WildLevelUtils::WILD_HEIGHT);
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
	static_assert(WildLevelUtils::WILD_WIDTH >= 2, "Can't fit city tiles in wild width.");
	static_assert(WildLevelUtils::WILD_HEIGHT >= 2, "Can't fit city tiles in wild height.");
	constexpr WEInt cityX = (WildLevelUtils::WILD_WIDTH / 2) - 1;
	constexpr SNInt cityY = (WildLevelUtils::WILD_HEIGHT / 2) - 1;
	indices.set(cityX, cityY, 1);
	indices.set(cityX + 1, cityY, 2);
	indices.set(cityX, cityY + 1, 3);
	indices.set(cityX + 1, cityY + 1, 4);

	return indices;
}

LevelUtils::MenuNamesList WildLevelUtils::generateWildChunkBuildingNames(
	const VoxelGrid &voxelGrid, const ExeData &exeData)
{
	LevelUtils::MenuNamesList menuNames;

	// Lambda for looping through main-floor voxels and generating names for *MENU blocks that
	// match the given menu type.
	auto generateNames = [&voxelGrid, &exeData, &menuNames](int wildX, int wildY,
		VoxelDefinition::WallData::MenuType menuType)
	{
		const uint32_t wildChunkSeed = (wildY << 16) + wildX;

		// Don't need hashInSeen() for the wilderness.

		// Lambdas for creating tavern and temple building names.
		auto createTavernName = [&exeData](int m, int n)
		{
			const auto &tavernPrefixes = exeData.cityGen.tavernPrefixes;
			const auto &tavernSuffixes = exeData.cityGen.tavernSuffixes;
			return tavernPrefixes.at(m) + ' ' + tavernSuffixes.at(n);
		};

		auto createTempleName = [&exeData](int model, int n)
		{
			const auto &templePrefixes = exeData.cityGen.templePrefixes;
			const auto &temple1Suffixes = exeData.cityGen.temple1Suffixes;
			const auto &temple2Suffixes = exeData.cityGen.temple2Suffixes;
			const auto &temple3Suffixes = exeData.cityGen.temple3Suffixes;

			const std::string &templeSuffix = [&temple1Suffixes, &temple2Suffixes,
				&temple3Suffixes, model, n]() -> const std::string&
			{
				if (model == 0)
				{
					return temple1Suffixes.at(n);
				}
				else if (model == 1)
				{
					return temple2Suffixes.at(n);
				}
				else
				{
					return temple3Suffixes.at(n);
				}
			}();

			// No extra whitespace needed, I think?
			return templePrefixes.at(model) + templeSuffix;
		};

		// The lambda called for each main-floor voxel in the area.
		auto tryGenerateBlockName = [&voxelGrid, &menuNames, wildX, wildY, menuType, wildChunkSeed,
			&createTavernName, &createTempleName](SNInt x, WEInt z)
		{
			ArenaRandom random(wildChunkSeed);

			// Make sure the coordinate math is done in the new coordinate system.
			const OriginalInt2 relativeOrigin(
				((RMDFile::DEPTH - 1) - wildX) * RMDFile::DEPTH,
				((RMDFile::WIDTH - 1) - wildY) * RMDFile::WIDTH);
			const NewInt2 dstPoint(
				relativeOrigin.y + (RMDFile::WIDTH - 1 - x),
				relativeOrigin.x + (RMDFile::DEPTH - 1 - z));

			// See if the current voxel is a *MENU block and matches the target menu type.
			const bool matchesTargetType = [&voxelGrid, menuType, &dstPoint]()
			{
				const bool isCity = false; // Wilderness only.
				const uint16_t voxelID = voxelGrid.getVoxel(dstPoint.x, 1, dstPoint.y);
				const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
				return (voxelDef.dataType == VoxelDataType::Wall) && voxelDef.wall.isMenu() &&
					(VoxelDefinition::WallData::getMenuType(voxelDef.wall.menuID, isCity) == menuType);
			}();

			if (matchesTargetType)
			{
				// Get the *MENU block's display name.
				const std::string name = [menuType, &random, &createTavernName, &createTempleName]()
				{
					if (menuType == VoxelDefinition::WallData::MenuType::Tavern)
					{
						// Tavern.
						const int m = random.next() % 23;
						const int n = random.next() % 23;
						return createTavernName(m, n);
					}
					else
					{
						// Temple.
						const int model = random.next() % 3;
						constexpr std::array<int, 3> ModelVars = { 5, 9, 10 };
						const int vars = ModelVars.at(model);
						const int n = random.next() % vars;
						return createTempleName(model, n);
					}
				}();

				menuNames.push_back(std::make_pair(dstPoint, std::move(name)));
			}
		};

		// Iterate blocks in the chunk in any order. They are order-independent in the wild.
		for (SNInt x = 0; x < RMDFile::DEPTH; x++)
		{
			for (WEInt z = 0; z < RMDFile::WIDTH; z++)
			{
				tryGenerateBlockName(x, z);
			}
		}
	};

	// Iterate over each wild chunk.
	for (int y = 0; y < WildLevelUtils::WILD_HEIGHT; y++)
	{
		for (int x = 0; x < WildLevelUtils::WILD_WIDTH; x++)
		{
			generateNames(x, y, VoxelDefinition::WallData::MenuType::Tavern);
			generateNames(x, y, VoxelDefinition::WallData::MenuType::Temple);
		}
	}

	return menuNames;
}

void WildLevelUtils::reviseWildernessCity(const LocationDefinition &locationDef,
	Buffer2D<uint16_t> &flor, Buffer2D<uint16_t> &map1, Buffer2D<uint16_t> &map2,
	const BinaryAssetLibrary &binaryAssetLibrary)
{
	// For now, assume the given buffers are for the entire 4096x4096 wilderness.
	// @todo: change to only care about 128x128 layers.
	DebugAssert(flor.getWidth() == (WildLevelUtils::WILD_WIDTH * RMDFile::WIDTH));
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
	CityLevelUtils::writeSkeleton(level, cityFlorView, cityMap1View, cityMap2View);

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
		CityLevelUtils::generateCity(citySeed, cityBlocksPerSide, mif.getWidth(), reservedBlocks,
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

OriginalInt2 WildLevelUtils::getRelativeWildOrigin(const Int2 &voxel)
{
	return OriginalInt2(
		voxel.x - (voxel.x % (RMDFile::WIDTH * 2)),
		voxel.y - (voxel.y % (RMDFile::DEPTH * 2)));
}

NewInt2 WildLevelUtils::getCenteredWildOrigin(const NewInt2 &voxel)
{
	return NewInt2(
		(std::max(voxel.x - 32, 0) / RMDFile::WIDTH) * RMDFile::WIDTH,
		(std::max(voxel.y - 32, 0) / RMDFile::DEPTH) * RMDFile::DEPTH);
}
