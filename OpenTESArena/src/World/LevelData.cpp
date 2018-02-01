#include <array>
#include <cassert>
#include <functional>

#include "LevelData.h"
#include "../Assets/INFFile.h"
#include "../Math/Constants.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"
#include "../World/VoxelType.h"

LevelData::Lock::Lock(const Int2 &position, int lockLevel)
	: position(position)
{
	this->lockLevel = lockLevel;
}

LevelData::Lock::~Lock()
{

}

const Int2 &LevelData::Lock::getPosition() const
{
	return this->position;
}

int LevelData::Lock::getLockLevel() const
{
	return this->lockLevel;
}

LevelData::TextTrigger::TextTrigger(const std::string &text, bool displayedOnce)
	: text(text)
{
	this->displayedOnce = displayedOnce;
	this->previouslyDisplayed = false;
}

LevelData::TextTrigger::~TextTrigger()
{

}

const std::string &LevelData::TextTrigger::getText() const
{
	return this->text;
}

bool LevelData::TextTrigger::isSingleDisplay() const
{
	return this->displayedOnce;
}

bool LevelData::TextTrigger::hasBeenDisplayed() const
{
	return this->previouslyDisplayed;
}

void LevelData::TextTrigger::setPreviouslyDisplayed(bool previouslyDisplayed)
{
	this->previouslyDisplayed = previouslyDisplayed;
}

LevelData::LevelData(int gridWidth, int gridHeight, int gridDepth)
	: voxelGrid(gridWidth, gridHeight, gridDepth)
{
	// Just for initializing grid dimensions. The rest is initialized by load methods.
}

LevelData::LevelData(VoxelGrid &&voxelGrid)
	: voxelGrid(std::move(voxelGrid))
{
	this->ceilingHeight = 1.0;
}

LevelData::~LevelData()
{

}

LevelData LevelData::loadInterior(const MIFFile::Level &level, int gridWidth, int gridDepth)
{
	// .INF file associated with the interior level.
	const INFFile inf(String::toUppercase(level.info));

	// Interior level.
	LevelData levelData(gridWidth, level.getHeight(), gridDepth);
	levelData.name = level.name;
	levelData.infName = inf.getName();
	levelData.ceilingHeight = static_cast<double>(inf.getCeiling().height) / MIFFile::ARENA_UNITS;
	levelData.outdoorDungeon = inf.getCeiling().outdoorDungeon;

	// Interior sky color (usually black, but also gray for "outdoor" dungeons).
	levelData.interiorSkyColor = std::unique_ptr<uint32_t>(new uint32_t(
		levelData.isOutdoorDungeon() ? Color::Gray.toARGB() : Color::Black.toARGB()));

	// Empty voxel data (for air).
	const int emptyID = levelData.voxelGrid.addVoxelData(VoxelData());

	// Load FLOR and MAP1 voxels.
	levelData.readFLOR(level.flor, inf, gridWidth, gridDepth);
	levelData.readMAP1(level.map1, inf, gridWidth, gridDepth);

	// All interiors have ceilings except some main quest dungeons which have a 1
	// as the third number after *CEILING in their .INF file.
	const bool hasCeiling = !inf.getCeiling().outdoorDungeon;

	// Fill the second floor with ceiling tiles if it's an "indoor dungeon". Otherwise,
	// leave it empty (for some "outdoor dungeons").
	if (hasCeiling)
	{
		levelData.readCeiling(inf, gridWidth, gridDepth);
	}

	// Assign locks.
	levelData.readLocks(level.lock, gridWidth, gridDepth);

	// Assign text and sound triggers.
	levelData.readTriggers(level.trig, inf, gridWidth, gridDepth);

	return levelData;
}

