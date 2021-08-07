#ifndef MAIN_MENU_PANEL_H
#define MAIN_MENU_PANEL_H

#include <optional>
#include <string>

#include "Panel.h"
#include "../Assets/ArenaTypes.h"
#include "../UI/Button.h"

class Renderer;

enum class MapType;

class MainMenuPanel : public Panel
{
private:
	Button<Game&> loadButton, newButton;
	Button<> exitButton;
	Button<Game&, int, int, const std::string&,
		const std::optional<ArenaTypes::InteriorType>&, ArenaTypes::WeatherType, MapType> quickStartButton;
	Button<int*, int*, int*, int*> testTypeUpButton, testTypeDownButton;
	Button<int*, int*, int*> testIndexUpButton, testIndexDownButton;
	Button<int, int, int*> testIndex2UpButton, testIndex2DownButton;
	Button<int, int*> testWeatherUpButton, testWeatherDownButton;
	int testType, testIndex, testIndex2, testWeather; // Test values for quickstart.

	std::string getSelectedTestName() const;
	std::optional<ArenaTypes::InteriorType> getSelectedTestInteriorType() const;
	ArenaTypes::WeatherType getSelectedTestWeatherType() const;
	MapType getSelectedTestMapType() const;

	void renderTestUI(Renderer &renderer);
public:
	MainMenuPanel(Game &game);
	~MainMenuPanel() override;

	bool init();

	virtual std::optional<CursorData> getCurrentCursor() const override;
	virtual void render(Renderer &renderer) override;
};

#endif
