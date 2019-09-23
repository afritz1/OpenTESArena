#include <algorithm>
#include <functional>

#include "LevelData.h"
#include "VoxelData.h"
#include "VoxelDataType.h"
#include "../Assets/ExeData.h"
#include "../Assets/INFFile.h"
#include "../Math/Constants.h"
#include "../Math/Random.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../World/WorldType.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/utilities/String.h"

LevelData::Lock::Lock(const Int2 &position, int lockLevel)
	: position(position)
{
	this->lockLevel = lockLevel;
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

LevelData::DoorState::DoorState(const Int2 &voxel, double percentOpen,
	DoorState::Direction direction)
	: voxel(voxel)
{
	this->percentOpen = percentOpen;
	this->direction = direction;
}

LevelData::DoorState::DoorState(const Int2 &voxel)
	: DoorState(voxel, 0.0, DoorState::Direction::Opening) { }

const Int2 &LevelData::DoorState::getVoxel() const
{
	return this->voxel;
}

double LevelData::DoorState::getPercentOpen() const
{
	return this->percentOpen;
}

bool LevelData::DoorState::isClosing() const
{
	return this->direction == Direction::Closing;
}

bool LevelData::DoorState::isClosed() const
{
	return this->percentOpen == 0.0;
}

void LevelData::DoorState::setDirection(DoorState::Direction direction)
{
	this->direction = direction;
}

void LevelData::DoorState::update(double dt)
{
	const double delta = DoorState::DEFAULT_SPEED * dt;

	// Decide how to change the door state depending on its current direction.
	if (this->direction == DoorState::Direction::Opening)
	{
		this->percentOpen = std::min(this->percentOpen + delta, 1.0);
		const bool isOpen = this->percentOpen == 1.0;

		if (isOpen)
		{
			this->direction = DoorState::Direction::None;
		}
	}
	else if (this->direction == DoorState::Direction::Closing)
	{
		this->percentOpen = std::max(this->percentOpen - delta, 0.0);

		if (this->isClosed())
		{
			this->direction = DoorState::Direction::None;
		}
	}
}

LevelData::LevelData(int gridWidth, int gridHeight, int gridDepth, const std::string &infName,
	const std::string &name)
	: voxelGrid(gridWidth, gridHeight, gridDepth), name(name)
{
	if (!this->inf.init(infName.c_str()))
	{
		DebugCrash("Could not init .INF file \"" + infName + "\".");
	}
}

LevelData::~LevelData()
{

}

const std::string &LevelData::getName() const
{
	return this->name;
}

double LevelData::getCeilingHeight() const
{
	return static_cast<double>(this->inf.getCeiling().height) / MIFFile::ARENA_UNITS;
}

std::vector<LevelData::DoorState> &LevelData::getOpenDoors()
{
	return this->openDoors;
}

const std::vector<LevelData::DoorState> &LevelData::getOpenDoors() const
{
	return this->openDoors;
}

const INFFile &LevelData::getInfFile() const
{
	return this->inf;
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
	return (lockIter != this->locks.end()) ? &lockIter->second : nullptr;
}

void LevelData::setVoxel(int x, int y, int z, uint16_t id)
{
	this->voxelGrid.setVoxel(x, y, z, id);
}

void LevelData::readFLOR(const uint16_t *flor, const INFFile &inf, int gridWidth, int gridDepth)
{
	// Lambda for obtaining a two-byte FLOR voxel.
	auto getFlorVoxel = [flor, gridWidth, gridDepth](int x, int z)
	{
		// Read voxel data in reverse order.
		const int index = (((gridDepth - 1) - z) * 2) + ((((gridWidth - 1) - x) * 2) * gridDepth);
		const uint16_t voxel = Bytes::getLE16(reinterpret_cast<const uint8_t*>(flor) + index);
		return voxel;
	};

	// Lambda for obtaining the voxel data index of a typical (non-chasm) FLOR voxel.
	auto getFlorDataIndex = [this](uint16_t florVoxel, int floorTextureID)
	{
		// See if the voxel already has a mapping.
		const auto floorIter = std::find_if(
			this->floorDataMappings.begin(), this->floorDataMappings.end(),
			[florVoxel](const std::pair<uint16_t, int> &pair)
		{
			return pair.first == florVoxel;
		});

		if (floorIter != this->floorDataMappings.end())
		{
			return floorIter->second;
		}
		else
		{
			// Insert new mapping.
			const int index = this->voxelGrid.addVoxelData(
				VoxelData::makeFloor(floorTextureID));
			this->floorDataMappings.push_back(std::make_pair(florVoxel, index));
			return index;
		}
	};

	using ChasmDataFunc = VoxelData(*)(const INFFile&, const std::array<bool, 4>&);

	// Lambda for obtaining the voxel data index of a chasm voxel. The given function argument
	// returns the created voxel data if there was no previous mapping.
	auto getChasmDataIndex = [this](uint16_t florVoxel, ChasmDataFunc chasmFunc,
		const INFFile &inf, const std::array<bool, 4> &adjacentFaces)
	{
		const auto chasmIter = std::find_if(
			this->chasmDataMappings.begin(), this->chasmDataMappings.end(),
			[florVoxel, &adjacentFaces](const auto &tuple)
		{
			return (std::get<0>(tuple) == florVoxel) && (std::get<1>(tuple) == adjacentFaces);
		});

		if (chasmIter != this->chasmDataMappings.end())
		{
			return std::get<2>(*chasmIter);
		}
		else
		{
			const int index = this->voxelGrid.addVoxelData(chasmFunc(inf, adjacentFaces));
			this->chasmDataMappings.push_back(std::make_tuple(florVoxel, adjacentFaces, index));
			return index;
		}
	};

	// Helper lambdas for creating each type of chasm voxel data.
	auto makeDryChasmVoxelData = [](const INFFile &inf, const std::array<bool, 4> &adjacentFaces)
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
				DebugLogWarning("Missing *DRYCHASM ID.");
				return 0;
			}
		}();

		DebugAssert(adjacentFaces.size() == 4);
		return VoxelData::makeChasm(dryChasmID,
			adjacentFaces[0], adjacentFaces[1], adjacentFaces[2], adjacentFaces[3],
			VoxelData::ChasmData::Type::Dry);
	};

	auto makeLavaChasmVoxelData = [](const INFFile &inf, const std::array<bool, 4> &adjacentFaces)
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
				DebugLogWarning("Missing *LAVACHASM ID.");
				return 0;
			}
		}();

		DebugAssert(adjacentFaces.size() == 4);
		return VoxelData::makeChasm(lavaChasmID,
			adjacentFaces[0], adjacentFaces[1], adjacentFaces[2], adjacentFaces[3],
			VoxelData::ChasmData::Type::Lava);
	};

	auto makeWetChasmVoxelData = [](const INFFile &inf, const std::array<bool, 4> &adjacentFaces)
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
				DebugLogWarning("Missing *WETCHASM ID.");
				return 0;
			}
		}();

		DebugAssert(adjacentFaces.size() == 4);
		return VoxelData::makeChasm(wetChasmID,
			adjacentFaces[0], adjacentFaces[1], adjacentFaces[2], adjacentFaces[3],
			VoxelData::ChasmData::Type::Wet);
	};

	// Write the voxel IDs into the voxel grid.
	for (int x = 0; x < gridWidth; x++)
	{
		for (int z = 0; z < gridDepth; z++)
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

			const uint16_t florVoxel = getFlorVoxel(x, z);
			const int floorTextureID = getFloorTextureID(florVoxel);

			// See if the floor voxel is either solid or a chasm.
			if (!isChasm(floorTextureID))
			{
				// Get the voxel data index associated with the floor value, or add it
				// if it doesn't exist yet.
				const int dataIndex = getFlorDataIndex(florVoxel, floorTextureID);
				this->setVoxel(x, 0, z, dataIndex);
			}
			else
			{
				// The voxel is a chasm. See which of its four faces are adjacent to
				// a solid floor voxel.
				const uint16_t northVoxel = getFlorVoxel(std::min(x + 1, gridWidth - 1), z);
				const uint16_t eastVoxel = getFlorVoxel(x, std::min(z + 1, gridDepth - 1));
				const uint16_t southVoxel = getFlorVoxel(std::max(x - 1, 0), z);
				const uint16_t westVoxel = getFlorVoxel(x, std::max(z - 1, 0));

				const std::array<bool, 4> adjacentFaces
				{
					!isChasm(getFloorTextureID(northVoxel)), // North.
					!isChasm(getFloorTextureID(eastVoxel)), // East.
					!isChasm(getFloorTextureID(southVoxel)), // South.
					!isChasm(getFloorTextureID(westVoxel)) // West.
				};

				if (floorTextureID == MIFFile::DRY_CHASM)
				{
					const int dataIndex = getChasmDataIndex(
						florVoxel, makeDryChasmVoxelData, inf, adjacentFaces);
					this->setVoxel(x, 0, z, dataIndex);
				}
				else if (floorTextureID == MIFFile::LAVA_CHASM)
				{
					const int dataIndex = getChasmDataIndex(
						florVoxel, makeLavaChasmVoxelData, inf, adjacentFaces);
					this->setVoxel(x, 0, z, dataIndex);
				}
				else if (floorTextureID == MIFFile::WET_CHASM)
				{
					const int dataIndex = getChasmDataIndex(
						florVoxel, makeWetChasmVoxelData, inf, adjacentFaces);
					this->setVoxel(x, 0, z, dataIndex);
				}
			}
		}
	}
}

