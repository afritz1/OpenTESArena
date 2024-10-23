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

std::string PauseMenuUiModel::getVolumeString(double percent)
{
	const int volumeInteger = VolumePercentToInteger(percent);
	return std::to_string(volumeInteger);
}

std::string PauseMenuUiModel::getSoundVolumeText(Game &game)
{
	const auto &options = game.options;
	const double volumePercent = options.getAudio_SoundVolume();
	return PauseMenuUiModel::getVolumeString(volumePercent);
}

std::string PauseMenuUiModel::getMusicVolumeText(Game &game)
{
	const auto &options = game.options;
	const double volumePercent = options.getAudio_MusicVolume();
	return PauseMenuUiModel::getVolumeString(volumePercent);
}

std::string PauseMenuUiModel::getOptionsButtonText(Game &game)
{
	return "OPTIONS";
}
