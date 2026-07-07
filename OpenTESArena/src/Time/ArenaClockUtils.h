#pragma once

#include "Clock.h"

namespace ArenaClockUtils
{
	// One real second is thirteen game seconds. One game round is sixty game seconds.
	static constexpr double GameSecondsPerRealTimeSecond = static_cast<double>(Clock::SECONDS_IN_A_DAY) / 6646.153846153846;
	static constexpr double GameSecondsPerRound = 60.0;
	static constexpr double RoundsPerRealTimeSecond = GameSecondsPerRealTimeSecond / GameSecondsPerRound;
	static constexpr double RealTimeSecondsPerRound = 1.0 / RoundsPerRealTimeSecond;
	static constexpr double CampingTimeScale = 600.0;

	// Beginnings of each time-of-day range.
	constexpr char Midnight[] = "Midnight";
	constexpr char Night1[] = "Night1";
	constexpr char EarlyMorning[] = "EarlyMorning";
	constexpr char Morning[] = "Morning";
	constexpr char Noon[] = "Noon";
	constexpr char Afternoon[] = "Afternoon";
	constexpr char Evening[] = "Evening";
	constexpr char Night2[] = "Night2";

	// Ambient lighting change times.
	constexpr char AmbientBrighteningStart[] = "AmbientBrighteningStart";
	constexpr char AmbientBrighteningEnd[] = "AmbientBrighteningEnd";
	constexpr char AmbientDimmingStart[] = "AmbientDimmingStart";
	constexpr char AmbientDimmingEnd[] = "AmbientDimmingEnd";

	// Lamppost activation times.
	constexpr char LamppostActivate[] = "LamppostActivate";
	constexpr char LamppostDeactivate[] = "LamppostDeactivate";

	// Change in music times.
	constexpr char MusicSwitchToDay[] = "MusicSwitchToDay";
	constexpr char MusicSwitchToNight[] = "MusicSwitchToNight";

	// Thunderstorm times.
	constexpr char ThunderstormStart[] = "ThunderstormStart";
	constexpr char ThunderstormEnd[] = "ThunderstormEnd";

	// Returns whether the current music should be for day or night.
	bool nightMusicIsActive(const Clock &clock);

	// Returns whether night lights (i.e., lampposts) should currently be active.
	bool nightLightsAreActive(const Clock &clock);

	// The original game doesn't supply nighttime colors in FOG.LGT, so it disables it.
	bool isDaytimeFogActive(const Clock &clock);
}
