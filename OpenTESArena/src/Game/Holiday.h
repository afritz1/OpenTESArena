#ifndef HOLIDAY_H
#define HOLIDAY_H

#include <cstdint>
#include <string>

enum class HolidayName;
enum class MonthName;

class Holiday
{
private:
	HolidayName holidayName;
public:
	Holiday(HolidayName holidayName);
	~Holiday();

	HolidayName getHolidayName() const;

	// These two methods together tell when a holiday is.
	int32_t getDayNumber() const;
	MonthName getMonthName() const;

	std::string toString() const;
};

#endif
