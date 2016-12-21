#include "Month.h"

const int Month::DAYS_PER_MONTH = 30;

Month::Month(const std::string &monthName)
	: monthName(monthName)
{
	
}

Month::~Month()
{

}

const std::string &Month::getMonthName() const
{
	return this->monthName;
}
