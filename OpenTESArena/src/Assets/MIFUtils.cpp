#include <array>
#include <cstdio>

#include "MIFUtils.h"

#include "components/debug/Debug.h"
#include "components/dos/DOSUtils.h"
#include "components/utilities/Bytes.h"

namespace MIFUtils
{
	// City block generation data, used by city generation. The order of data is tightly coupled
	// with the original generation algorithm.
	const std::array<std::string, 7> CityBlockCodes = { "EQ", "MG", "NB", "TP", "TV", "TS", "BS" };
	const std::array<int, 7> CityBlockVariations = { 13, 11, 10, 12, 15, 11, 20 };
	const std::array<std::string, 4> CityBlockRotations = { "A", "B", "C", "D" };
}

bool MIFUtils::isChasm(int textureID)
{
	return (textureID == MIFUtils::DRY_CHASM) || (textureID == MIFUtils::WET_CHASM) ||
		(textureID == MIFUtils::LAVA_CHASM);
}

std::string MIFUtils::makeMainQuestDungeonMifName(int dungeonX, int dungeonY, int provinceID)
{
	uint32_t mifID = (dungeonY << 16) + dungeonX + provinceID;
	mifID = static_cast<uint32_t>(-static_cast<int32_t>(Bytes::rol(mifID, 5)));

	std::array<char, DOSUtils::FILENAME_BUFFER_SIZE> buffer;
	std::snprintf(buffer.data(), buffer.size(), "%d.MIF", mifID);
	return std::string(buffer.data());
}

int MIFUtils::getCityBlockCodeCount()
{
	return static_cast<int>(CityBlockCodes.size());
}

int MIFUtils::getCityBlockVariationsCount()
{
	return static_cast<int>(CityBlockVariations.size());
}

int MIFUtils::getCityBlockRotationCount()
{
	return static_cast<int>(CityBlockRotations.size());
}

const std::string &MIFUtils::getCityBlockCode(int index)
{
	DebugAssertIndex(CityBlockCodes, index);
	return CityBlockCodes[index];
}

int MIFUtils::getCityBlockVariations(int index)
{
	DebugAssertIndex(CityBlockVariations, index);
	return CityBlockVariations[index];
}

const std::string &MIFUtils::getCityBlockRotation(int index)
{
	DebugAssertIndex(CityBlockRotations, index);
	return CityBlockRotations[index];
}

std::string MIFUtils::makeCityBlockMifName(const char *code, int variation, const char *rotation)
{
	std::array<char, DOSUtils::FILENAME_BUFFER_SIZE> buffer;
	std::snprintf(buffer.data(), buffer.size(), "%sBD%d%s.MIF", code, variation, rotation);
	return std::string(buffer.data());
}
