#ifndef DATE_H
#define DATE_H

#include <string>

#include "Month.h"
#include "Weekday.h"
#include "Year.h"

// The Date class manages all the data needed to define a particular day in the game.

// Perhaps a "DateManager" class will know all the names of months and weekdays so that
// they can be properly incremented? Those strings will need to be read from the Arena
// executable.

class Date
{
private:
	Year year;
	Month month;
	Weekday weekday;
	int day;
	// Seconds? Minutes? Maybe put that in a "TimeOfDay" class.

	// These increment methods are private so that the user doesn't accidentally make
	// the change twice.
	void incrementWeekday();
	void incrementMonth();
	void incrementYear();
public:
	// Date constructor for a fully defined date.
	Date(const Year &year, const Month &month, const Weekday &weekday, int day);
	~Date();

	const Year &getYear() const;
	const Month &getMonth() const;
	const Weekday &getWeekday() const;
	int getDayNumber() const;
	std::string getOrdinalDay() const;

	// No need for a "fullNameToString()" method because each name can be obtained
	// individually from the various member methods and concatenated together as
	// needed.

	// Only days can be incremented directly.
	void incrementDay();
};

#endif
