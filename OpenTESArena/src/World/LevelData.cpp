#include "LevelData.h"
#include "../Assets/INFFile.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Bytes.h"

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
	int gridWidth, int gridDepth)
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

	// Empty voxel data (for air).
	const int emptyID = voxelGrid.addVoxelData(VoxelData(0));

	// Lambda for setting a voxel at some coordinate to some ID.
	auto setVoxel = [this](int x, int y, int z, int id)
	{
		char *voxels = this->voxelGrid.getVoxels();
		voxels[x + (y * this->voxelGrid.getWidth()) +
			(z * this->voxelGrid.getWidth() * this->voxelGrid.getHeight())] = id;
	};

	// Get FLOR and MAP1 data from the level (should be present in all .MIF files).
	const uint8_t *florData = level.flor.data();
	const uint8_t *map1Data = level.map1.data();

	// Indices for voxel data, stepping two bytes at a time.
	int floorIndex = 0;
	int map1Index = 0;

	auto getNextFloorVoxel = [florData, &floorIndex]()
	{
		const uint16_t voxel = Bytes::getLE16(florData + floorIndex);
		floorIndex += 2;
		return voxel;
	};

	auto getNextMap1Voxel = [map1Data, &map1Index]()
	{
		const uint16_t voxel = Bytes::getLE16(map1Data + map1Index);
		map1Index += 2;
		return voxel;
	};

	// Mappings of floor and wall IDs to voxel data indices.
	std::unordered_map<uint16_t, int> floorDataMappings, wallDataMappings;

	// Write the .MIF file's voxel IDs into the voxel grid.
	for (int x = (gridWidth - 1); x >= 0; x--)
	{
		for (int z = (gridDepth - 1); z >= 0; z--)
		{
			const int index = x + (z * gridWidth);
			const uint16_t florVoxel = getNextFloorVoxel();
			const uint16_t map1Voxel = getNextMap1Voxel();

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
						// To do: Also assign some "seawall" texture for interiors and exteriors.
						// Retrieve it beforehand from the *...CHASM members and assign here?
						// Not sure how that works.
						const int index = this->voxelGrid.addVoxelData(VoxelData(floorTextureID + 1));
						return floorDataMappings.insert(
							std::make_pair(florVoxel, index)).first->second;
					}
				}();

				setVoxel(x, 0, z, dataIndex);
			}

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
						// Regular 1x1x1 wall.
						const int wallTextureID = mostSigByte;

						// Get the voxel data index associated with the wall value, or add it
						// if it doesn't exist yet.
						const int dataIndex = [this, &inf, &wallDataMappings,
							map1Voxel, wallTextureID]()
						{
							const auto wallIter = wallDataMappings.find(map1Voxel);
							if (wallIter != wallDataMappings.end())
							{
								return wallIter->second;
							}
							else
							{
								const double ceilingHeight =
									static_cast<double>(inf.getCeiling().height) /
									MIFFile::ARENA_UNITS;

								const int index = this->voxelGrid.addVoxelData(VoxelData(
									wallTextureID,
									wallTextureID,
									wallTextureID,
									0.0, ceilingHeight, 0.0, 1.0));
								return wallDataMappings.insert(
									std::make_pair(map1Voxel, index)).first->second;
							}
						}();

						setVoxel(x, 1, z, dataIndex);
					}
					else
					{
						// Raised platform. The height appears to be some fraction of 64,
						// and when it's greater than 64, then that determines the offset?
						const uint8_t capTextureID = (map1Voxel & 0x00F0) >> 4;
						const uint8_t wallTextureID = map1Voxel & 0x000F;
						const double platformHeight = static_cast<double>(mostSigByte) /
							static_cast<double>(MIFFile::ARENA_UNITS);

						// Get the voxel data index associated with the wall value, or add it
						// if it doesn't exist yet.
						const int dataIndex = [this, &inf, &wallDataMappings, map1Voxel,
							capTextureID, wallTextureID, platformHeight]()
						{
							const auto wallIter = wallDataMappings.find(map1Voxel);
							if (wallIter != wallDataMappings.end())
							{
								return wallIter->second;
							}
							else
							{
								// To do: Clamp top V coordinate positive until the correct platform 
								// height calculation is figured out. Maybe the platform height
								// needs to be multiplied by the ratio between the current ceiling
								// height and the default ceiling height (128)? I.e., multiply by
								// the "ceilingHeight" local variable used a couple dozen lines up?
								const double topV = std::max(0.0, 1.0 - platformHeight);
								const double bottomV = 1.0; // To do: should also be a function.

								const int index = this->voxelGrid.addVoxelData(VoxelData(
									(*inf.getBoxSide(wallTextureID)) + 1,
									inf.getCeiling().textureIndex + 1,
									(*inf.getBoxCap(capTextureID)) + 1,
									0.0, platformHeight, topV, bottomV));
								return wallDataMappings.insert(
									std::make_pair(map1Voxel, index)).first->second;
							}
						}();

						setVoxel(x, 1, z, dataIndex);
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
					// Transparent block with 1-sided texture on all sides.
					// To do.
				}
				else if (mostSigNibble == 0xA)
				{
					// Transparent block with 2-sided texture on one side.
					// To do.
				}
				else if (mostSigNibble == 0xB)
				{
					// Door with texture.
					const int dataIndex = [this, &inf, &wallDataMappings, map1Voxel]()
					{
						const auto wallIter = wallDataMappings.find(map1Voxel);
						if (wallIter != wallDataMappings.end())
						{
							return wallIter->second;
						}
						else
						{
							const int doorTextureIndex = map1Voxel & 0x003F;
							const double ceilingHeight =
								static_cast<double>(inf.getCeiling().height) /
								MIFFile::ARENA_UNITS;

							const int index = this->voxelGrid.addVoxelData(VoxelData(
								doorTextureIndex, 0, 0, 0.0, ceilingHeight, 0.0, 1.0));
							return wallDataMappings.insert(
								std::make_pair(map1Voxel, index)).first->second;
						}
					}();

					setVoxel(x, 1, z, dataIndex);
				}
				else if (mostSigNibble == 0xC)
				{
					// Unknown (perhaps "curtains" for beds?).
					// To do.
				}
				else if (mostSigNibble == 0xD)
				{
					// Diagonal wall -- direction depends on the nineth bit.
					const int dataIndex = [this, &inf, &wallDataMappings, map1Voxel]()
					{
						const auto wallIter = wallDataMappings.find(map1Voxel);
						if (wallIter != wallDataMappings.end())
						{
							return wallIter->second;
						}
						else
						{
							const bool isRightDiag = (map1Voxel & 0x0100) == 0;
							const int diagTextureIndex = map1Voxel & 0x00FF;
							const double ceilingHeight =
								static_cast<double>(inf.getCeiling().height) /
								MIFFile::ARENA_UNITS;

							const int index = this->voxelGrid.addVoxelData(VoxelData(
								isRightDiag ? diagTextureIndex : 0,
								!isRightDiag ? diagTextureIndex : 0,
								0.0, ceilingHeight, 0.0, 1.0));
							return wallDataMappings.insert(
								std::make_pair(map1Voxel, index)).first->second;
						}
					}();

					setVoxel(x, 1, z, dataIndex);
				}
			}
		}
	}

	// Fill the second floor with the ceiling tiles if it's an interior, or MAP2 if it exists.
	const INFFile::CeilingData &ceiling = inf.getCeiling();

	// To do: Replace this. Figure out how to determine if some place is an interior.
	const bool isInterior = !ceiling.outdoorDungeon;

	if (isInterior)
	{
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

		// Define the ceiling voxel data as being right above the top of the main floor.
		const int index = this->voxelGrid.addVoxelData(VoxelData(
			0,
			ceilingIndex + 1,
			0,
			(static_cast<double>(ceiling.height) / MIFFile::ARENA_UNITS) - 1.0,
			1.0,
			0.0,
			1.0));

		// Set all the ceiling voxels.
		for (int x = 0; x < gridWidth; x++)
		{
			for (int z = 0; z < gridDepth; z++)
			{
				setVoxel(x, 2, z, index);
			}
		}
	}
	else if (level.map2.size() > 0)
	{
		// To do: Write second story voxels, and extend them higher if necessary.
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
			Int2(trigger.x, trigger.y), gridWidth - 1, gridDepth - 1);

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
	: voxelGrid(std::move(voxelGrid)) { }

LevelData::~LevelData()
{

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
