#ifndef WEEKDAY_H
#define WEEKDAY_H

#include <cstdint>
#include <string>

enum class WeekdayName;

class Weekday
{
private:
	WeekdayName weekdayName;
public:
	Weekday(WeekdayName weekdayName);
	~Weekday();

	// This isn't really a necessary value, because weeks themselves are unnamed,
	// but it's in the manual, so it's here for completeness.
	static const int32_t DAYS_PER_WEEK;

	WeekdayName getWeekdayName() const;
	std::string toString() const;

	void incrementWeekday();
};

#endif
