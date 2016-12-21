#include <cassert>

#include "Holiday.h"

Holiday::Holiday(const std::string &holidayName, const std::string &monthName, int day)
	: holidayName(holidayName), monthName(monthName)
{
	assert(day >= 1);
	assert(day <= 30);

	this->day = day;
}

Holiday::~Holiday()
{

}

const std::string &Holiday::getHolidayName() const
{
	return this->holidayName;
}

const std::string &Holiday::getMonthName() const
{
	return this->monthName;
}

int Holiday::getDay() const
{
	return this->day;
}