void LevelData::readMAP1(const uint16_t *map1, const INFFile &inf, WorldType worldType,
	int gridWidth, int gridDepth, const ExeData &exeData)
{
	// Lambda for obtaining a two-byte MAP1 voxel.
	auto getMap1Voxel = [map1, gridWidth, gridDepth](int x, int z)
	{
		// Read voxel data in reverse order.
		const int index = (((gridDepth - 1) - z) * 2) + ((((gridWidth - 1) - x) * 2) * gridDepth);
		const uint16_t voxel = Bytes::getLE16(reinterpret_cast<const uint8_t*>(map1) + index);
		return voxel;
	};

	// Lambda for finding if there's an existing mapping for a MAP1 voxel.
	auto findWallMapping = [this](uint16_t map1Voxel)
	{
		return std::find_if(this->wallDataMappings.begin(), this->wallDataMappings.end(),
			[map1Voxel](const std::pair<uint16_t, int> &pair)
		{
			return pair.first == map1Voxel;
		});
	};

	// Lambda for obtaining the voxel data index of a general-case MAP1 object. The function
	// parameter returns the created voxel data if no previous mapping exists, and is intended
	// for general cases where the voxel data type does not need extra parameters.
	auto getDataIndex = [this, &findWallMapping](uint16_t map1Voxel, VoxelData (*func)(uint16_t))
	{
		const auto wallIter = findWallMapping(map1Voxel);
		if (wallIter != this->wallDataMappings.end())
		{
			return wallIter->second;
		}
		else
		{
			const int index = this->voxelGrid.addVoxelData(func(map1Voxel));
			this->wallDataMappings.push_back(std::make_pair(map1Voxel, index));
			return index;
		}
	};

	// Lambda for obtaining the voxel data index of a solid wall.
	auto getWallDataIndex = [this, &inf, &findWallMapping](uint16_t map1Voxel, uint8_t mostSigByte)
	{
		const auto wallIter = findWallMapping(map1Voxel);
		if (wallIter != this->wallDataMappings.end())
		{
			return wallIter->second;
		}
		else
		{
			// Lambda for creating a basic solid wall voxel data.
			auto makeWallVoxelData = [map1Voxel, &inf, mostSigByte]()
			{
				const int textureIndex = mostSigByte - 1;

				// Menu index if the voxel has the *MENU tag, or -1 if it is
				// not a *MENU voxel.
				const int menuIndex = inf.getMenuIndex(textureIndex);
				const bool isMenu = menuIndex != -1;

				// Determine what the type of the wall is (level up/down, menu, 
				// or just plain solid).
				const VoxelData::WallData::Type type = [&inf, textureIndex, isMenu]()
				{
					// Returns whether the given index pointer is non-null and
					// matches the current texture index.
					auto matchesIndex = [textureIndex](const int *index)
					{
						return (index != nullptr) && (*index == textureIndex);
					};

					if (matchesIndex(inf.getLevelUpIndex()))
					{
						return VoxelData::WallData::Type::LevelUp;
					}
					else if (matchesIndex(inf.getLevelDownIndex()))
					{
						return VoxelData::WallData::Type::LevelDown;
					}
					else if (isMenu)
					{
						return VoxelData::WallData::Type::Menu;
					}
					else
					{
						return VoxelData::WallData::Type::Solid;
					}
				}();

				VoxelData voxelData = VoxelData::makeWall(textureIndex, textureIndex, textureIndex,
					(isMenu ? &menuIndex : nullptr), type);

				// Set the *MENU index if it's a menu voxel.
				if (isMenu)
				{
					VoxelData::WallData &wallData = voxelData.wall;
					wallData.menuID = menuIndex;
				}

				return voxelData;
			};

			const int index = this->voxelGrid.addVoxelData(makeWallVoxelData());
			this->wallDataMappings.push_back(std::make_pair(map1Voxel, index));
			return index;
		}
	};

	// Lambda for obtaining the voxel data index of a raised platform.
	auto getRaisedDataIndex = [this, &inf, worldType, &exeData, &findWallMapping](
		uint16_t map1Voxel, uint8_t mostSigByte, int x, int z)
	{
		const auto wallIter = findWallMapping(map1Voxel);
		if (wallIter != this->wallDataMappings.end())
		{
			return wallIter->second;
		}
		else
		{
			// Lambda for creating a raised voxel data.
			auto makeRaisedVoxelData = [worldType, &exeData, map1Voxel, &inf, mostSigByte, x, z]()
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
						DebugLogWarning("Missing *BOXSIDE ID \"" +
							std::to_string(wallTextureID) + "\".");
						return 0;
					}
				}();

				const int floorID = [&inf, x, z]()
				{
					const auto &id = inf.getCeiling().textureIndex;

					if (id.has_value())
					{
						return id.value();
					}
					else
					{
						DebugLogWarning("Missing platform floor ID (" +
							std::to_string(x) + ", " + std::to_string(z) + ").");
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
						DebugLogWarning("Missing *BOXCAP ID \"" +
							std::to_string(capTextureID) + "\".");
						return 0;
					}
				}();

				const auto &wallHeightTables = exeData.wallHeightTables;
				const int heightIndex = mostSigByte & 0x07;
				const int thicknessIndex = (mostSigByte & 0x78) >> 3;
				int baseOffset, baseSize;

				if (worldType == WorldType::City)
				{
					baseOffset = wallHeightTables.box1b.at(heightIndex);
					baseSize = wallHeightTables.box2b.at(thicknessIndex);
				}
				else if (worldType == WorldType::Interior)
				{
					baseOffset = wallHeightTables.box1a.at(heightIndex);

					const int boxSize = wallHeightTables.box2a.at(thicknessIndex);
					const auto &boxScale = inf.getCeiling().boxScale;
					baseSize = boxScale.has_value() ?
						((boxSize * (*boxScale)) / 256) : boxSize;
				}
				else if (worldType == WorldType::Wilderness)
				{
					baseOffset = wallHeightTables.box1c.at(heightIndex);

					const int boxSize = 32;
					const auto &boxScale = inf.getCeiling().boxScale;
					baseSize = (boxSize *
						(boxScale.has_value() ? boxScale.value() : 192)) / 256;
				}
				else
				{
					throw DebugException("Invalid world type \"" +
						std::to_string(static_cast<int>(worldType)) + "\".");
				}

				const double yOffset =
					static_cast<double>(baseOffset) / MIFFile::ARENA_UNITS;
				const double ySize =
					static_cast<double>(baseSize) / MIFFile::ARENA_UNITS;

				const double normalizedScale =
					static_cast<double>(inf.getCeiling().height) /
					MIFFile::ARENA_UNITS;
				const double yOffsetNormalized = yOffset / normalizedScale;
				const double ySizeNormalized = ySize / normalizedScale;

				// @todo: might need some tweaking with box3/box4 values.
				const double vTop = std::max(
					0.0, 1.0 - yOffsetNormalized - ySizeNormalized);
				const double vBottom = std::min(vTop + ySizeNormalized, 1.0);

				return VoxelData::makeRaised(sideID, floorID, ceilingID,
					yOffsetNormalized, ySizeNormalized, vTop, vBottom);
			};

			const int index = this->voxelGrid.addVoxelData(makeRaisedVoxelData());
			this->wallDataMappings.push_back(std::make_pair(map1Voxel, index));
			return index;
		}
	};

	// Lambda for creating type 0x9 voxel data.
	auto makeType9VoxelData = [](uint16_t map1Voxel)
	{
		const int textureIndex = (map1Voxel & 0x00FF) - 1;
		const bool collider = (map1Voxel & 0x0100) == 0;
		return VoxelData::makeTransparentWall(textureIndex, collider);
	};

	// Lambda for obtaining the voxel data index of a type 0xA voxel.
	auto getTypeADataIndex = [this, worldType, &findWallMapping](
		uint16_t map1Voxel, int textureIndex)
	{
		const auto wallIter = findWallMapping(map1Voxel);
		if (wallIter != this->wallDataMappings.end())
		{
			return wallIter->second;
		}
		else
		{
			// Lambda for creating type 0xA voxel data.
			auto makeTypeAVoxelData = [worldType, map1Voxel, textureIndex]()
			{
				const double yOffset = [worldType, map1Voxel]()
				{
					const int baseOffset = (map1Voxel & 0x0E00) >> 9;
					const int fullOffset = (worldType == WorldType::Interior) ?
						(baseOffset * 8) : ((baseOffset * 32) - 8);

					return static_cast<double>(fullOffset) / MIFFile::ARENA_UNITS;
				}();

				const bool collider = (map1Voxel & 0x0100) != 0;

				// "Flipped" is not present in the original game, but has been added
				// here so that all edge voxel texture coordinates (i.e., palace
				// graphics, store signs) can be correct. Currently only palace
				// graphics and gates are type 0xA colliders, I believe.
				const bool flipped = collider;

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

				return VoxelData::makeEdge(
					textureIndex, yOffset, collider, flipped, facing);
			};

			const int index = this->voxelGrid.addVoxelData(makeTypeAVoxelData());
			this->wallDataMappings.push_back(std::make_pair(map1Voxel, index));
			return index;
		}
	};

	// Lambda for creating type 0xB voxel data.
	auto makeTypeBVoxelData = [](uint16_t map1Voxel)
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
				DebugUnhandledReturnMsg(
					VoxelData::DoorData::Type, std::to_string(type));
			}
		}();

		return VoxelData::makeDoor(textureIndex, doorType);
	};

	// Lambda for creating type 0xD voxel data.
	auto makeTypeDVoxelData = [](uint16_t map1Voxel)
	{
		const int textureIndex = (map1Voxel & 0x00FF) - 1;
		const bool isRightDiag = (map1Voxel & 0x0100) == 0;
		return VoxelData::makeDiagonal(textureIndex, isRightDiag);
	};

	// Write the voxel IDs into the voxel grid.
	for (int x = 0; x < gridWidth; x++)
	{
		for (int z = 0; z < gridDepth; z++)
		{
			const uint16_t map1Voxel = getMap1Voxel(x, z);

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
						const int dataIndex = getWallDataIndex(map1Voxel, mostSigByte);
						this->setVoxel(x, 1, z, dataIndex);
					}
					else
					{
						// Raised platform.
						const int dataIndex = getRaisedDataIndex(map1Voxel, mostSigByte, x, z);
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
					// @todo.
				}
				else if (mostSigNibble == 0x9)
				{
					// Transparent block with 1-sided texture on all sides, such as wooden 
					// arches in dungeons. These do not have back-faces (especially when 
					// standing in the voxel itself).
					const int dataIndex = getDataIndex(map1Voxel, makeType9VoxelData);
					this->setVoxel(x, 1, z, dataIndex);
				}
				else if (mostSigNibble == 0xA)
				{
					// Transparent block with 2-sided texture on one side (i.e., fence).
					const int textureIndex = (map1Voxel & 0x003F) - 1;

					// It is clamped non-negative due to a case in the center province's city
					// where one temple voxel has all zeroes for its texture index, and it
					// appears solid gray in the original game (presumably a silent bug).
					if (textureIndex >= 0)
					{
						const int dataIndex = getTypeADataIndex(map1Voxel, textureIndex);
						this->setVoxel(x, 1, z, dataIndex);
					}
				}
				else if (mostSigNibble == 0xB)
				{
					// Door voxel.
					const int dataIndex = getDataIndex(map1Voxel, makeTypeBVoxelData);
					this->setVoxel(x, 1, z, dataIndex);
				}
				else if (mostSigNibble == 0xC)
				{
					// Unknown.
					DebugLogWarning("Voxel type 0xC not implemented.");
				}
				else if (mostSigNibble == 0xD)
				{
					// Diagonal wall. Its type is determined by the nineth bit.
					const int dataIndex = getDataIndex(map1Voxel, makeTypeDVoxelData);
					this->setVoxel(x, 1, z, dataIndex);
				}
			}
		}
	}
}

