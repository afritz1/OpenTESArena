#include <array>

#include "LevelData.h"
#include "../Assets/INFFile.h"
#include "../Math/Constants.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"
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

LevelData::LevelData(const MIFFile::Level &level, const INFFile &inf,
	int gridWidth, int gridDepth, bool isInterior)
	: voxelGrid(gridWidth, level.getHeight(), gridDepth)
{
	// Arena's level origins start at the top-right corner of the map, so X increases 
	// going to the left, and Z increases going down. The wilderness uses this same 
	// pattern. Each chunk looks like this:
	// +++++++ <- Origin (0, 0)
	// +++++++
	// +++++++
	// +++++++
	// ^
	// |
	// Max (mapWidth - 1, mapDepth - 1)

	// Ceiling height of the .INF file.
	this->ceilingHeight = static_cast<double>(inf.getCeiling().height) / MIFFile::ARENA_UNITS;

	// Empty voxel data (for air).
	const int emptyID = voxelGrid.addVoxelData(VoxelData());

	// Load FLOR and MAP1 voxels.
	this->readFLOR(level.flor, gridWidth, gridDepth, inf);
	this->readMAP1(level.map1, gridWidth, gridDepth, inf);

	// Fill the second floor with the ceiling tiles if it's an interior location, or MAP2 if 
	// it's an exterior location.
	if (isInterior)
	{
		const INFFile::CeilingData &ceiling = inf.getCeiling();

		// Get the index of the ceiling texture name in the textures array.
		const int ceilingIndex = [&inf, &ceiling]()
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
		for (int x = 0; x < gridWidth; x++)
		{
			for (int z = 0; z < gridDepth; z++)
			{
				this->setVoxel(x, 2, z, index);
			}
		}
	}
	else if (level.map2.size() > 0)
	{
		// Load MAP2 voxels.
		this->readMAP2(level.map2, gridWidth, gridDepth, inf);
	}

	// Assign locks.
	for (const auto &lock : level.lock)
	{
		const Int2 lockPosition = VoxelGrid::arenaVoxelToNewVoxel(
			Int2(lock.x, lock.y), gridWidth, gridDepth);
		this->locks.insert(std::make_pair(
			lockPosition, LevelData::Lock(lockPosition, lock.lockLevel)));
	}

	// Assign text and sound triggers.
	for (const auto &trigger : level.trig)
	{
		// Transform the voxel coordinates from the Arena layout to the new layout.
		// - For some reason, the grid dimensions have a minus one here, whereas
		//   the dimensions for player starting points do not.
		const Int2 voxel = VoxelGrid::arenaVoxelToNewVoxel(
			Int2(trigger.x, trigger.y), gridWidth, gridDepth);

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

LevelData::LevelData(VoxelGrid &&voxelGrid)
	: voxelGrid(std::move(voxelGrid))
{
	this->ceilingHeight = 1.0;
}

LevelData::~LevelData()
{

}

double LevelData::getCeilingHeight() const
{
	return this->ceilingHeight;
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

void LevelData::setVoxel(int x, int y, int z, int id)
{
	char *voxels = this->voxelGrid.getVoxels();
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

void LevelData::readFLOR(const std::vector<uint8_t> &flor, int width, int depth,
	const INFFile &inf)
{
	// Indices for voxel data, stepping two bytes at a time.
	int floorIndex = 0;

	auto getNextFloorVoxel = [&flor, &floorIndex]()
	{
		const uint16_t voxel = Bytes::getLE16(flor.data() + floorIndex);
		floorIndex += 2;
		return voxel;
	};

	// Mappings of floor and chasm IDs to voxel data indices. Chasms are treated
	// separately since their voxel data index is also a function of the four
	// adjacent voxels.
	std::unordered_map<uint16_t, int> floorDataMappings;
	std::unordered_map<std::pair<uint16_t, std::array<bool, 4>>, int> chasmDataMappings;

	// Write the .MIF file's voxel IDs into the voxel grid.
	for (int x = (width - 1); x >= 0; x--)
	{
		for (int z = (depth - 1); z >= 0; z--)
		{
			const int index = x + (z * width);
			const uint16_t florVoxel = getNextFloorVoxel();

			// The floor voxel has a texture if it's not a chasm.
			const int floorTextureID = (florVoxel & 0xFF00) >> 8;
			if ((floorTextureID != MIFFile::DRY_CHASM) &&
				(floorTextureID != MIFFile::WET_CHASM) &&
				(floorTextureID != MIFFile::LAVA_CHASM))
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
				// Assign voxel types to the empty voxels (i.e., a water voxel is a different
				// type than a lava voxel).

				// To do: there are ~10 combinations of chasm faces, which means there will be
				// up to ~10 voxel data for each type of chasm. The voxel data index is a function 
				// of the four surrounding voxels in addition to the voxel ID itself.

				// To do: get 4 adjacent voxels (for each: true if floor is solid).
				// - The resulting voxel data arguments depend on this (each wall face).
				const std::array<bool, 4> adjacentFaces
				{
					false, // North.
					false, // East.
					false, // South.
					false // West.
				};

				if (floorTextureID == MIFFile::DRY_CHASM)
				{
					const int dataIndex = [this, &inf, &chasmDataMappings, florVoxel,
						floorTextureID, &adjacentFaces]()
					{
						const auto chasmPair = std::make_pair(florVoxel, adjacentFaces);
						const auto chasmIter = chasmDataMappings.find(chasmPair);
						if (chasmIter != chasmDataMappings.end())
						{
							return chasmIter->second;
						}
						else
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

							const int index = this->voxelGrid.addVoxelData(VoxelData::makeChasm(
								dryChasmID,
								adjacentFaces.at(0),
								adjacentFaces.at(1),
								adjacentFaces.at(2),
								adjacentFaces.at(3),
								VoxelData::ChasmData::Type::Dry));
							return chasmDataMappings.insert(
								std::make_pair(chasmPair, index)).first->second;
						}
					}();

					this->setVoxel(x, 0, z, dataIndex);
				}
				else if (floorTextureID == MIFFile::WET_CHASM)
				{
					const int dataIndex = [this, &inf, &chasmDataMappings, florVoxel,
						floorTextureID, &adjacentFaces]()
					{
						const auto chasmPair = std::make_pair(florVoxel, adjacentFaces);
						const auto chasmIter = chasmDataMappings.find(chasmPair);
						if (chasmIter != chasmDataMappings.end())
						{
							return chasmIter->second;
						}
						else
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

							const int index = this->voxelGrid.addVoxelData(VoxelData::makeChasm(
								wetChasmID,
								adjacentFaces.at(0),
								adjacentFaces.at(1),
								adjacentFaces.at(2),
								adjacentFaces.at(3),
								VoxelData::ChasmData::Type::Wet));
							return chasmDataMappings.insert(
								std::make_pair(chasmPair, index)).first->second;
						}
					}();

					this->setVoxel(x, 0, z, dataIndex);
				}
				else if (floorTextureID == MIFFile::LAVA_CHASM)
				{
					const int dataIndex = [this, &inf, &chasmDataMappings, florVoxel,
						floorTextureID, &adjacentFaces]()
					{
						const auto chasmPair = std::make_pair(florVoxel, adjacentFaces);
						const auto chasmIter = chasmDataMappings.find(chasmPair);
						if (chasmIter != chasmDataMappings.end())
						{
							return chasmIter->second;
						}
						else
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

							const int index = this->voxelGrid.addVoxelData(VoxelData::makeChasm(
								lavaChasmID,
								adjacentFaces.at(0),
								adjacentFaces.at(1),
								adjacentFaces.at(2),
								adjacentFaces.at(3),
								VoxelData::ChasmData::Type::Lava));
							return chasmDataMappings.insert(
								std::make_pair(chasmPair, index)).first->second;
						}
					}();

					this->setVoxel(x, 0, z, dataIndex);
				}
			}
		}
	}
}

