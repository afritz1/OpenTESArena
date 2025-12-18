#include "MainMenuUiController.h"
#include "MainMenuUiModel.h"
#include "MainMenuUiState.h"
#include "MainMenuUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../Rendering/Renderer.h"
#include "../UI/ArenaFontName.h"
#include "../UI/UiButton.h"
#include "../UI/UiLibrary.h"
#include "../UI/UiTextBox.h"

namespace
{
	std::string MakeTestTypeText(int testType)
	{
		return "Test type: " + MainMenuUiModel::getTestTypeName(testType);
	}

	std::string MakeTestLocationText(Game &game, int testType, int testIndex, int testIndex2)
	{
		return "Test location: " + MainMenuUiModel::getSelectedTestName(game, testType, testIndex, testIndex2);
	}

	std::string MakeTestWeatherText(int testWeather)
	{
		const ArenaWeatherType testWeatherType = MainMenuUiModel::getSelectedTestWeatherType(testWeather);
		return "Test weather: " + MainMenuUiModel::WeatherTypeNames.at(testWeatherType);
	}
}

MainMenuUiState::MainMenuUiState()
{
	this->game = nullptr;
	this->testType = -1;
	this->testIndex = -1;
	this->testIndex2 = -1;
	this->testWeather = -1;
}

void MainMenuUiState::init(Game &game)
{
	this->game = &game;
	this->testType = 0;
	this->testIndex = 0;
	this->testIndex2 = 1;
	this->testWeather = 0;
}

void MainMenuUI::create(Game &game)
{
	MainMenuUiState &state = MainMenuUI::state;
	state.init(game);

	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(MainMenuUI::ContextType);
	uiManager.createContext(contextDef, state.elements, inputManager, state.inputListeners, textureManager, renderer);

	MainMenuUI::updateTypeTextBox();
	MainMenuUI::updateNameTextBox();
	MainMenuUI::updateWeatherTextBox();
}

void MainMenuUI::destroy()
{
	MainMenuUiState &state = MainMenuUI::state;
	Game &game = *state.game;

	state.elements.free(game.uiManager, game.renderer);
	state.inputListeners.free(game.inputManager);
	state.testType = -1;
	state.testIndex = -1;
	state.testIndex2 = -1;
	state.testWeather = -1;
}

void MainMenuUI::update(double dt)
{
	const MainMenuUiState &state = MainMenuUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	const UiElementInstanceID testIndex2ImageElementInstID = uiManager.getElementByName("TestIndex2ArrowsImage");
	const UiElementInstanceID testIndex2UpButtonElementInstID = uiManager.getElementByName("TestIndex2UpButton");
	const UiElementInstanceID testIndex2DownButtonElementInstID = uiManager.getElementByName("TestIndex2DownButton");
	const bool currentTestIsInterior = state.testType == MainMenuUiModel::TestType_Interior;
	uiManager.setElementActive(testIndex2ImageElementInstID, currentTestIsInterior);
	uiManager.setElementActive(testIndex2UpButtonElementInstID, currentTestIsInterior);
	uiManager.setElementActive(testIndex2DownButtonElementInstID, currentTestIsInterior);

	const UiElementInstanceID testWeatherImageElementInstID = uiManager.getElementByName("TestWeatherArrowsImage");
	const UiElementInstanceID testWeatherTextBoxElementInstID = uiManager.getElementByName("TestWeatherTextBox");
	const UiElementInstanceID testWeatherUpButtonElementInstID = uiManager.getElementByName("TestWeatherUpButton");
	const UiElementInstanceID testWeatherDownButtonElementInstID = uiManager.getElementByName("TestWeatherDownButton");
	const bool currentTestHasWeather = (state.testType == MainMenuUiModel::TestType_City) || (state.testType == MainMenuUiModel::TestType_Wilderness);
	uiManager.setElementActive(testWeatherImageElementInstID, currentTestHasWeather);
	uiManager.setElementActive(testWeatherTextBoxElementInstID, currentTestHasWeather);
	uiManager.setElementActive(testWeatherUpButtonElementInstID, currentTestHasWeather);
	uiManager.setElementActive(testWeatherDownButtonElementInstID, currentTestHasWeather);

	const Renderer &renderer = game.renderer;
	const UiTextureID cursorTextureID = game.defaultCursorTextureID;
	const std::optional<Int2> cursorDims = renderer.tryGetUiTextureDims(cursorTextureID);
	DebugAssert(cursorDims.has_value());

	const Options &options = game.options;
	const double cursorScale = options.getGraphics_CursorScale();
	const Int2 cursorSize(
		static_cast<int>(static_cast<double>(cursorDims->x) * cursorScale),
		static_cast<int>(static_cast<double>(cursorDims->y) * cursorScale));
	uiManager.setTransformSize(game.cursorImageElementInstID, cursorSize);
	uiManager.setImageTexture(game.cursorImageElementInstID, cursorTextureID);
}

void MainMenuUI::updateTypeTextBox()
{
	const MainMenuUiState &state = MainMenuUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	const UiElementInstanceID textBoxElementInstID = uiManager.getElementByName("TestTypeTextBox");
	const std::string text = MakeTestTypeText(state.testType);
	uiManager.setTextBoxText(textBoxElementInstID, text.c_str());
}

