#ifndef MAIN_MENU_UI_CONTROLLER_H
#define MAIN_MENU_UI_CONTROLLER_H

#include <optional>
#include <string>

#include "../Assets/ArenaTypes.h"

class Game;
class MainMenuPanel;

enum class MapType;

namespace MainMenuUiController
{
	void onLoadGameButtonSelected(Game &game);
	void onNewGameButtonSelected(Game &game);
	void onExitGameButtonSelected();

	void onQuickStartButtonSelected(Game &game, int testType, int testIndex, const std::string &mifName,
		const std::optional<ArenaTypes::InteriorType> &optInteriorType, ArenaTypes::WeatherType weatherType,
		MapType mapType);

	// Takes pointers to MainMenuPanel members due to them being private.
	void onTestTypeUpButtonSelected(int *testType, int *testIndex, int *testIndex2, int *testWeather);
	void onTestTypeDownButtonSelected(int *testType, int *testIndex, int *testIndex2, int *testWeather);
	void onTestIndexUpButtonSelected(int *testType, int *testIndex, int *testIndex2);
	void onTestIndexDownButtonSelected(int *testType, int *testIndex, int *testIndex2);
	void onTestIndex2UpButtonSelected(int testType, int testIndex, int *testIndex2);
	void onTestIndex2DownButtonSelected(int testType, int testIndex, int *testIndex2);
	void onTestWeatherUpButtonSelected(int testType, int *testWeather);
	void onTestWeatherDownButtonSelected(int testType, int *testWeather);
}

#endif
