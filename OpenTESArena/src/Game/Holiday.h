#ifndef HOLIDAY_H
#define HOLIDAY_H

#include <string>

#include "HolidayName.h"

enum class MonthName;

class Holiday
{
private:
	HolidayName holidayName;
public:
	Holiday(HolidayName holidayName);
	~Holiday();

	HolidayName getHolidayName() const;
	std::string toString() const;

	// These two methods together tell when a holiday is.
	int getDayNumber() const;
	MonthName getMonthName() const;
};

#endif
