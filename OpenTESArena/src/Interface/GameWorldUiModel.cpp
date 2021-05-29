#include "GameWorldUiModel.h"
#include "../Entities/Player.h"
#include "../Game/ArenaClockUtils.h"
#include "../Game/ArenaDateUtils.h"
#include "../Game/Game.h"

#include "components/utilities/String.h"

std::string GameWorldUiModel::getPlayerNameText(Game &game)
{
	auto &gameState = game.getGameState();
	const auto &player = gameState.getPlayer();
	return player.getFirstName();
}

std::string GameWorldUiModel::getStatusButtonText(Game &game)
{
	auto &gameState = game.getGameState();
	const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
	const auto &exeData = binaryAssetLibrary.getExeData();
	const LocationDefinition &locationDef = gameState.getLocationDefinition();
	const LocationInstance &locationInst = gameState.getLocationInstance();
	const std::string &locationName = locationInst.getName(locationDef);

	const std::string timeString = [&game, &gameState, &exeData]()
	{
		const Clock &clock = gameState.getClock();
		const int hours = clock.getHours12();
		const int minutes = clock.getMinutes();
		const std::string clockTimeString = std::to_string(hours) + ":" +
			((minutes < 10) ? "0" : "") + std::to_string(minutes);

		const int timeOfDayIndex = [&gameState]()
		{
			// Arena has eight time ranges for each time of day. They aren't
			// uniformly distributed -- midnight and noon are only one minute.
			const std::array<std::pair<Clock, int>, 8> clocksAndIndices =
			{
				std::make_pair(ArenaClockUtils::Midnight, 6),
				std::make_pair(ArenaClockUtils::Night1, 5),
				std::make_pair(ArenaClockUtils::EarlyMorning, 0),
				std::make_pair(ArenaClockUtils::Morning, 1),
				std::make_pair(ArenaClockUtils::Noon, 2),
				std::make_pair(ArenaClockUtils::Afternoon, 3),
				std::make_pair(ArenaClockUtils::Evening, 4),
				std::make_pair(ArenaClockUtils::Night2, 5)
			};

			const Clock &presentClock = gameState.getClock();

			// Reverse iterate, checking which range the active clock is in.
			const auto pairIter = std::find_if(
				clocksAndIndices.rbegin(), clocksAndIndices.rend(),
				[&presentClock](const std::pair<Clock, int> &pair)
			{
				const Clock &clock = pair.first;
				return presentClock.getTotalSeconds() >= clock.getTotalSeconds();
			});

			DebugAssertMsg(pairIter != clocksAndIndices.rend(), "No valid time of day.");
			return pairIter->second;
		}();

		const std::string &timeOfDayString =
			exeData.calendar.timesOfDay.at(timeOfDayIndex);

		return clockTimeString + ' ' + timeOfDayString;
	}();

	// Get the base status text.
	std::string baseText = exeData.status.popUp;

	// Replace carriage returns with newlines.
	baseText = String::replace(baseText, '\r', '\n');

	// Replace first %s with location name.
	size_t index = baseText.find("%s");
	baseText.replace(index, 2, locationName);

	// Replace second %s with time string.
	index = baseText.find("%s", index);
	baseText.replace(index, 2, timeString);

	// Replace third %s with date string, filled in with each value.
	std::string dateString = ArenaDateUtils::makeDateString(gameState.getDate(), exeData);
	dateString.back() = '\n'; // Replace \r with \n.

	index = baseText.find("%s", index);
	baseText.replace(index, 2, dateString);

	// Replace %d's with current and total weight.
	const int currentWeight = 0;
	index = baseText.find("%d", index);
	baseText.replace(index, 2, std::to_string(currentWeight));

	const int weightCapacity = 0;
	index = baseText.find("%d", index);
	baseText.replace(index, 2, std::to_string(weightCapacity));

	// Append the list of effects at the bottom (healthy/diseased...).
	const std::string effectText = [&exeData]()
	{
		std::string text = exeData.status.effect;

		// Replace carriage returns with newlines.
		text = String::replace(text, '\r', '\n');

		// Replace %s with placeholder.
		const std::string &effectStr = exeData.status.effectsList.front();
		size_t index = text.find("%s");
		text.replace(index, 2, effectStr);

		// Remove newline on end.
		text.pop_back();

		return text;
	}();

	return baseText + effectText;
}
