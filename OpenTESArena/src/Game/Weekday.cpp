#include "Weekday.h"

const int Weekday::DAYS_PER_WEEK = 7;

Weekday::Weekday(const std::string &weekdayName)
	: weekdayName(weekdayName)
{
	
}

Weekday::~Weekday()
{

}

const std::string &Weekday::getWeekdayName() const
{
	return this->weekdayName;
}
