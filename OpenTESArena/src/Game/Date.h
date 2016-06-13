#ifndef DATE_H
#define DATE_H

#include <memory>
#include <string>

class Month;
class Weekday;
class Year;

enum class MonthName;
enum class WeekdayName;

class Date
{
private:
	std::unique_ptr<Year> year;
	std::unique_ptr<Month> month;
	std::unique_ptr<Weekday> weekday;
	int day;
	// Seconds? Minutes? Maybe put that in a "TimeOfDay" class.

	// These increment methods are private so that the user doesn't accidentally make
	// the change twice.
	void incrementWeekday();
	void incrementMonth();
	void incrementYear();
public:
	// Date constructor for a fully defined date.
	Date(int day, WeekdayName weekdayName, MonthName monthName, const Year &year);
	Date(const Date &date);
	~Date();

	int getDayNumber() const;
	const Weekday &getWeekday() const;
	const Month &getMonth() const;
	const Year &getYear() const;
	std::string getOrdinalDay() const;

	// No need for a "fullNameToString()" method because each name can be obtained
	// individually from the various member methods and concatenated together as
	// needed.

	// Only days can be incremented directly.
	void incrementDay();
};

#endif
