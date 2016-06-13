#include <cassert>
#include <map>

#include "Holiday.h"

#include "MonthName.h"

const auto HolidayDisplayNames = std::map<HolidayName, std::string>
{
	{ HolidayName::NewLifeFestival, "New Life Festival" },
	{ HolidayName::SouthWindsPrayer, "South Winds Prayer" },
	{ HolidayName::HeartsDay, "Heart's Day" },
	{ HolidayName::FirstPlanting, "First Planting" },
	{ HolidayName::JestersDay, "Jester's Day" },
	{ HolidayName::SecondPlanting, "Second Planting" },
	{ HolidayName::MidYearCelebration, "Mid Year Celebration" },
	{ HolidayName::MerchantsFestival, "Merchant's Festival" }, // "Merchants" in the manual.
	{ HolidayName::SunsRest, "Sun's Rest" },
	{ HolidayName::HarvestsEnd, "Harvest's End" },
	{ HolidayName::TalesAndTallows, "Tales and Tallows" },
	{ HolidayName::WitchesFestival, "Witches' Festival" }, // "Witches" in the manual.
	{ HolidayName::EmperorsDay, "Emperor's Day" },
	{ HolidayName::WarriorsFestival, "Warrior's Festival" }, // "Warriors" in the manual.
	{ HolidayName::NorthWindsPrayer, "North Winds Prayer" }
};

// Every holiday has an associated day and month. No particular weekday association.
const auto HolidayDates = std::map<HolidayName, std::pair<MonthName, int>>
{
	{ HolidayName::NewLifeFestival, { MonthName::MorningStar, 1 } },
	{ HolidayName::SouthWindsPrayer, { MonthName::MorningStar, 15 } },
	{ HolidayName::HeartsDay, { MonthName::SunsDawn, 16 } },
	{ HolidayName::FirstPlanting, { MonthName::FirstSeed, 7 } },
	{ HolidayName::JestersDay, { MonthName::RainsHand, 28 } },
	{ HolidayName::SecondPlanting, { MonthName::SecondSeed, 7 } },
	{ HolidayName::MidYearCelebration, { MonthName::MidYear, 16 } },
	{ HolidayName::MerchantsFestival, { MonthName::SunsHeight, 10 } },
	{ HolidayName::SunsRest, { MonthName::SunsHeight, 20 } },
	{ HolidayName::HarvestsEnd, { MonthName::LastSeed, 27 } },
	{ HolidayName::TalesAndTallows, { MonthName::Hearthfire, 3 } },
	{ HolidayName::WitchesFestival, { MonthName::Frostfall, 13 } },
	{ HolidayName::EmperorsDay, { MonthName::Frostfall, 30 } },
	{ HolidayName::WarriorsFestival, { MonthName::SunsDusk, 20 } },
	{ HolidayName::NorthWindsPrayer, { MonthName::EveningStar, 1 } }
};

Holiday::Holiday(HolidayName holidayName)
{
	this->holidayName = holidayName;
}

Holiday::~Holiday()
{

}

HolidayName Holiday::getHolidayName() const
{
	return this->holidayName;
}

std::string Holiday::toString() const
{
	auto displayName = HolidayDisplayNames.at(this->getHolidayName());
	assert(displayName.size() > 0);
	return displayName;
}

int Holiday::getDayNumber() const
{
	int dayNumber = HolidayDates.at(this->getHolidayName()).second;
	assert(dayNumber >= 1);
	assert(dayNumber <= 30);
	return dayNumber;
}

MonthName Holiday::getMonthName() const
{
	auto monthName = HolidayDates.at(this->getHolidayName()).first;
	return monthName;
}
