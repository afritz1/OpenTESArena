#ifndef HOLIDAY_NAME_H
#define HOLIDAY_NAME_H

// If holidays are imported from a text file, their behaviors will need to be made
// into general events of some kind that are in use for a given day. 

// Maybe it could be implemented within each specific function's properties; that is, 
// it's not some kind of "global" event affecting everything, but if it's, say, the
// holiday that makes shop items half price, then that price deduction is determined
// when the shopkeeper makes their offer by looking it up somewhere with parameters.

enum class HolidayName
{
	NewLifeFestival,
	SouthWindsPrayer,
	HeartsDay,
	FirstPlanting,
	JestersDay,
	SecondPlanting,
	MidYearCelebration,
	MerchantsFestival,
	SunsRest,
	HarvestsEnd,
	TalesAndTallows,
	WitchesFestival,
	EmperorsDay,
	WarriorsFestival,
	NorthWindsPrayer
};

#endif