LevelData LevelData::loadPremadeCity(const MIFFile::Level &level, const INFFile &inf,
	int gridWidth, int gridDepth)
{
	// Premade exterior level (only used by center province).
	LevelData levelData(gridWidth, level.getHeight(), gridDepth);
	levelData.name = level.name;
	levelData.infName = inf.getName();
	levelData.ceilingHeight = 1.0;
	levelData.outdoorDungeon = false;

	// Empty voxel data (for air).
	const int emptyID = levelData.voxelGrid.addVoxelData(VoxelData());

	// Load FLOR, MAP1, and MAP2 voxels. No locks or triggers.
	levelData.readFLOR(level.flor, inf, gridWidth, gridDepth);
	levelData.readMAP1(level.map1, inf, gridWidth, gridDepth);
	levelData.readMAP2(level.map2, inf, gridWidth, gridDepth);

	return levelData;
}

LevelData LevelData::loadCity(const MIFFile::Level &level, const INFFile &inf,
	int gridWidth, int gridDepth)
{
	// To do: city generation.
	// - Load city skeleton from level parameter.
	// - Figure out required .MIF chunks from city ID (pass cityID parameter?).

	// Exterior level (skeleton + random chunks).
	LevelData levelData(gridWidth, level.getHeight(), gridDepth);
	levelData.name = level.name;
	levelData.infName = inf.getName();
	levelData.ceilingHeight = 1.0;
	levelData.outdoorDungeon = false;

	// Empty voxel data (for air).
	const int emptyID = levelData.voxelGrid.addVoxelData(VoxelData());

	// Load FLOR, MAP1, and MAP2 voxels. No locks or triggers.
	levelData.readFLOR(level.flor, inf, gridWidth, gridDepth);
	levelData.readMAP1(level.map1, inf, gridWidth, gridDepth);
	levelData.readMAP2(level.map2, inf, gridWidth, gridDepth);

	return levelData;
}

bool LevelData::isOutdoorDungeon() const
{
	return this->outdoorDungeon;
}

double LevelData::getCeilingHeight() const
{
	return this->ceilingHeight;
}

const std::string &LevelData::getName() const
{
	return this->name;
}

const std::string &LevelData::getInfName() const
{
	return this->infName;
}

uint32_t LevelData::getInteriorSkyColor() const
{
	assert(this->interiorSkyColor.get() != nullptr);
	return *this->interiorSkyColor.get();
}

VoxelGrid &LevelData::getVoxelGrid()
{
	return this->voxelGrid;
}

const VoxelGrid &LevelData::getVoxelGrid() const
{
	return this->voxelGrid;
}

const LevelData::Lock *LevelData::getLock(const Int2 &voxel) const
{
	const auto lockIter = this->locks.find(voxel);
	return (lockIter != this->locks.end()) ? (&lockIter->second) : nullptr;
}

LevelData::TextTrigger *LevelData::getTextTrigger(const Int2 &voxel)
{
	const auto textIter = this->textTriggers.find(voxel);
	return (textIter != this->textTriggers.end()) ? (&textIter->second) : nullptr;
}

const std::string *LevelData::getSoundTrigger(const Int2 &voxel) const
{
	const auto soundIter = this->soundTriggers.find(voxel);
	return (soundIter != this->soundTriggers.end()) ? (&soundIter->second) : nullptr;
}

void LevelData::setVoxel(int x, int y, int z, uint8_t id)
{
	uint8_t *voxels = this->voxelGrid.getVoxels();
	const int index = x + (y * this->voxelGrid.getWidth()) +
		(z * this->voxelGrid.getWidth() * this->voxelGrid.getHeight());

	voxels[index] = id;
}

// Hash specialization for readFLOR() chasm mappings.
namespace std
{
	template <>
	struct hash<std::pair<uint16_t, std::array<bool, 4>>>
	{
		size_t operator()(const std::pair<uint16_t, std::array<bool, 4>> &p) const
		{
			// XOR with some arbitrary prime numbers (not sure if this is any good).
			return static_cast<size_t>(p.first ^
				(p.second.at(0) ? 41 : 73) ^
				(p.second.at(1) ? 89 : 113) ^
				(p.second.at(2) ? 127 : 149) ^
				(p.second.at(3) ? 157 : 193));
		}
	};
}

