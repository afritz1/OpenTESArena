#include "ArenaClockUtils.h"

bool ArenaClockUtils::nightMusicIsActive(const Clock &clock)
{
	const double clockTime = clock.getPreciseTotalSeconds();
	const bool beforeDayMusicChange = clockTime < ArenaClockUtils::MusicSwitchToDay.getPreciseTotalSeconds();
	const bool afterNightMusicChange = clockTime >= ArenaClockUtils::MusicSwitchToNight.getPreciseTotalSeconds();
	return beforeDayMusicChange || afterNightMusicChange;
}

bool ArenaClockUtils::nightLightsAreActive(const Clock &clock)
{
	const double clockTime = clock.getPreciseTotalSeconds();
	const bool beforeLamppostDeactivate = clockTime < ArenaClockUtils::LamppostDeactivate.getPreciseTotalSeconds();
	const bool afterLamppostActivate = clockTime >= ArenaClockUtils::LamppostActivate.getPreciseTotalSeconds();
	return beforeLamppostDeactivate || afterLamppostActivate;
}

bool ArenaClockUtils::isDaytimeFogActive(const Clock &clock)
{
	const double clockTime = clock.getPreciseTotalSeconds();
	const bool afterAmbientEndBrightening = clockTime >= ArenaClockUtils::AmbientEndBrightening.getPreciseTotalSeconds();
	const bool beforeAmbientStartDimming = clockTime < ArenaClockUtils::AmbientStartDimming.getPreciseTotalSeconds();
	return afterAmbientEndBrightening && beforeAmbientStartDimming;
}
