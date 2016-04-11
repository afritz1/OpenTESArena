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

	const HolidayName &getHolidayName() const;
	std::string toString() const;
	int getDayNumber() const;
	MonthName getMonthName() const;
};

#endif
