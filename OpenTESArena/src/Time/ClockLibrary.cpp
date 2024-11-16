#include <charconv>
#include <string>

#include "components/utilities/KeyValueFile.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"

#include "ClockLibrary.h"

bool ClockLibrary::init(const char *filename)
{
	KeyValueFile keyValueFile;
	if (!keyValueFile.init(filename))
	{
		DebugLogError("Couldn't init KeyValueFile \"" + std::string(filename) + "\".");
		return false;
	}

	const KeyValueFileSection &section = keyValueFile.getSection(0);
	const int clockCount = section.getPairCount();
	this->clocks.init(clockCount);
	this->clockNames.init(clockCount);

	for (int i = 0; i < clockCount; i++)
	{
		const std::pair<std::string, std::string> &pair = section.getPair(i);
		const std::string &clockKey = pair.first;
		const std::string &clockValue = pair.second;
		const Buffer<std::string> valueTokens = String::split(clockValue, ',');
		if (valueTokens.getCount() != 3)
		{
			DebugLogError("Invalid clock value \"" + clockValue + "\".");
			continue;
		}

		const std::string &hoursToken = valueTokens.get(0);
		const std::string &minutesToken = valueTokens.get(1);
		const std::string &secondsToken = valueTokens.get(2);

		int hours, minutes, seconds;
		const std::from_chars_result hoursParse = std::from_chars(hoursToken.data(), hoursToken.data() + hoursToken.size(), hours);
		const std::from_chars_result minutesParse = std::from_chars(minutesToken.data(), minutesToken.data() + minutesToken.size(), minutes);
		const std::from_chars_result secondsParse = std::from_chars(secondsToken.data(), secondsToken.data() + secondsToken.size(), seconds);
		if ((hoursParse.ec != std::errc()) || (minutesParse.ec != std::errc()) || (secondsParse.ec != std::errc()))
		{
			DebugLogError("Couldn't parse clock value \"" + clockValue + "\".");
			continue;
		}

		this->clocks.get(i).init(hours, minutes, seconds);
		this->clockNames.set(i, clockKey);
	}

	return true;
}

const Clock &ClockLibrary::getClock(const char *name) const
{
	DebugAssert(this->clocks.getCount() == this->clockNames.getCount());

	const std::string_view nameView(name);

	for (int i = 0; i < this->clockNames.getCount(); i++)
	{
		const std::string &clockName = this->clockNames.get(i);
		if (StringView::caseInsensitiveEquals(clockName, nameView))
		{
			return this->clocks.get(i);
		}
	}

	DebugLogError("Couldn't find clock \"" + std::string(nameView) + "\".");
	return this->clocks.get(0);
}