void LevelData::readFLOR(const std::vector<uint8_t> &flor, const INFFile &inf,
	int width, int depth)
{
	// Lambda for obtaining a two-byte FLOR voxel.
	auto getFloorVoxel = [&flor, width, depth](int x, int z)
	{
		// Read voxel data in reverse order.
		const int index = (((depth - 1) - z) * 2) + ((((width - 1) - x) * 2) * depth);
		const uint16_t voxel = Bytes::getLE16(flor.data() + index);
		return voxel;
	};

	// Mappings of floor and chasm IDs to voxel data indices. Chasms are treated
	// separately since their voxel data index is also a function of the four
	// adjacent voxels.
	std::unordered_map<uint16_t, int> floorDataMappings;
	std::unordered_map<std::pair<uint16_t, std::array<bool, 4>>, int> chasmDataMappings;

	// Write the .MIF file's voxel IDs into the voxel grid.
	for (int x = 0; x < width; x++)
	{
		for (int z = 0; z < depth; z++)
		{
			auto getFloorTextureID = [](uint16_t voxel)
			{
				return (voxel & 0xFF00) >> 8;
			};

			auto isChasm = [](int id)
			{
				return (id == MIFFile::DRY_CHASM) ||
					(id == MIFFile::LAVA_CHASM) ||
					(id == MIFFile::WET_CHASM);
			};

			const uint16_t florVoxel = getFloorVoxel(x, z);
			const int floorTextureID = getFloorTextureID(florVoxel);

			// See if the floor voxel is either solid or a chasm.
			if (!isChasm(floorTextureID))
			{
				// Get the voxel data index associated with the floor value, or add it
				// if it doesn't exist yet.
				const int dataIndex = [this, &floorDataMappings, florVoxel, floorTextureID]()
				{
					const auto floorIter = floorDataMappings.find(florVoxel);
					if (floorIter != floorDataMappings.end())
					{
						return floorIter->second;
					}
					else
					{
						const int index = this->voxelGrid.addVoxelData(
							VoxelData::makeFloor(floorTextureID));
						return floorDataMappings.insert(
							std::make_pair(florVoxel, index)).first->second;
					}
				}();

				this->setVoxel(x, 0, z, dataIndex);
			}
			else
			{
				// The voxel is a chasm. See which of its four faces are adjacent to
				// a solid floor voxel.
				const uint16_t northVoxel = getFloorVoxel(std::min(x + 1, width - 1), z);
				const uint16_t eastVoxel = getFloorVoxel(x, std::min(z + 1, depth - 1));
				const uint16_t southVoxel = getFloorVoxel(std::max(x - 1, 0), z);
				const uint16_t westVoxel = getFloorVoxel(x, std::max(z - 1, 0));

				const std::array<bool, 4> adjacentFaces
				{
					!isChasm(getFloorTextureID(northVoxel)), // North.
					!isChasm(getFloorTextureID(eastVoxel)), // East.
					!isChasm(getFloorTextureID(southVoxel)), // South.
					!isChasm(getFloorTextureID(westVoxel)) // West.
				};

				// Lambda for obtaining the index of a newly-added VoxelData object, and
				// inserting it into the chasm data mappings if it hasn't been already. The
				// function parameter decodes the voxel and returns the created VoxelData.
				auto getChasmDataIndex = [this, &inf, &chasmDataMappings, florVoxel,
					&adjacentFaces](const std::function<VoxelData(void)> &function)
				{
					const auto chasmPair = std::make_pair(florVoxel, adjacentFaces);
					const auto chasmIter = chasmDataMappings.find(chasmPair);
					if (chasmIter != chasmDataMappings.end())
					{
						return chasmIter->second;
					}
					else
					{
						const int index = this->voxelGrid.addVoxelData(function());
						return chasmDataMappings.insert(
							std::make_pair(chasmPair, index)).first->second;
					}
				};

				if (floorTextureID == MIFFile::DRY_CHASM)
				{
					const int dataIndex = getChasmDataIndex(
						[&inf, floorTextureID, &adjacentFaces]()
					{
						const int dryChasmID = [&inf]()
						{
							const int *ptr = inf.getDryChasmIndex();
							if (ptr != nullptr)
							{
								return *ptr;
							}
							else
							{
								DebugWarning("Missing *DRYCHASM ID.");
								return 0;
							}
						}();

						return VoxelData::makeChasm(
							dryChasmID,
							adjacentFaces.at(0),
							adjacentFaces.at(1),
							adjacentFaces.at(2),
							adjacentFaces.at(3),
							VoxelData::ChasmData::Type::Dry);
					});

					this->setVoxel(x, 0, z, dataIndex);
				}
				else if (floorTextureID == MIFFile::LAVA_CHASM)
				{
					const int dataIndex = getChasmDataIndex(
						[&inf, floorTextureID, &adjacentFaces]()
					{
						const int lavaChasmID = [&inf]()
						{
							const int *ptr = inf.getLavaChasmIndex();
							if (ptr != nullptr)
							{
								return *ptr;
							}
							else
							{
								DebugWarning("Missing *LAVACHASM ID.");
								return 0;
							}
						}();

						return VoxelData::makeChasm(
							lavaChasmID,
							adjacentFaces.at(0),
							adjacentFaces.at(1),
							adjacentFaces.at(2),
							adjacentFaces.at(3),
							VoxelData::ChasmData::Type::Lava);
					});

					this->setVoxel(x, 0, z, dataIndex);
				}
				else if (floorTextureID == MIFFile::WET_CHASM)
				{
					const int dataIndex = getChasmDataIndex(
						[&inf, floorTextureID, &adjacentFaces]()
					{
						const int wetChasmID = [&inf]()
						{
							const int *ptr = inf.getWetChasmIndex();
							if (ptr != nullptr)
							{
								return *ptr;
							}
							else
							{
								DebugWarning("Missing *WETCHASM ID.");
								return 0;
							}
						}();

						return VoxelData::makeChasm(
							wetChasmID,
							adjacentFaces.at(0),
							adjacentFaces.at(1),
							adjacentFaces.at(2),
							adjacentFaces.at(3),
							VoxelData::ChasmData::Type::Wet);
					});

					this->setVoxel(x, 0, z, dataIndex);
				}
			}
		}
	}
}

