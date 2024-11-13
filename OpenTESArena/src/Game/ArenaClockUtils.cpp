#include "ArenaClockUtils.h"
#include "ClockLibrary.h"

bool ArenaClockUtils::nightMusicIsActive(const Clock &clock)
{
	const ClockLibrary &clockLibrary = ClockLibrary::getInstance();
	const Clock &dayMusicStartClock = clockLibrary.getClock(ArenaClockUtils::MusicSwitchToDay);
	const Clock &nightMusicStartClock = clockLibrary.getClock(ArenaClockUtils::MusicSwitchToNight);

	const double clockTime = clock.getTotalSeconds();
	const bool isBeforeDayMusicChange = clockTime < dayMusicStartClock.getTotalSeconds();
	const bool isAfterNightMusicChange = clockTime >= nightMusicStartClock.getTotalSeconds();
	return isBeforeDayMusicChange || isAfterNightMusicChange;
}

bool ArenaClockUtils::nightLightsAreActive(const Clock &clock)
{
	const ClockLibrary &clockLibrary = ClockLibrary::getInstance();
	const Clock &lamppostDeactivateClock = clockLibrary.getClock(ArenaClockUtils::LamppostDeactivate);
	const Clock &lamppostActivateClock = clockLibrary.getClock(ArenaClockUtils::LamppostActivate);

	const double clockTime = clock.getTotalSeconds();
	const bool isBeforeLamppostDeactivate = clockTime < lamppostDeactivateClock.getTotalSeconds();
	const bool isAfterLamppostActivate = clockTime >= lamppostActivateClock.getTotalSeconds();
	return isBeforeLamppostDeactivate || isAfterLamppostActivate;
}

bool ArenaClockUtils::isDaytimeFogActive(const Clock &clock)
{
	const ClockLibrary &clockLibrary = ClockLibrary::getInstance();
	const Clock &ambientBrighteningEndClock = clockLibrary.getClock(ArenaClockUtils::AmbientBrighteningEnd);
	const Clock &ambientDimmingStartClock = clockLibrary.getClock(ArenaClockUtils::AmbientDimmingStart);

	const double clockTime = clock.getTotalSeconds();
	const bool isAfterAmbientEndBrightening = clockTime >= ambientBrighteningEndClock.getTotalSeconds();
	const bool isBeforeAmbientStartDimming = clockTime < ambientDimmingStartClock.getTotalSeconds();
	return isAfterAmbientEndBrightening && isBeforeAmbientStartDimming;
}
