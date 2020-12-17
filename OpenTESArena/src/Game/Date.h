#ifndef DATE_H
#define DATE_H

#include <string>

// The Date class manages all the data needed to identify a particular date in the game.

// The month and weekday are both zero-based so they can index into the executable strings.

class Date
{
private:
	int year;
	int month;
	int day;
public:
	Date(int year, int month, int day);

	// Starts in default year.
	Date(int month, int day);

	// Starts on first month and day.
	Date();

	static constexpr int INITIAL_YEAR = 389; // @todo: put these in an ArenaDateUtils?
	static constexpr int MONTHS_PER_YEAR = 12;
	static constexpr int DAYS_PER_MONTH = 30;
	static constexpr int DAYS_PER_WEEK = 7;

	int getYear() const; // One-based (i.e., 1->1000, not 0->999).
	int getMonth() const; // Zero-based for name indexing.
	int getWeekday() const; // Zero-based for name indexing.
	int getDay() const; // Zero-based.
	std::string getOrdinalDay() const;
	int getSeason() const; // March is 0 (spring).

	void incrementYear();
	void incrementMonth();
	void incrementDay();
};

#endif