void MainMenuUI::updateNameTextBox()
{
	const MainMenuUiState &state = MainMenuUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	const UiElementInstanceID textBoxElementInstID = uiManager.getElementByName("TestIndexTextBox");
	const std::string text = MakeTestLocationText(game, state.testType, state.testIndex, state.testIndex2);
	uiManager.setTextBoxText(textBoxElementInstID, text.c_str());
}

void MainMenuUI::updateWeatherTextBox()
{
	const MainMenuUiState &state = MainMenuUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	const UiElementInstanceID textBoxElementInstID = uiManager.getElementByName("TestWeatherTextBox");
	const std::string text = MakeTestWeatherText(state.testWeather);
	uiManager.setTextBoxText(textBoxElementInstID, text.c_str());
}

void MainMenuUI::onLoadGameButtonSelected(MouseButtonType mouseButtonType)
{
	const MainMenuUiState &state = MainMenuUI::state;
	MainMenuUiController::onLoadGameButtonSelected(*state.game);
}

void MainMenuUI::onNewGameButtonSelected(MouseButtonType mouseButtonType)
{
	const MainMenuUiState &state = MainMenuUI::state;
	MainMenuUiController::onNewGameButtonSelected(*state.game);
}

void MainMenuUI::onExitGameButtonSelected(MouseButtonType mouseButtonType)
{
	MainMenuUiController::onExitGameButtonSelected();
}

void MainMenuUI::onTestGameButtonSelected(MouseButtonType mouseButtonType)
{
	const MainMenuUiState &state = MainMenuUI::state;
	Game &game = *state.game;

	const std::string testName = MainMenuUiModel::getSelectedTestName(game, state.testType, state.testIndex, state.testIndex2);
	const std::optional<ArenaInteriorType> interiorType = MainMenuUiModel::getSelectedTestInteriorType(state.testType, state.testIndex);
	const ArenaWeatherType weatherType = MainMenuUiModel::getSelectedTestWeatherType(state.testWeather);
	const MapType mapType = MainMenuUiModel::getSelectedTestMapType(state.testType);
	MainMenuUiController::onQuickStartButtonSelected(game, state.testType, state.testIndex, testName, interiorType, weatherType, mapType);
}

void MainMenuUI::onTestTypeUpButtonSelected(MouseButtonType mouseButtonType)
{
	MainMenuUiState &state = MainMenuUI::state;
	MainMenuUiController::onTestTypeUpButtonSelected(&state.testType, &state.testIndex, &state.testIndex2, &state.testWeather);
	MainMenuUI::updateTypeTextBox();
	MainMenuUI::updateNameTextBox();
}

void MainMenuUI::onTestTypeDownButtonSelected(MouseButtonType mouseButtonType)
{
	MainMenuUiState &state = MainMenuUI::state;
	MainMenuUiController::onTestTypeDownButtonSelected(&state.testType, &state.testIndex, &state.testIndex2, &state.testWeather);
	MainMenuUI::updateTypeTextBox();
	MainMenuUI::updateNameTextBox();
}

void MainMenuUI::onTestIndexUpButtonSelected(MouseButtonType mouseButtonType)
{
	MainMenuUiState &state = MainMenuUI::state;
	MainMenuUiController::onTestIndexUpButtonSelected(&state.testType, &state.testIndex, &state.testIndex2);
	MainMenuUI::updateNameTextBox();
}

void MainMenuUI::onTestIndexDownButtonSelected(MouseButtonType mouseButtonType)
{
	MainMenuUiState &state = MainMenuUI::state;
	MainMenuUiController::onTestIndexDownButtonSelected(&state.testType, &state.testIndex, &state.testIndex2);
	MainMenuUI::updateNameTextBox();
}

void MainMenuUI::onTestIndex2UpButtonSelected(MouseButtonType mouseButtonType)
{
	MainMenuUiState &state = MainMenuUI::state;
	MainMenuUiController::onTestIndex2UpButtonSelected(state.testType, state.testIndex, &state.testIndex2);
	MainMenuUI::updateNameTextBox();
}

void MainMenuUI::onTestIndex2DownButtonSelected(MouseButtonType mouseButtonType)
{
	MainMenuUiState &state = MainMenuUI::state;
	MainMenuUiController::onTestIndex2DownButtonSelected(state.testType, state.testIndex, &state.testIndex2);
	MainMenuUI::updateNameTextBox();
}

void MainMenuUI::onTestWeatherUpButtonSelected(MouseButtonType mouseButtonType)
{
	MainMenuUiState &state = MainMenuUI::state;
	MainMenuUiController::onTestWeatherUpButtonSelected(state.testType, &state.testWeather);
	MainMenuUI::updateWeatherTextBox();
}

void MainMenuUI::onTestWeatherDownButtonSelected(MouseButtonType mouseButtonType)
{
	MainMenuUiState &state = MainMenuUI::state;
	MainMenuUiController::onTestWeatherDownButtonSelected(state.testType, &state.testWeather);
	MainMenuUI::updateWeatherTextBox();
}

void MainMenuUI::onLoadGameInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		MainMenuUI::onLoadGameButtonSelected(MouseButtonType::Left);
	}
}

void MainMenuUI::onNewGameInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		MainMenuUI::onNewGameButtonSelected(MouseButtonType::Left);
	}
}

void MainMenuUI::onExitGameInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		MainMenuUI::onExitGameButtonSelected(MouseButtonType::Left);
	}
}

void MainMenuUI::onTestGameInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		MainMenuUI::onTestGameButtonSelected(MouseButtonType::Left);
	}
}
