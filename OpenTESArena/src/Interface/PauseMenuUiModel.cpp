#include <cmath>

#include "PauseMenuUiModel.h"

std::string PauseMenuUiModel::getVolumeString(double percent)
{
	const int volumeInteger = static_cast<int>(std::round(percent * 100.0));
	return std::to_string(volumeInteger);
}
