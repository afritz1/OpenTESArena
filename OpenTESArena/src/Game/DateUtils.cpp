#include "Date.h"
#include "DateUtils.h"
#include "../Assets/ExeData.h"

#include "components/debug/Debug.h"

std::string DateUtils::getDateString(const Date &date, const ExeData &exeData)
{
	std::string text = exeData.status.date;

	// Replace first %s with weekday.
	const auto &weekdayNames = exeData.calendar.weekdayNames;
	const int weekday = date.getWeekday();
	DebugAssertIndex(weekdayNames, weekday);
	const std::string &weekdayString = weekdayNames[weekday];
	size_t index = text.find("%s");
	text.replace(index, 2, weekdayString);

	// Replace %u%s with day and ordinal suffix.
	const std::string dayString = date.getOrdinalDay();
	index = text.find("%u%s");
	text.replace(index, 4, dayString);

	// Replace third %s with month.
	const auto &monthNames = exeData.calendar.monthNames;
	const int month = date.getMonth();
	DebugAssertIndex(monthNames, month);
	const std::string &monthString = monthNames[month];
	index = text.find("%s");
	text.replace(index, 2, monthString);

	// Replace %d with year.
	index = text.find("%d");
	text.replace(index, 2, std::to_string(date.getYear()));

	return text;
}
