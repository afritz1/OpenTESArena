#ifndef ARENA_DATE_UTILS_H
#define ARENA_DATE_UTILS_H

#include <string>

class Date;

struct ExeData;

namespace ArenaDateUtils
{
	static constexpr int INITIAL_YEAR = 389;
	static constexpr int MONTHS_PER_YEAR = 12;
	static constexpr int DAYS_PER_MONTH = 30;
	static constexpr int DAYS_PER_WEEK = 7;

	// Makes the date string for a given date, using strings from the executable data.
	std::string makeDateString(const Date &date, const ExeData &exeData);
}

#endif
