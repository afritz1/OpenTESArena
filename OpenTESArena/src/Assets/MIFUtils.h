#ifndef MIF_UTILS_H
#define MIF_UTILS_H

#include <cstdint>
#include <string>
#include <string_view>

// Various functions for working with .MIF files.

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

	// Returns whether the texture ID points to a chasm texture.
	bool isChasm(int textureID);

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
}

#endif
