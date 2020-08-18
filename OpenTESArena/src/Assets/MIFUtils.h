#ifndef MIF_UTILS_H
#define MIF_UTILS_H

#include <cstdint>
#include <string>

#include "../Math/Vector2.h"
#include "../World/VoxelUtils.h"

// Various functions for working with .MIF files.

class ArenaRandom;

namespace MIFUtils
{
	// Texture IDs for various chasms in voxel data.
	constexpr uint8_t DRY_CHASM = 0xC;
	constexpr uint8_t WET_CHASM = 0xD;
	constexpr uint8_t LAVA_CHASM = 0xE;

	// This value is used for transforming .MIF coordinates to voxel coordinates. For example, 
	// if the values in the .MIF files are centimeters, then dividing by this value converts 
	// them to voxel coordinates (including decimal values; i.e., X=1.5 means the middle of the 
	// voxel at X coordinate 1).
	constexpr double ARENA_UNITS = 128.0;

	// City generation block types.
	enum class BlockType
	{
		Empty,
		Reserved,
		Equipment,
		MagesGuild,
		NobleHouse,
		Temple,
		Tavern,
		Spacer,
		Houses
	};

	// Returns whether the texture ID points to a chasm texture.
	bool isChasm(int textureID);

	// Converts a .MIF start point from "centimeter-like" units to real voxel units, where the
	// fractional value is the position inside the voxel.
	Double2 convertStartPointToReal(const OriginalInt2 &startPoint);

	// Generates the filename for a main quest .MIF file given the XY province coordinates 
	// and the province ID.
	std::string makeMainQuestDungeonMifName(int dungeonX, int dungeonY, int provinceID);

	// City block generation data functions.
	int getCityBlockCodeCount();
	int getCityBlockVariationsCount();
	int getCityBlockRotationCount();
	const std::string &getCityBlockCode(int index);
	int getCityBlockVariations(int index);
	const std::string &getCityBlockRotation(int index);

	// Makes a city block .MIF filename for city generation.
	std::string makeCityBlockMifName(const char *code, int variation, const char *rotation);
	std::string makeCityBlockMifName(BlockType blockType, ArenaRandom &random);

	// Generates a random .MIF block type for use with city generation.
	BlockType generateRandomBlockType(ArenaRandom &random);
}

#endif
