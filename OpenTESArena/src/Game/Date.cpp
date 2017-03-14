#include <cassert>

#include "Date.h"

#include "Month.h"
#include "Weekday.h"
#include "Year.h"
#include "../Utilities/Debug.h"

Date::Date(const Year &year, const Month &month, const Weekday &weekday, int day)
	: year(year), month(month), weekday(weekday)
{
	// Programmer error if the day is ever out of range.
	assert(day >= 1);
	assert(day <= 30);

	this->day = day;
}

Date::~Date()
{

}

const Year &Date::getYear() const
{
	return this->year;
}

const Month &Date::getMonth() const
{
	return this->month;
}

const Weekday &Date::getWeekday() const
{
	return this->weekday;
}

int Date::getDayNumber() const
{
	return this->day;
}

std::string Date::getOrdinalDay() const
{
	// Programmer error if the day is ever out of range.
	assert(this->day >= 1);
	assert(this->day <= 30);

	int dayNumber = this->getDayNumber();
	int ordinalDay = dayNumber % 10;

	auto dayString = std::to_string(dayNumber);
	if (ordinalDay == 1)
	{
		dayString += "st";
	}
	else if (ordinalDay == 2)
	{
		dayString += "nd";
	}
	else if (ordinalDay == 3)
	{
		dayString += "rd";
	}
	else
	{
		dayString += "th";
	}

	return dayString;
}

void Date::incrementDay()
{
	// Programmer error if the day is ever out of range.
	assert(this->day >= 1);
	assert(this->day <= 30);

	this->day++;
	this->incrementWeekday();

	// No need to check for "> 31"; the assertions take care of that.
	if (this->day == 31)
	{
		this->day = 1;
		this->incrementMonth();
	}
}

void Date::incrementWeekday()
{
	Debug::crash("Date", "incrementWeekday() not implemented.");
	//this->weekday->incrementWeekday();
}

void Date::incrementMonth()
{
	Debug::crash("Date", "incrementMonth() not implemented.");
	/*bool isLastMonth = this->month->isLastMonthInYear();

	this->month->incrementMonth();

	if (isLastMonth)
	{
		this->incrementYear();
	}*/
}

void Date::incrementYear()
{
	this->year.incrementYear();
}
