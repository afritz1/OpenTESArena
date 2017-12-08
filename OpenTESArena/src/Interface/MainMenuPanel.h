#ifndef MAIN_MENU_PANEL_H
#define MAIN_MENU_PANEL_H

#include <string>

#include "Button.h"
#include "Panel.h"

class Renderer;

enum class ClimateType;
enum class WeatherType;
enum class WorldType;

class MainMenuPanel : public Panel
{
private:
	Button<Game&> loadButton, newButton;
	Button<Game&, const std::string&, ClimateType, WeatherType, WorldType> quickStartButton;
	Button<> exitButton;
	Button<MainMenuPanel&> testTypeUpButton, testTypeDownButton, testIndexUpButton,
		testIndexDownButton, testIndex2UpButton, testIndex2DownButton, testClimateUpButton,
		testClimateDownButton, testWeatherUpButton, testWeatherDownButton;
	int testType, testIndex, testIndex2, testClimate, testWeather; // Test values for quickstart.

	std::string getSelectedTestName() const;
	ClimateType getSelectedTestClimateType() const;
	WeatherType getSelectedTestWeatherType() const;
	WorldType getSelectedTestWorldType() const;
public:
	MainMenuPanel(Game &game);
	virtual ~MainMenuPanel();

	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
