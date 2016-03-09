#ifndef MONTH_H
#define MONTH_H

#include <string>

#include "MonthName.h"

class Month
{
private:
	MonthName monthName;
public:
	Month(MonthName monthName);
	~Month();

	static const int DAYS_PER_MONTH;

	const MonthName &getMonthName() const;
	std::string toString() const;

	// For knowing when to increment the year.
	bool isLastMonthInYear() const;

	void incrementMonth();
};

#endif