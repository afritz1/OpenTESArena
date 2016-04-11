#include <cassert>

#include "Date.h"

#include "Month.h"
#include "Weekday.h"
#include "Year.h"

Date::Date(int day, WeekdayName weekdayName, MonthName monthName, const Year &year)
{
	this->day = day;

	this->weekday = std::unique_ptr<Weekday>(new Weekday(weekdayName));
	this->month = std::unique_ptr<Month>(new Month(monthName));
	this->year = std::unique_ptr<Year>(new Year(year));

	// Programmer error if the day is ever out of range.
	assert(this->day >= 1);
	assert(this->day <= 30);

	assert(this->weekday.get() != nullptr);
	assert(this->month.get() != nullptr);
	assert(this->year.get() != nullptr);
}

Date::Date(const Date &date)
	: Date(date.getDayNumber(), date.getWeekday().getWeekdayName(), 
		date.getMonth().getMonthName(), date.getYear()) { }

Date::~Date()
{

}

const int &Date::getDayNumber() const
{
	return this->day;
}

const Weekday &Date::getWeekday() const
{
	return *this->weekday.get();
}

const Month &Date::getMonth() const
{
	return *this->month.get();
}

const Year &Date::getYear() const
{
	return *this->year.get();
}

std::string Date::getOrdinalDay() const
{
	assert(this->day >= 1);
	assert(this->day <= 30);

	int dayNumber = this->getDayNumber();
	auto dayString = std::to_string(dayNumber);

	int ordinalDay = dayNumber % 10;

	if (ordinalDay == 1)
	{
		return dayString + "st";
	}
	else if (ordinalDay == 2)
	{
		return dayString + "nd";
	}
	else if (ordinalDay == 3)
	{
		return dayString + "rd";
	}
	else
	{
		return dayString + "th";
	}
}

void Date::incrementDay()
{
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
	this->weekday->incrementWeekday();
}

void Date::incrementMonth()
{
	bool isLastMonth = this->month->isLastMonthInYear();

	this->month->incrementMonth();

	if (isLastMonth)
	{
		this->incrementYear();
	}
}

void Date::incrementYear()
{
	this->year->incrementYear();
}
