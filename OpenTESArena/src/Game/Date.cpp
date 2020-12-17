#include "Date.h"

#include "components/debug/Debug.h"

Date::Date(int year, int month, int day)
{
	// Make sure each value is in a valid range.
	DebugAssert(year >= 1);
	DebugAssert(month >= 0);
	DebugAssert(month < Date::MONTHS_PER_YEAR);
	DebugAssert(day >= 0);
	DebugAssert(day < Date::DAYS_PER_MONTH);

	this->year = year;
	this->month = month;
	this->day = day;
}

Date::Date(int month, int day)
	: Date(Date::INITIAL_YEAR, month, day) { }

Date::Date()
	: Date(Date::INITIAL_YEAR, 0, 0) { }

int Date::getYear() const
{
	return this->year;
}

int Date::getMonth() const
{
	return this->month;
}

int Date::getWeekday() const
{
	// For now, all months start on the same weekday (Monday).
	return this->day % Date::DAYS_PER_WEEK;
}

int Date::getDay() const
{
	return this->day;
}

std::string Date::getOrdinalDay() const
{
	// The current day is zero-based, so add one to get the "actual" day.
	const int displayedDay = this->day + 1;
	const int ordinalDay = displayedDay % 10;

	// Days in the teens have some special cases.
	auto dayString = std::to_string(displayedDay);
	if ((ordinalDay == 1) && (displayedDay != 11))
	{
		dayString += "st";
	}
	else if ((ordinalDay == 2) && (displayedDay != 12))
	{
		dayString += "nd";
	}
	else if ((ordinalDay == 3) && (displayedDay != 13))
	{
		dayString += "rd";
	}
	else
	{
		dayString += "th";
	}

	return dayString;
}

int Date::getSeason() const
{
	return ((this->month + 10) % Date::MONTHS_PER_YEAR) / 3;
}

void Date::incrementYear()
{
	this->year++;
}

void Date::incrementMonth()
{
	this->month++;

	if (this->month == Date::MONTHS_PER_YEAR)
	{
		this->incrementYear();
		this->month = 0;
	}
}

void Date::incrementDay()
{
	this->day++;

	if (this->day == Date::DAYS_PER_MONTH)
	{
		this->incrementMonth();
		this->day = 0;
	}
}
