#include <cassert>
#include <map>

#include "Month.h"

#include "MonthName.h"

const std::map<MonthName, std::string> MonthDisplayNames =
{
	// Spring
	{ MonthName::SunsDawn, "Sun's Dawn" },
	{ MonthName::FirstSeed, "First Seed" },
	{ MonthName::RainsHand, "Rain's Hand" },

	// Summer
	{ MonthName::SecondSeed, "Second Seed" },
	{ MonthName::MidYear, "Mid Year" },
	{ MonthName::SunsHeight, "Sun's Height" },

	// Fall
	{ MonthName::LastSeed, "Last Seed" },
	{ MonthName::Hearthfire, "Hearthfire" },
	{ MonthName::Frostfall, "Frostfall" },

	// Winter
	{ MonthName::SunsDusk, "Sun's Dusk" },
	{ MonthName::EveningStar, "Evening Star" },
	{ MonthName::MorningStar, "Morning Star" }
};

// Each month's next month. I didn't feel like doing some kind of circular buffer
// when this works just fine, even if it's a little bit more verbose.
const std::map<MonthName, MonthName> MonthNexts =
{
	// Spring
	{ MonthName::SunsDawn, MonthName::FirstSeed },
	{ MonthName::FirstSeed, MonthName::RainsHand },
	{ MonthName::RainsHand, MonthName::SecondSeed },

	// Summer
	{ MonthName::SecondSeed, MonthName::MidYear },
	{ MonthName::MidYear, MonthName::SunsHeight },
	{ MonthName::SunsHeight, MonthName::LastSeed },

	// Fall
	{ MonthName::LastSeed, MonthName::Hearthfire },
	{ MonthName::Hearthfire, MonthName::Frostfall },
	{ MonthName::Frostfall, MonthName::SunsDusk },

	// Winter
	{ MonthName::SunsDusk, MonthName::EveningStar },
	{ MonthName::EveningStar, MonthName::MorningStar },
	{ MonthName::MorningStar, MonthName::SunsDawn }
};

const int Month::DAYS_PER_MONTH = 30;

Month::Month(MonthName monthName)
{
	this->monthName = monthName;
}

Month::~Month()
{

}

MonthName Month::getMonthName() const
{
	return this->monthName;
}

bool Month::isLastMonthInYear() const
{
	return this->monthName == MonthName::EveningStar;
}

std::string Month::toString() const
{
	auto displayName = MonthDisplayNames.at(this->getMonthName());
	return displayName;
}

void Month::incrementMonth()
{
	auto nextMonth = MonthNexts.at(this->getMonthName());
	this->monthName = nextMonth;
}
