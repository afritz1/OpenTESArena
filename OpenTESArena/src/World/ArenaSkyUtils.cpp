#include <algorithm>
#include <array>

#include "ArenaSkyUtils.h"
#include "ClimateType.h"

#include "components/debug/Debug.h"

namespace ArenaSkyUtils
{
	const std::array<std::pair<ClimateType, LandTraits>, 3> LandTraitsMappings =
	{
		{
			{ ClimateType::Temperate, ArenaSkyUtils::LandTraits(2, 4, 10, 2) },
			{ ClimateType::Desert, ArenaSkyUtils::LandTraits(1, 6, 4, 1) },
			{ ClimateType::Mountain, ArenaSkyUtils::LandTraits(0, 6, 11, 2) }
		}
	};
}

ArenaSkyUtils::LandTraits::LandTraits(int filenameIndex, int position, int variation, int maxDigits)
{
	this->filenameIndex = filenameIndex;
	this->position = position;
	this->variation = variation;
	this->maxDigits = maxDigits;
}

const ArenaSkyUtils::LandTraits &ArenaSkyUtils::getLandTraits(ClimateType climateType)
{
	const auto iter = std::find_if(ArenaSkyUtils::LandTraitsMappings.begin(),
		ArenaSkyUtils::LandTraitsMappings.end(), [climateType](const auto &pair)
	{
		return pair.first == climateType;
	});

	DebugAssertMsg(iter != ArenaSkyUtils::LandTraitsMappings.end(), "Invalid climate type \"" +
		std::to_string(static_cast<int>(climateType)) + "\".");

	return iter->second;
}

Radians ArenaSkyUtils::arenaAngleToRadians(int arenaAngle)
{
	// Arena angles: 0 = south, 128 = west, 256 = north, 384 = east.
	// Change from clockwise to counter-clockwise and move 0 to east (the origin).
	const Radians arenaRadians = Constants::TwoPi *
		(static_cast<double>(arenaAngle) / static_cast<double>(ArenaSkyUtils::UNIQUE_ANGLES));
	const Radians flippedArenaRadians = Constants::TwoPi - arenaRadians;
	return flippedArenaRadians - Constants::HalfPi;
}
