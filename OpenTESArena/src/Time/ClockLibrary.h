#pragma once

#include <string>

#include "components/utilities/Buffer.h"
#include "components/utilities/Singleton.h"

#include "Clock.h"

class ClockLibrary : public Singleton<ClockLibrary>
{
private:
	Buffer<Clock> clocks;
	Buffer<std::string> clockNames;
public:
	bool init(const char *filename);

	const Clock &getClock(const char *name) const;
};