void LevelData::readMAP2(const uint16_t *map2, const INFFile &inf, int gridWidth, int gridDepth)
{
	// Lambda for obtaining a two-byte MAP2 voxel.
	auto getMap2Voxel = [map2, gridWidth, gridDepth](int x, int z)
	{
		// Read voxel data in reverse order.
		const int index = (((gridDepth - 1) - z) * 2) + ((((gridWidth - 1) - x) * 2) * gridDepth);
		const uint16_t voxel = Bytes::getLE16(reinterpret_cast<const uint8_t*>(map2) + index);
		return voxel;
	};

	// Write the voxel IDs into the voxel grid.
	for (int x = 0; x < gridWidth; x++)
	{
		for (int z = 0; z < gridDepth; z++)
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

				const int dataIndex = [this, &inf, map2Voxel, height]()
				{
					const auto map2Iter = std::find_if(
						this->map2DataMappings.begin(), this->map2DataMappings.end(),
						[map2Voxel](const std::pair<uint16_t, int> &pair)
					{
						return pair.first == map2Voxel;
					});

					if (map2Iter != this->map2DataMappings.end())
					{
						return map2Iter->second;
					}
					else
					{
						const int textureIndex = (map2Voxel & 0x007F) - 1;
						const int *menuID = nullptr;
						const int index = this->voxelGrid.addVoxelData(VoxelData::makeWall(
							textureIndex, textureIndex, textureIndex, menuID,
							VoxelData::WallData::Type::Solid));
						this->map2DataMappings.push_back(std::make_pair(map2Voxel, index));
						return index;
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
		// @todo: get ceiling from .INFs without *CEILING (like START.INF). Maybe
		// hardcoding index 1 is enough?
		return ceiling.textureIndex.value_or(1);
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

void LevelData::readLocks(const std::vector<ArenaTypes::MIFLock> &locks, int width, int depth)
{
	for (const auto &lock : locks)
	{
		const Int2 lockPosition = VoxelGrid::getTransformedCoordinate(
			Int2(lock.x, lock.y), width, depth);
		this->locks.insert(std::make_pair(
			lockPosition, LevelData::Lock(lockPosition, lock.lockLevel)));
	}
}

void LevelData::setActive(TextureManager &textureManager, Renderer &renderer)
{
	// Clear all entities.
	// @todo: entities.
	/*for (const auto *entity : this->entityManager.getAllEntities())
	{
		renderer.removeFlat(entity->getID());
		this->entityManager.remove(entity->getID());
	}*/

	// Clear renderer textures and distant sky.
	renderer.clearTextures();
	renderer.clearDistantSky();

	// Load .INF voxel textures into the renderer.
	const int voxelTextureCount = static_cast<int>(this->inf.getVoxelTextures().size());
	for (int i = 0; i < voxelTextureCount; i++)
	{
		const auto &textureData = this->inf.getVoxelTextures().at(i);

		const std::string textureName = String::toUppercase(textureData.filename);
		const std::string extension = String::getExtension(textureName);

		const bool isIMG = extension == "IMG";
		const bool isSET = extension == "SET";
		const bool noExtension = extension.size() == 0;

		if (isSET)
		{
			// Use the texture data's .SET index to obtain the correct surface.
			const auto &surfaces = textureManager.getSurfaces(textureName);
			const Surface &surface = surfaces.at(textureData.setIndex.value());
			renderer.setVoxelTexture(i, static_cast<const uint32_t*>(surface.getPixels()));
		}
		else if (isIMG)
		{
			const Surface &surface = textureManager.getSurface(textureName);
			renderer.setVoxelTexture(i, static_cast<const uint32_t*>(surface.getPixels()));
		}
		else if (noExtension)
		{
			// Ignore texture names with no extension. They appear to be lore-related names
			// that were used at one point in Arena's development.
			static_cast<void>(textureData);
		}
		else
		{
			DebugCrash("Unrecognized voxel texture extension \"" + extension + "\".");
		}
	}

	// Load .INF flat textures into the renderer.
	// - @todo: maybe turn this into a while loop, so the index variable can be incremented
	//   by the size of each .DFA. It's incorrect as-is.
	/*const int flatTextureCount = static_cast<int>(inf.getFlatTextures().size());
	for (int i = 0; i < flatTextureCount; i++)
	{
		const auto &textureData = inf.getFlatTextures().at(i);
		const std::string textureName = String::toUppercase(textureData.filename);
		const std::string extension = String::getExtension(textureName);
		const bool isDFA = extension == "DFA";
		const bool isIMG = extension == "IMG";
		const bool noExtension = extension.size() == 0;
		if (isDFA)
		{
			// @todo: creatures don't have .DFA files (although they're referenced in the .INF
			// files), so I think the extension needs to be .CFA instead for them.
			//const auto &surfaces = textureManager.getSurfaces(textureName);
			//for (const auto *surface : surfaces)
			//{
			//renderer.addTexture(static_cast<const uint32_t*>(surface->pixels),
			//surface->w, surface->h);
			//}
		}
		else if (isIMG)
		{
			const SDL_Surface *surface = textureManager.getSurface(textureName);
			renderer.setFlatTexture(i, static_cast<const uint32_t*>(surface->pixels),
				surface->w, surface->h);
		}
		else if (noExtension)
		{
			// Ignore texture names with no extension. They appear to be lore-related names
			// that were used at one point in Arena's development.
			static_cast<void>(textureData);
		}
		else
		{
			DebugCrash("Unrecognized texture extension \"" + extension + "\".");
		}
	}*/
}

void LevelData::tick(double dt)
{
	// Do nothing by default.
	static_cast<void>(dt);
}
