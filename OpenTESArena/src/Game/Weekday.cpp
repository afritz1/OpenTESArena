#include <cassert>
#include <map>

#include "Weekday.h"

const auto WeekdayDisplayNames = std::map<WeekdayName, std::string>
{
	{ WeekdayName::Morndas, "Morndas" },
	{ WeekdayName::Tirdas, "Tirdas" },
	{ WeekdayName::Middas, "Middas" },
	{ WeekdayName::Turdas, "Turdas" },
	{ WeekdayName::Fredas, "Fredas" },
	{ WeekdayName::Loredas, "Loredas" },
	{ WeekdayName::Sundas, "Sundas" }
};

// Mappings of each weekday to the next weekday.
const auto WeekdayNexts = std::map<WeekdayName, WeekdayName>
{
	{ WeekdayName::Morndas, WeekdayName::Tirdas },
	{ WeekdayName::Tirdas, WeekdayName::Middas },
	{ WeekdayName::Middas, WeekdayName::Turdas },
	{ WeekdayName::Turdas, WeekdayName::Fredas },
	{ WeekdayName::Fredas, WeekdayName::Loredas },
	{ WeekdayName::Loredas, WeekdayName::Sundas },
	{ WeekdayName::Sundas, WeekdayName::Morndas }
};

const int Weekday::DAYS_PER_WEEK = 7;

Weekday::Weekday(WeekdayName weekdayName)
{
	this->weekdayName = weekdayName;
}

Weekday::~Weekday()
{

}

const WeekdayName &Weekday::getWeekdayName() const
{
	return this->weekdayName;
}

std::string Weekday::toString() const
{
	auto displayName = WeekdayDisplayNames.at(this->getWeekdayName());
	assert(displayName.size() > 0);
	return displayName;
}

void Weekday::incrementWeekday()
{
	auto nextWeekday = WeekdayNexts.at(this->getWeekdayName());
	this->weekdayName = nextWeekday;
}
