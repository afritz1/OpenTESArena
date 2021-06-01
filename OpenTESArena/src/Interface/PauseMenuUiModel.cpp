#include <cmath>

#include "PauseMenuUiModel.h"
#include "../Game/Game.h"

namespace
{
	int VolumePercentToInteger(double percent)
	{
		return static_cast<int>(std::round(percent * 100.0));
	}
}

std::string PauseMenuUiModel::getSoundVolumeText(Game &game)
{
	const auto &options = game.getOptions();
	const double volumePercent = options.getAudio_SoundVolume();
	const int volumeInteger = VolumePercentToInteger(volumePercent);
	return std::to_string(volumeInteger);
}

std::string PauseMenuUiModel::getMusicVolumeText(Game &game)
{
	const auto &options = game.getOptions();
	const double volumePercent = options.getAudio_MusicVolume();
	const int volumeInteger = VolumePercentToInteger(volumePercent);
	return std::to_string(volumeInteger);
}

std::string PauseMenuUiModel::getOptionsButtonText(Game &game)
{
	return "OPTIONS";
}
