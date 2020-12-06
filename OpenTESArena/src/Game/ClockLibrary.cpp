#include "ClockLibrary.h"

const Clock ClockLibrary::Midnight(0, 0, 0);
const Clock ClockLibrary::Night1(0, 1, 0);
const Clock ClockLibrary::EarlyMorning(3, 0, 0);
const Clock ClockLibrary::Morning(6, 0, 0);
const Clock ClockLibrary::Noon(12, 0, 0);
const Clock ClockLibrary::Afternoon(12, 1, 0);
const Clock ClockLibrary::Evening(18, 0, 0);
const Clock ClockLibrary::Night2(21, 0, 0);

const Clock ClockLibrary::AmbientStartBrightening(6, 0, 0);
const Clock ClockLibrary::AmbientEndBrightening(6, 15, 0);
const Clock ClockLibrary::AmbientStartDimming(17, 45, 0);
const Clock ClockLibrary::AmbientEndDimming(18, 0, 0);

const Clock ClockLibrary::LamppostActivate(17, 45, 0);
const Clock ClockLibrary::LamppostDeactivate(6, 15, 0);

const Clock ClockLibrary::MusicSwitchToDay(6, 19, 0);
const Clock ClockLibrary::MusicSwitchToNight(17, 45, 0);
