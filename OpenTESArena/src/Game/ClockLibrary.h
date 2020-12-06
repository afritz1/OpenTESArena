#ifndef CLOCK_LIBRARY_H
#define CLOCK_LIBRARY_H

#include "Clock.h"

class ClockLibrary
{
public:
	// Beginnings of each time-of-day range.
	static const Clock Midnight;
	static const Clock Night1;
	static const Clock EarlyMorning;
	static const Clock Morning;
	static const Clock Noon;
	static const Clock Afternoon;
	static const Clock Evening;
	static const Clock Night2;

	// Ambient lighting change times.
	static const Clock AmbientStartBrightening;
	static const Clock AmbientEndBrightening;
	static const Clock AmbientStartDimming;
	static const Clock AmbientEndDimming;

	// Lamppost activation times.
	static const Clock LamppostActivate;
	static const Clock LamppostDeactivate;

	// Change in music times.
	static const Clock MusicSwitchToDay;
	static const Clock MusicSwitchToNight;
};

#endif
