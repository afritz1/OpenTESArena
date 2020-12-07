#ifndef DATE_UTILS_H
#define DATE_UTILS_H

#include <string>

class Date;
class ExeData;

namespace DateUtils
{
	// Gets the date string for a given date, using strings from the executable data.
	std::string getDateString(const Date &date, const ExeData &exeData);
}

#endif
