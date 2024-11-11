#include "ArenaClockUtils.h"

bool ArenaClockUtils::nightMusicIsActive(const Clock &clock)
{
	const double clockTime = clock.getTotalSeconds();
	const bool beforeDayMusicChange = clockTime < ArenaClockUtils::MusicSwitchToDay.getTotalSeconds();
	const bool afterNightMusicChange = clockTime >= ArenaClockUtils::MusicSwitchToNight.getTotalSeconds();
	return beforeDayMusicChange || afterNightMusicChange;
}

bool ArenaClockUtils::nightLightsAreActive(const Clock &clock)
{
	const double clockTime = clock.getTotalSeconds();
	const bool beforeLamppostDeactivate = clockTime < ArenaClockUtils::LamppostDeactivate.getTotalSeconds();
	const bool afterLamppostActivate = clockTime >= ArenaClockUtils::LamppostActivate.getTotalSeconds();
	return beforeLamppostDeactivate || afterLamppostActivate;
}

bool ArenaClockUtils::isDaytimeFogActive(const Clock &clock)
{
	const double clockTime = clock.getTotalSeconds();
	const bool afterAmbientEndBrightening = clockTime >= ArenaClockUtils::AmbientEndBrightening.getTotalSeconds();
	const bool beforeAmbientStartDimming = clockTime < ArenaClockUtils::AmbientStartDimming.getTotalSeconds();
	return afterAmbientEndBrightening && beforeAmbientStartDimming;
}
