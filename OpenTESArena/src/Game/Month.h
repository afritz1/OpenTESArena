#ifndef MONTH_H
#define MONTH_H

#include <string>

// Another part of the program will need to implement "isLastMonthInYear()" and 
// "incrementMonth()" eventually.
// - Maybe "incrementMonth()" could be implemented as "const std::string &nextMonth(
//   const std::string &monthName)".

// By reading certain strings from the Arena executable, a list of months can 
// be generated.

class Month
{
private:
	std::string monthName;
public:
	Month(const std::string &monthName);
	~Month();

	static const int DAYS_PER_MONTH;

	const std::string &getMonthName() const;
};

#endif
