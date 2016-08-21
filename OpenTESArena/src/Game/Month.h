#ifndef MONTH_H
#define MONTH_H

#include <cstdint>
#include <string>

enum class MonthName;

class Month
{
private:
	MonthName monthName;
public:
	Month(MonthName monthName);
	~Month();

	static const int32_t DAYS_PER_MONTH;

	MonthName getMonthName() const;

	// For knowing when to increment the year.
	bool isLastMonthInYear() const;

	std::string toString() const;

	void incrementMonth();
};

#endif
