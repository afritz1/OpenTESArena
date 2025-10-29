#ifndef MAIN_MENU_PANEL_H
#define MAIN_MENU_PANEL_H

#include <optional>
#include <string>

#include "Panel.h"
#include "../Assets/ArenaTypes.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"

enum class MapType;

class MainMenuPanel : public Panel
{
private:
	Button<Game&> loadButton, newButton;
	Button<> exitButton;

	// Test.
	TextBox testButtonTextBox, testTypeTextBox, testNameTextBox, testWeatherTextBox;
	Button<Game&, int, int, const std::string&, const std::optional<ArenaInteriorType>&, ArenaWeatherType, MapType> quickStartButton;
	Button<int*, int*, int*, int*> testTypeUpButton, testTypeDownButton;
	Button<int*, int*, int*> testIndexUpButton, testIndexDownButton;
	Button<int, int, int*> testIndex2UpButton, testIndex2DownButton;
	Button<int, int*> testWeatherUpButton, testWeatherDownButton;
	ScopedUiTextureRef testArrowsTextureRef, testButtonTextureRef;
	int testType, testIndex, testIndex2, testWeather;

	void initTestUI();
public:
	MainMenuPanel(Game &game);
	~MainMenuPanel() override;

	bool init();
};

#endif
