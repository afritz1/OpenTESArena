#ifndef MAIN_MENU_PANEL_H
#define MAIN_MENU_PANEL_H

#include <optional>
#include <string>

#include "Panel.h"
#include "../Assets/ArenaTypes.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"
#include "../UI/UiContext.h"
#include "../UI/UiElement.h"

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

struct MainMenuUiState
{
	UiTextureID bgTextureID;

	// Test option state.
	UiTextureID testArrowsTextureID;
	UiTextureID testButtonTextureID;
	int testType, testIndex, testIndex2, testWeather;

	UiContextElements elements;

	MainMenuUiState();

	void allocate(UiManager &uiManager, TextureManager &textureManager, Renderer &renderer);
	void free(UiManager &uiManager, Renderer &renderer);
};

namespace MainMenuUI
{
	static constexpr UiContextType ContextType = UiContextType::MainMenu;

	static MainMenuUiState state;

	void create(Game &game);
	void destroy(Game &game);
}

#endif
