#include <cassert>

#include "Date.h"
#include "../Utilities/Debug.h"

const int Date::INITIAL_ERA = 3;
const int Date::INITIAL_YEAR = 389;
const int Date::YEARS_PER_ERA = 1000;
const int Date::MONTHS_PER_YEAR = 12;
const int Date::DAYS_PER_MONTH = 30;
const int Date::DAYS_PER_WEEK = 7;

Date::Date(int era, int year, int month, int day)
{
	// Make sure each value is in a valid range.
	assert(era >= 0);
	assert(year >= 1);
	assert(year <= Date::YEARS_PER_ERA);
	assert(month >= 0);
	assert(month < Date::MONTHS_PER_YEAR);
	assert(day >= 0);
	assert(day < Date::DAYS_PER_MONTH);

	this->era = era;
	this->year = year;
	this->month = month;
	this->day = day;
}

Date::Date(int month, int day)
	: Date(Date::INITIAL_ERA, Date::INITIAL_YEAR, month, day) { }

Date::~Date()
{

}

int Date::getEra() const
{
	return this->era;
}

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

	auto dayString = std::to_string(displayedDay);
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

void Date::incrementEra()
{
	this->era++;
}

void Date::incrementYear()
{
	this->year++;

	if (this->year == (Date::YEARS_PER_ERA + 1))
	{
		this->incrementEra();
		this->year = 1;
	}
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
