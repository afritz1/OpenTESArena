#ifndef ARENA_CLOCK_UTILS_H
#define ARENA_CLOCK_UTILS_H

#include "Clock.h"

// @todo: maybe eventually read these from a text file

namespace ArenaClockUtils
{
	// Beginnings of each time-of-day range.
	const Clock Midnight(0, 0, 0);
	const Clock Night1(0, 1, 0);
	const Clock EarlyMorning(3, 0, 0);
	const Clock Morning(6, 0, 0);
	const Clock Noon(12, 0, 0);
	const Clock Afternoon(12, 1, 0);
	const Clock Evening(18, 0, 0);
	const Clock Night2(21, 0, 0);

	// Ambient lighting change times.
	const Clock AmbientStartBrightening(6, 0, 0);
	const Clock AmbientEndBrightening(6, 15, 0);
	const Clock AmbientStartDimming(17, 45, 0);
	const Clock AmbientEndDimming(18, 0, 0);

	// Lamppost activation times.
	const Clock LamppostActivate(17, 45, 0);
	const Clock LamppostDeactivate(6, 15, 0);

	// Change in music times.
	const Clock MusicSwitchToDay(6, 19, 0);
	const Clock MusicSwitchToNight(17, 45, 0);

	// Thunderstorm times.
	const Clock ThunderstormStart(18, 1, 0);
	const Clock ThunderstormEnd(6, 0, 0);

	// Returns whether the current music should be for day or night.
	bool nightMusicIsActive(const Clock &clock);

	// Returns whether night lights (i.e., lampposts) should currently be active.
	bool nightLightsAreActive(const Clock &clock);

	// The original game doesn't supply nighttime colors in FOG.LGT, so it disables it.
	bool isDaytimeFogActive(const Clock &clock);
}

#endif
