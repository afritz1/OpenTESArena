#ifndef MAIN_MENU_PANEL_H
#define MAIN_MENU_PANEL_H

#include <optional>
#include <string>

#include "Button.h"
#include "Panel.h"
#include "../Assets/ArenaTypes.h"
#include "../World/VoxelDefinition.h"

class Renderer;

enum class WeatherType;
enum class WorldType;

class MainMenuPanel : public Panel
{
private:
	Button<Game&> loadButton, newButton;
	Button<Game&, int, int, const std::string&,
		const std::optional<ArenaTypes::MenuType>&, WeatherType, WorldType> quickStartButton;
	Button<> exitButton;
	Button<MainMenuPanel&> testTypeUpButton, testTypeDownButton, testIndexUpButton,
		testIndexDownButton, testIndex2UpButton, testIndex2DownButton, testWeatherUpButton,
		testWeatherDownButton;
	int testType, testIndex, testIndex2, testWeather; // Test values for quickstart.

	std::string getSelectedTestName() const;
	std::optional<ArenaTypes::MenuType> getSelectedTestInteriorType() const;
	WeatherType getSelectedTestWeatherType() const;
	WorldType getSelectedTestWorldType() const;
public:
	MainMenuPanel(Game &game);
	virtual ~MainMenuPanel() = default;

	virtual Panel::CursorData getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
