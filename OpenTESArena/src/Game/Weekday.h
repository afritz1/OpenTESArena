#ifndef WEEKDAY_H
#define WEEKDAY_H

#include <string>

#include "WeekdayName.h"

class Weekday
{
private:
	WeekdayName weekdayName;
public:
	Weekday(WeekdayName weekdayName);
	~Weekday();

	// This isn't really a necessary value, because weeks themselves are unnamed,
	// but it's in the manual, so it's here for completeness.
	static const int DAYS_PER_WEEK;

	const WeekdayName &getWeekdayName() const;
	std::string toString() const;

	void incrementWeekday();
};

#endif