void LevelData::readMAP1(const std::vector<uint8_t> &map1, const INFFile &inf,
	int width, int depth)
{
	// Lambda for obtaining a two-byte MAP1 voxel.
	auto getMap1Voxel = [&map1, width, depth](int x, int z)
	{
		// Read voxel data in reverse order.
		const int index = (((depth - 1) - z) * 2) + ((((width - 1) - x) * 2) * depth);
		const uint16_t voxel = Bytes::getLE16(map1.data() + index);
		return voxel;
	};

	// Mappings of wall IDs to voxel data indices.
	std::unordered_map<uint16_t, int> wallDataMappings;

	// Write the .MIF file's voxel IDs into the voxel grid.
	for (int x = 0; x < width; x++)
	{
		for (int z = 0; z < depth; z++)
		{
			const uint16_t map1Voxel = getMap1Voxel(x, z);

			// Lambda for obtaining the index of a newly-added VoxelData object, and inserting
			// it into the data mappings if it hasn't been already. The function parameter
			// decodes the voxel and returns the created VoxelData.
			auto getDataIndex = [this, &inf, &wallDataMappings, map1Voxel](
				const std::function<VoxelData(void)> &function)
			{
				const auto wallIter = wallDataMappings.find(map1Voxel);
				if (wallIter != wallDataMappings.end())
				{
					return wallIter->second;
				}
				else
				{
					const int index = this->voxelGrid.addVoxelData(function());
					return wallDataMappings.insert(
						std::make_pair(map1Voxel, index)).first->second;
				}
			};

			if ((map1Voxel & 0x8000) == 0)
			{
				// A voxel of some kind.
				const bool voxelIsEmpty = map1Voxel == 0;

				if (!voxelIsEmpty)
				{
					const uint8_t mostSigByte = (map1Voxel & 0x7F00) >> 8;
					const uint8_t leastSigByte = map1Voxel & 0x007F;
					const bool voxelIsSolid = mostSigByte == leastSigByte;

					if (voxelIsSolid)
					{
						// Regular solid wall.
						const int dataIndex = getDataIndex([&inf, mostSigByte]()
						{
							const int textureIndex = mostSigByte - 1;

							// Menu index if the voxel has the *MENU tag, or -1 if it is
							// not a *MENU voxel.
							const int menuIndex = inf.getMenuIndex(textureIndex);
							const bool isMenu = menuIndex != -1;

							// Determine what the type of the voxel is (level up/down, menu,
							// transition, etc.).
							const VoxelType type = [&inf, textureIndex, isMenu]()
							{
								// Returns whether the given index pointer is non-null and
								// matches the current texture index.
								auto matchesIndex = [textureIndex](const int *index)
								{
									return (index != nullptr) && (*index == textureIndex);
								};

								if (matchesIndex(inf.getLevelUpIndex()))
								{
									return VoxelType::LevelUp;
								}
								else if (matchesIndex(inf.getLevelDownIndex()))
								{
									return VoxelType::LevelDown;
								}
								else if (isMenu)
								{
									return VoxelType::Menu;
								}
								else
								{
									return VoxelType::Solid;
								}
							}();

							VoxelData voxelData = VoxelData::makeWall(
								textureIndex, textureIndex, textureIndex, type);

							// Set the *MENU index if it's a menu voxel.
							if (isMenu)
							{
								VoxelData::WallData &wallData = voxelData.wall;
								wallData.menuID = menuIndex;
							}

							return voxelData;
						});

						this->setVoxel(x, 1, z, dataIndex);
					}
					else
					{
						// Raised platform.
						const int dataIndex = getDataIndex([&inf, map1Voxel, mostSigByte]()
						{
							const uint8_t wallTextureID = map1Voxel & 0x000F;
							const uint8_t capTextureID = (map1Voxel & 0x00F0) >> 4;

							const int sideID = [&inf, wallTextureID]()
							{
								const int *ptr = inf.getBoxSide(wallTextureID);
								if (ptr != nullptr)
								{
									return *ptr;
								}
								else
								{
									DebugWarning("Missing *BOXSIDE ID \"" +
										std::to_string(wallTextureID) + "\".");
									return 0;
								}
							}();

							const int floorID = [&inf]()
							{
								const int id = inf.getCeiling().textureIndex;

								if (id >= 0)
								{
									return id;
								}
								else
								{
									DebugWarning("Invalid platform floor ID \"" +
										std::to_string(id) + "\".");
									return 0;
								}
							}();

							const int ceilingID = [&inf, capTextureID]()
							{
								const int *ptr = inf.getBoxCap(capTextureID);
								if (ptr != nullptr)
								{
									return *ptr;
								}
								else
								{
									DebugWarning("Missing *BOXCAP ID \"" +
										std::to_string(capTextureID) + "\".");
									return 0;
								}
							}();

							// To do: The height appears to be some fraction of 64, and 
							// when it's greater than 64, then that determines the offset?
							const double platformHeight = static_cast<double>(mostSigByte) /
								static_cast<double>(MIFFile::ARENA_UNITS);

							const double yOffset = 0.0;
							const double ySize = platformHeight;

							// To do: Clamp top V coordinate positive until the correct platform 
							// height calculation is figured out. Maybe the platform height
							// needs to be multiplied by the ratio between the current ceiling
							// height and the default ceiling height (128)? I.e., multiply by
							// "ceilingHeight"?
							const double vTop = std::max(0.0, 1.0 - platformHeight);
							const double vBottom = Constants::JustBelowOne; // To do: should also be a function.

							return VoxelData::makeRaised(sideID, floorID, ceilingID,
								yOffset, ySize, vTop, vBottom);
						});

						this->setVoxel(x, 1, z, dataIndex);
					}
				}
			}
			else
			{
				// A special voxel, or an object of some kind.
				const uint8_t mostSigNibble = (map1Voxel & 0xF000) >> 12;

				if (mostSigNibble == 0x8)
				{
					// The lower byte determines the index of a FLAT for an object.
					const uint8_t flatIndex = map1Voxel & 0x00FF;
					// To do.
				}
				else if (mostSigNibble == 0x9)
				{
					// Transparent block with 1-sided texture on all sides, such as wooden 
					// arches in dungeons. These do not have back-faces (especially when 
					// standing in the voxel itself).
					const int dataIndex = getDataIndex([map1Voxel]()
					{
						const int textureIndex = (map1Voxel & 0x00FF) - 1;
						const bool collider = (map1Voxel & 0x0100) == 0;
						return VoxelData::makeTransparentWall(textureIndex, collider);
					});

					this->setVoxel(x, 1, z, dataIndex);
				}
				else if (mostSigNibble == 0xA)
				{
					// Transparent block with 2-sided texture on one side (i.e., fence).
					const int textureIndex = (map1Voxel & 0x003F) - 1;

					// It is clamped non-negative due to a case in IMPERIAL.MIF where one temple
					// voxel has all zeroes for its texture index, and it appears solid gray
					// in the original game (presumably a silent bug).
					if (textureIndex >= 0)
					{
						const int dataIndex = getDataIndex([map1Voxel, textureIndex]()
						{
							const double yOffset =
								static_cast<double>((map1Voxel & 0x0E00) >> 8) / 7.0;
							const bool collider = (map1Voxel & 0x0100) != 0;

							const VoxelData::Facing facing = [map1Voxel]()
							{
								// Orientation is a multiple of 4 (0, 4, 8, C), where 0 is north
								// and C is east. It is stored in two bits above the texture index.
								const int orientation = (map1Voxel & 0x00C0) >> 4;
								if (orientation == 0x0)
								{
									return VoxelData::Facing::PositiveX;
								}
								else if (orientation == 0x4)
								{
									return VoxelData::Facing::NegativeZ;
								}
								else if (orientation == 0x8)
								{
									return VoxelData::Facing::NegativeX;
								}
								else
								{
									return VoxelData::Facing::PositiveZ;
								}
							}();

							return VoxelData::makeEdge(textureIndex, yOffset, collider, facing);
						});

						this->setVoxel(x, 1, z, dataIndex);
					}
				}
				else if (mostSigNibble == 0xB)
				{
					// Door voxel.
					const int dataIndex = getDataIndex([map1Voxel]()
					{
						const int textureIndex = (map1Voxel & 0x003F) - 1;
						const VoxelData::DoorData::Type doorType = [map1Voxel]()
						{
							const int type = (map1Voxel & 0x00C0) >> 4;
							if (type == 0x0)
							{
								return VoxelData::DoorData::Type::Swinging;
							}
							else if (type == 0x4)
							{
								return VoxelData::DoorData::Type::Sliding;
							}
							else if (type == 0x8)
							{
								return VoxelData::DoorData::Type::Raising;
							}
							else
							{
								// I don't believe any doors in Arena split (but they are
								// supported by the engine).
								throw std::runtime_error("Bad door type \"" +
									std::to_string(type) + "\".");
							}
						}();

						return VoxelData::makeDoor(textureIndex, doorType);
					});

					this->setVoxel(x, 1, z, dataIndex);
				}
				else if (mostSigNibble == 0xC)
				{
					// Unknown.
					DebugWarning("Voxel type 0xC not implemented.");
				}
				else if (mostSigNibble == 0xD)
				{
					// Diagonal wall. Its type is determined by the nineth bit.
					const int dataIndex = getDataIndex([map1Voxel]()
					{
						const int textureIndex = (map1Voxel & 0x00FF) - 1;
						const bool isRightDiag = (map1Voxel & 0x0100) == 0;
						return VoxelData::makeDiagonal(textureIndex, isRightDiag);
					});

					this->setVoxel(x, 1, z, dataIndex);
				}
			}
		}
	}
}

