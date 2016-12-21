#ifndef HOLIDAY_H
#define HOLIDAY_H

#include <string>

// Holidays affect certain things in the game world, like prices at shops and temples.

// They will be read in from the Arena executable eventually.

class Holiday
{
private:
	std::string holidayName, monthName;
	int day;
public:
	Holiday(const std::string &holidayName, const std::string &monthName, int day);
	~Holiday();

	const std::string &getHolidayName() const;

	// These two methods together tell when a holiday is.
	const std::string &getMonthName() const;
	int getDay() const;
};

#endif