void LevelData::readMAP1(const std::vector<uint8_t> &map1, int width, int depth,
	const INFFile &inf)
{
	// Indices for voxel data, stepping two bytes at a time.
	int map1Index = 0;

	auto getNextMap1Voxel = [&map1, &map1Index]()
	{
		const uint16_t voxel = Bytes::getLE16(map1.data() + map1Index);
		map1Index += 2;
		return voxel;
	};

	// Mappings of wall IDs to voxel data indices.
	std::unordered_map<uint16_t, int> wallDataMappings;

	// Write the .MIF file's voxel IDs into the voxel grid.
	for (int x = (width - 1); x >= 0; x--)
	{
		for (int z = (depth - 1); z >= 0; z--)
		{
			const int index = x + (z * width);
			const uint16_t map1Voxel = getNextMap1Voxel();

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
						const int dataIndex = [this, &inf, &wallDataMappings,
							map1Voxel, mostSigByte]()
						{
							const auto wallIter = wallDataMappings.find(map1Voxel);
							if (wallIter != wallDataMappings.end())
							{
								return wallIter->second;
							}
							else
							{
								const int textureIndex = mostSigByte;
								const int index = this->voxelGrid.addVoxelData(VoxelData::makeWall(
									textureIndex, textureIndex, textureIndex, VoxelType::Solid));
								return wallDataMappings.insert(
									std::make_pair(map1Voxel, index)).first->second;
							}
						}();

						this->setVoxel(x, 1, z, dataIndex);
					}
					else
					{
						// Raised platform.
						const int dataIndex = [this, &inf, &wallDataMappings,
							map1Voxel, mostSigByte]()
						{
							const auto wallIter = wallDataMappings.find(map1Voxel);
							if (wallIter != wallDataMappings.end())
							{
								return wallIter->second;
							}
							else
							{
								const uint8_t wallTextureID = map1Voxel & 0x000F;
								const uint8_t capTextureID = (map1Voxel & 0x00F0) >> 4;

								const int sideID = [&inf, wallTextureID]()
								{
									const int *ptr = inf.getBoxSide(wallTextureID);
									if (ptr != nullptr)
									{
										return *ptr + 1;
									}
									else
									{
										DebugWarning("Missing *BOXSIDE ID \"" +
											std::to_string(wallTextureID) + "\".");
										return 0;
									}
								}();

								const int floorID = inf.getCeiling().textureIndex + 1;

								const int ceilingID = [&inf, capTextureID]()
								{
									const int *ptr = inf.getBoxCap(capTextureID);
									if (ptr != nullptr)
									{
										return *ptr + 1;
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

								const int index = this->voxelGrid.addVoxelData(VoxelData::makeRaised(
									sideID, floorID, ceilingID, yOffset, ySize, vTop, vBottom));
								return wallDataMappings.insert(
									std::make_pair(map1Voxel, index)).first->second;
							}
						}();

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
					const int dataIndex = [this, &inf, &wallDataMappings, map1Voxel]()
					{
						const auto wallIter = wallDataMappings.find(map1Voxel);
						if (wallIter != wallDataMappings.end())
						{
							return wallIter->second;
						}
						else
						{
							const int textureIndex = map1Voxel & 0x00FF;
							const int index = this->voxelGrid.addVoxelData(
								VoxelData::makeTransparentWall(textureIndex));
							return wallDataMappings.insert(
								std::make_pair(map1Voxel, index)).first->second;
						}
					}();

					this->setVoxel(x, 1, z, dataIndex);
				}
				else if (mostSigNibble == 0xA)
				{
					// Transparent block with 2-sided texture on one side (i.e., fence).
					const int dataIndex = [this, &inf, &wallDataMappings, map1Voxel]()
					{
						const auto wallIter = wallDataMappings.find(map1Voxel);
						if (wallIter != wallDataMappings.end())
						{
							return wallIter->second;
						}
						else
						{
							const int textureIndex = map1Voxel & 0x000F;

							const VoxelData::Facing facing = [map1Voxel]()
							{
								// Orientation is a multiple of 4 (0, 4, 8, C), where 0 is north 
								// and C is east.
								const int orientation = (map1Voxel & 0x00F0) >> 4;
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

							const int index = this->voxelGrid.addVoxelData(
								VoxelData::makeEdge(textureIndex, facing));
							return wallDataMappings.insert(
								std::make_pair(map1Voxel, index)).first->second;
						}
					}();

					this->setVoxel(x, 1, z, dataIndex);
				}
				else if (mostSigNibble == 0xB)
				{
					// Door voxel.
					const int dataIndex = [this, &inf, &wallDataMappings, map1Voxel]()
					{
						const auto wallIter = wallDataMappings.find(map1Voxel);
						if (wallIter != wallDataMappings.end())
						{
							return wallIter->second;
						}
						else
						{
							const int textureIndex = map1Voxel & 0x003F;

							// To do: see if *DOOR values in the .INF should actually be used 
							// to determine which door type it is.
							const int index = this->voxelGrid.addVoxelData(VoxelData::makeDoor(
								textureIndex, VoxelData::DoorData::Type::Swinging));
							return wallDataMappings.insert(
								std::make_pair(map1Voxel, index)).first->second;
						}
					}();

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
					const int dataIndex = [this, &inf, &wallDataMappings, map1Voxel]()
					{
						const auto wallIter = wallDataMappings.find(map1Voxel);
						if (wallIter != wallDataMappings.end())
						{
							return wallIter->second;
						}
						else
						{
							const int textureIndex = map1Voxel & 0x00FF;
							const bool isRightDiag = (map1Voxel & 0x0100) == 0;
							const int index = this->voxelGrid.addVoxelData(
								VoxelData::makeDiagonal(textureIndex, isRightDiag));
							return wallDataMappings.insert(
								std::make_pair(map1Voxel, index)).first->second;
						}
					}();

					this->setVoxel(x, 1, z, dataIndex);
				}
			}
		}
	}
}

void LevelData::readMAP2(const std::vector<uint8_t> &map2, int width, int depth,
	const INFFile &inf)
{
	const uint8_t *map2Data = map2.data();
	int map2Index = 0;

	auto getNextMap2Voxel = [map2Data, &map2Index]()
	{
		const uint16_t voxel = Bytes::getLE16(map2Data + map2Index);
		map2Index += 2;
		return voxel;
	};

	// Mappings of second floor IDs to voxel data indices.
	std::unordered_map<uint16_t, int> map2DataMappings;

	// Write the .MIF file's voxel IDs into the voxel grid.
	for (int x = (width - 1); x >= 0; x--)
	{
		for (int z = (depth - 1); z >= 0; z--)
		{
			const int index = x + (z * width);
			const uint16_t map2Voxel = getNextMap2Voxel();

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
						const int textureIndex = map2Voxel & 0x007F;
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