void LevelData::readMAP2(const std::vector<uint8_t> &map2, const INFFile &inf,
	int width, int depth)
{
	const uint8_t *map2Data = map2.data();

	// Lambda for obtaining a two-byte MAP2 voxel.
	auto getMap2Voxel = [&map2, width, depth](int x, int z)
	{
		// Read voxel data in reverse order.
		const int index = (((depth - 1) - z) * 2) + ((((width - 1) - x) * 2) * depth);
		const uint16_t voxel = Bytes::getLE16(map2.data() + index);
		return voxel;
	};

	// Mappings of second floor IDs to voxel data indices.
	std::unordered_map<uint16_t, int> map2DataMappings;

	// Write the .MIF file's voxel IDs into the voxel grid.
	for (int x = 0; x < width; x++)
	{
		for (int z = 0; z < depth; z++)
		{
			const uint16_t map2Voxel = getMap2Voxel(x, z);

			if (map2Voxel != 0)
			{
				// Number of blocks to extend upwards (including second story).
				const int height = [map2Voxel]()
				{
					if ((map2Voxel & 0x80) == 0x80)
					{
						return 2;
					}
					else if ((map2Voxel & 0x8000) == 0x8000)
					{
						return 3;
					}
					else if ((map2Voxel & 0x8080) == 0x8080)
					{
						return 4;
					}
					else
					{
						return 1;
					}
				}();

				const int dataIndex = [this, &inf, &map2DataMappings, map2Voxel, height]()
				{
					const auto map2Iter = map2DataMappings.find(map2Voxel);
					if (map2Iter != map2DataMappings.end())
					{
						return map2Iter->second;
					}
					else
					{
						const int textureIndex = (map2Voxel & 0x007F) - 1;
						const int index = this->voxelGrid.addVoxelData(VoxelData::makeWall(
							textureIndex, textureIndex, textureIndex, VoxelType::Solid));
						return map2DataMappings.insert(
							std::make_pair(map2Voxel, index)).first->second;
					}
				}();

				for (int y = 2; y < (height + 2); y++)
				{
					this->setVoxel(x, y, z, dataIndex);
				}
			}
		}
	}
}

