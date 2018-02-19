#ifndef DATE_H
#define DATE_H

#include <string>

// The Date class manages all the data needed to identify a particular date in the game.

// The month and weekday are both zero-based so they can index into the executable strings.

class Date
{
private:
	int era;
	int year;
	int month;
	int day;
public:
	Date(int era, int year, int month, int day);

	// Starts in default era and year.
	Date(int month, int day);

	// Starts on first month and day.
	Date();

	~Date();

	static const int INITIAL_ERA;
	static const int INITIAL_YEAR;
	static const int YEARS_PER_ERA;
	static const int MONTHS_PER_YEAR;
	static const int DAYS_PER_MONTH;
	static const int DAYS_PER_WEEK;

	int getEra() const;
	int getYear() const; // One-based (i.e., 1->1000, not 0->999).
	int getMonth() const; // Zero-based for name indexing.
	int getWeekday() const; // Zero-based for name indexing.
	int getDay() const; // Zero-based.
	std::string getOrdinalDay() const;

	void incrementEra();
	void incrementYear();
	void incrementMonth();
	void incrementDay();
};

#endif