void LevelData::readCeiling(const INFFile &inf, int width, int depth)
{
	const INFFile::CeilingData &ceiling = inf.getCeiling();

	// Get the index of the ceiling texture name in the textures array.
	const int ceilingIndex = [&ceiling]()
	{
		if (ceiling.textureIndex != INFFile::NO_INDEX)
		{
			return ceiling.textureIndex;
		}
		else
		{
			// To do: get ceiling from .INFs without *CEILING (like START.INF). Maybe
			// hardcoding index 1 is enough?
			return 1;
		}
	}();

	// Define the ceiling voxel data.
	const int index = this->voxelGrid.addVoxelData(
		VoxelData::makeCeiling(ceilingIndex));

	// Set all the ceiling voxels.
	for (int x = 0; x < width; x++)
	{
		for (int z = 0; z < depth; z++)
		{
			this->setVoxel(x, 2, z, index);
		}
	}
}

void LevelData::readLocks(const std::vector<MIFFile::Level::Lock> &locks, int width, int depth)
{
	for (const auto &lock : locks)
	{
		const Int2 lockPosition = VoxelGrid::getTransformedCoordinate(
			Int2(lock.x, lock.y), width, depth);
		this->locks.insert(std::make_pair(
			lockPosition, LevelData::Lock(lockPosition, lock.lockLevel)));
	}
}

void LevelData::readTriggers(const std::vector<MIFFile::Level::Trigger> &triggers,
	const INFFile &inf, int width, int depth)
{
	for (const auto &trigger : triggers)
	{
		// Transform the voxel coordinates from the Arena layout to the new layout.
		const Int2 voxel = VoxelGrid::getTransformedCoordinate(
			Int2(trigger.x, trigger.y), width, depth);

		// There can be a text trigger and sound trigger in the same voxel.
		const bool isTextTrigger = trigger.textIndex != -1;
		const bool isSoundTrigger = trigger.soundIndex != -1;

		// Make sure the text index points to a text value (i.e., not a key or riddle).
		if (isTextTrigger && inf.hasTextIndex(trigger.textIndex))
		{
			const INFFile::TextData &textData = inf.getText(trigger.textIndex);
			this->textTriggers.insert(std::make_pair(
				voxel, TextTrigger(textData.text, textData.displayedOnce)));
		}

		if (isSoundTrigger)
		{
			this->soundTriggers.insert(std::make_pair(voxel, inf.getSound(trigger.soundIndex)));
		}
	}
}
