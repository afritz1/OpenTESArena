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
	this->testType = -1;
	this->testIndex = -1;
	this->testIndex2 = -1;
	this->testWeather = -1;
}

void MainMenuUiState::allocate(UiManager &uiManager, TextureManager &textureManager, Renderer &renderer)
{

}

void MainMenuUiState::free(UiManager &uiManager, InputManager &inputManager, Renderer &renderer)
{
	this->elements.free(uiManager, renderer);
	this->inputListeners.free(inputManager);
}

void MainMenuUI::create(Game &game)
{
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	UiManager &uiManager = game.uiManager;
	MainMenuUiState &state = MainMenuUI::state;
	state.allocate(uiManager, game.textureManager, renderer);

	state.testType = 0;
	state.testIndex = 0;
	state.testIndex2 = 1;
	state.testWeather = 0;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(MainMenuUI::ContextType);
	uiManager.createContext(contextDef, state.elements, inputManager, state.inputListeners, textureManager, renderer);

	/*UiElementInitInfo bgImageElementInitInfo;
	uiManager.createImage(bgImageElementInitInfo, state.bgTextureID, contextType, state.elements);*/

	/*const Rect testGameButtonRect = MainMenuUiView::getTestButtonRect();
	const Rect testTypeUpButtonRect = MainMenuUiView::getTestTypeUpButtonRect();
	const Rect testTypeDownButtonRect = MainMenuUiView::getTestTypeDownButtonRect();
	const Rect testIndexUpButtonRect = MainMenuUiView::getTestIndexUpButtonRect();
	const Rect testIndexDownButtonRect = MainMenuUiView::getTestIndexDownButtonRect();
	const Rect testIndex2UpButtonRect = MainMenuUiView::getTestIndex2UpButtonRect();
	const Rect testIndex2DownButtonRect = MainMenuUiView::getTestIndex2DownButtonRect();
	const Rect testWeatherUpButtonRect = MainMenuUiView::getTestWeatherUpButtonRect();
	const Rect testWeatherDownButtonRect = MainMenuUiView::getTestWeatherDownButtonRect();

	UiElementInitInfo testGameButtonImageElementInitInfo;
	testGameButtonImageElementInitInfo.position = testGameButtonRect.getCenter();
	testGameButtonImageElementInitInfo.pivotType = UiPivotType::Middle;
	testGameButtonImageElementInitInfo.drawOrder = 1;
	const UiElementInstanceID testGameButtonImageElementInstID = uiManager.createImage(testGameButtonImageElementInitInfo, state.testButtonTextureID, contextType, state.elements);

	UiElementInitInfo testGameButtonTextBoxElementInitInfo;
	testGameButtonTextBoxElementInitInfo.position = testGameButtonRect.getCenter();
	testGameButtonTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	testGameButtonTextBoxElementInitInfo.drawOrder = 2;

	UiTextBoxInitInfo testGameButtonTextBoxInitInfo;
	testGameButtonTextBoxInitInfo.worstCaseText = std::string(5, TextRenderUtils::LARGEST_CHAR);
	testGameButtonTextBoxInitInfo.text = "Test";
	testGameButtonTextBoxInitInfo.fontName = MainMenuUiView::TestButtonFontName.c_str();
	testGameButtonTextBoxInitInfo.defaultColor = MainMenuUiView::getTestButtonTextColor();
	testGameButtonTextBoxInitInfo.alignment = MainMenuUiView::TestButtonTextAlignment;
	uiManager.createTextBox(testGameButtonTextBoxElementInitInfo, testGameButtonTextBoxInitInfo, contextType, state.elements, renderer);

	UiElementInitInfo testTypeArrowImageElementInitInfo;
	testTypeArrowImageElementInitInfo.position = testTypeUpButtonRect.getTopLeft();
	testTypeArrowImageElementInitInfo.drawOrder = 2;
	uiManager.createImage(testTypeArrowImageElementInitInfo, state.testArrowsTextureID, contextType, state.elements);

	UiElementInitInfo testIndexArrowImageElementInitInfo;
	testIndexArrowImageElementInitInfo.position = testIndexUpButtonRect.getTopLeft();
	testIndexArrowImageElementInitInfo.drawOrder = 2;
	uiManager.createImage(testIndexArrowImageElementInitInfo, state.testArrowsTextureID, contextType, state.elements);

	UiElementInitInfo testIndex2ArrowImageElementInitInfo;
	testIndex2ArrowImageElementInitInfo.position = testIndex2UpButtonRect.getTopLeft();
	testIndex2ArrowImageElementInitInfo.drawOrder = 2;
	state.testIndex2ImageElementInstID = uiManager.createImage(testIndex2ArrowImageElementInitInfo, state.testArrowsTextureID, contextType, state.elements);

	UiElementInitInfo testWeatherArrowImageElementInitInfo;
	testWeatherArrowImageElementInitInfo.position = testWeatherUpButtonRect.getTopLeft();
	testWeatherArrowImageElementInitInfo.drawOrder = 2;
	state.testWeatherImageElementInstID = uiManager.createImage(testWeatherArrowImageElementInitInfo, state.testArrowsTextureID, contextType, state.elements);

	UiElementInitInfo testTypeTextBoxElementInitInfo;
	testTypeTextBoxElementInitInfo.position = testTypeUpButtonRect.getBottomLeft() - Int2(2, 0);
	testTypeTextBoxElementInitInfo.pivotType = UiPivotType::MiddleRight;
	testTypeTextBoxElementInitInfo.drawOrder = 3;

	UiTextBoxInitInfo testTypeTextBoxInitInfo;
	testTypeTextBoxInitInfo.worstCaseText = TextRenderUtils::makeWorstCaseText(15);
	testTypeTextBoxInitInfo.text = MakeTestTypeText(state.testType);
	testTypeTextBoxInitInfo.fontName = MainMenuUiView::TestButtonFontName.c_str();
	testTypeTextBoxInitInfo.defaultColor = MainMenuUiView::getTestButtonTextColor();
	testTypeTextBoxInitInfo.alignment = TextAlignment::MiddleRight;
	const UiElementInstanceID testTypeTextBoxElementInstID = uiManager.createTextBox(testTypeTextBoxElementInitInfo, testTypeTextBoxInitInfo, contextType, state.elements, renderer);

	UiElementInitInfo testNameTextBoxElementInitInfo;
	testNameTextBoxElementInitInfo.position = testIndexUpButtonRect.getBottomLeft() - Int2(2, 0);
	testNameTextBoxElementInitInfo.pivotType = UiPivotType::MiddleRight;
	testNameTextBoxElementInitInfo.drawOrder = 3;

	UiTextBoxInitInfo testNameTextBoxInitInfo;
	testNameTextBoxInitInfo.worstCaseText = TextRenderUtils::makeWorstCaseText(15);
	testNameTextBoxInitInfo.text = MakeTestLocationText(game, state.testType, state.testIndex, state.testIndex2);
	testNameTextBoxInitInfo.fontName = MainMenuUiView::TestButtonFontName.c_str();
	testNameTextBoxInitInfo.defaultColor = MainMenuUiView::getTestButtonTextColor();
	testNameTextBoxInitInfo.alignment = TextAlignment::MiddleRight;
	const UiElementInstanceID testNameTextBoxElementInstID = uiManager.createTextBox(testNameTextBoxElementInitInfo, testNameTextBoxInitInfo, contextType, state.elements, renderer);

	UiElementInitInfo testWeatherTextBoxElementInitInfo;
	testWeatherTextBoxElementInitInfo.position = testWeatherUpButtonRect.getBottomLeft() - Int2(2, 0);
	testWeatherTextBoxElementInitInfo.pivotType = UiPivotType::MiddleRight;
	testWeatherTextBoxElementInitInfo.drawOrder = 3;

	UiTextBoxInitInfo testWeatherTextBoxInitInfo;
	testWeatherTextBoxInitInfo.worstCaseText = TextRenderUtils::makeWorstCaseText(16);
	testWeatherTextBoxInitInfo.text = MakeTestWeatherText(state.testWeather);
	testWeatherTextBoxInitInfo.fontName = MainMenuUiView::TestButtonFontName.c_str();
	testWeatherTextBoxInitInfo.defaultColor = MainMenuUiView::getTestButtonTextColor();
	testWeatherTextBoxInitInfo.alignment = TextAlignment::MiddleRight;
	state.testWeatherTextBoxElementInstID = uiManager.createTextBox(testWeatherTextBoxElementInitInfo, testWeatherTextBoxInitInfo, contextType, state.elements, renderer);

	const Rect loadGameButtonRect = MainMenuUiView::getLoadButtonRect();
	const Rect newGameButtonRect = MainMenuUiView::getNewGameButtonRect();
	const Rect exitGameButtonRect = MainMenuUiView::getExitButtonRect();

	UiElementInitInfo loadGameButtonElementInitInfo;
	loadGameButtonElementInitInfo.position = loadGameButtonRect.getTopLeft();
	loadGameButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	loadGameButtonElementInitInfo.size = loadGameButtonRect.getSize();

	UiButtonInitInfo loadGameButtonInitInfo;
	loadGameButtonInitInfo.callback = [&game](MouseButtonType) { MainMenuUiController::onLoadGameButtonSelected(game); };
	state.loadGameButtonElementInstID = uiManager.createButton(loadGameButtonElementInitInfo, loadGameButtonInitInfo, contextType, state.elements);

	UiElementInitInfo newGameButtonElementInitInfo;
	newGameButtonElementInitInfo.position = newGameButtonRect.getTopLeft();
	newGameButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	newGameButtonElementInitInfo.size = newGameButtonRect.getSize();

	UiButtonInitInfo newGameButtonInitInfo;
	newGameButtonInitInfo.callback = [&game](MouseButtonType) { MainMenuUiController::onNewGameButtonSelected(game); };
	state.newGameButtonElementInstID = uiManager.createButton(newGameButtonElementInitInfo, newGameButtonInitInfo, contextType, state.elements);

	UiElementInitInfo exitGameButtonElementInitInfo;
	exitGameButtonElementInitInfo.position = exitGameButtonRect.getTopLeft();
	exitGameButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	exitGameButtonElementInitInfo.size = exitGameButtonRect.getSize();

	UiButtonInitInfo exitGameButtonInitInfo;
	exitGameButtonInitInfo.callback = [](MouseButtonType) { MainMenuUiController::onExitGameButtonSelected(); };
	state.exitGameButtonElementInstID = uiManager.createButton(exitGameButtonElementInitInfo, exitGameButtonInitInfo, contextType, state.elements);

	UiElementInitInfo testGameButtonElementInitInfo;
	testGameButtonElementInitInfo.position = testGameButtonRect.getTopLeft();

	UiButtonInitInfo testGameButtonInitInfo;
	testGameButtonInitInfo.callback = [&game, &state](MouseButtonType)
	{
		MainMenuUiController::onQuickStartButtonSelected(
			game,
			state.testType,
			state.testIndex,
			MainMenuUiModel::getSelectedTestName(game, state.testType, state.testIndex, state.testIndex2),
			MainMenuUiModel::getSelectedTestInteriorType(state.testType, state.testIndex),
			MainMenuUiModel::getSelectedTestWeatherType(state.testWeather),
			MainMenuUiModel::getSelectedTestMapType(state.testType));
	};

	testGameButtonInitInfo.contentElementInstID = testGameButtonImageElementInstID;
	state.testGameButtonElementInstID = uiManager.createButton(testGameButtonElementInitInfo, testGameButtonInitInfo, contextType, state.elements);

	auto updateTypeTextBox = [&uiManager, &state, testTypeTextBoxElementInstID]()
	{
		// @todo lookup text box element id by hardcoded string instead
		const std::string text = MakeTestTypeText(state.testType);
		uiManager.setTextBoxText(testTypeTextBoxElementInstID, text.c_str());
	};

	auto updateNameTextBox = [&game, &uiManager, &state, testNameTextBoxElementInstID]()
	{
		// @todo lookup text box element id by hardcoded string instead
		const std::string text = MakeTestLocationText(game, state.testType, state.testIndex, state.testIndex2);
		uiManager.setTextBoxText(testNameTextBoxElementInstID, text.c_str());
	};

	auto updateWeatherTextBox = [&uiManager, &state]()
	{
		// @todo lookup text box element id by hardcoded string instead
		const std::string text = MakeTestWeatherText(state.testWeather);
		uiManager.setTextBoxText(state.testWeatherTextBoxElementInstID, text.c_str());
	};

	UiElementInitInfo testTypeUpButtonElementInitInfo;
	testTypeUpButtonElementInitInfo.position = testTypeUpButtonRect.getTopLeft();
	testTypeUpButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	testTypeUpButtonElementInitInfo.size = testTypeUpButtonRect.getSize();

	UiButtonInitInfo testTypeUpButtonInitInfo;
	testTypeUpButtonInitInfo.callback = [&state, updateTypeTextBox, updateNameTextBox](MouseButtonType)
	{
		MainMenuUiController::onTestTypeUpButtonSelected(&state.testType, &state.testIndex, &state.testIndex2, &state.testWeather);
		updateTypeTextBox();
		updateNameTextBox();
	};

	uiManager.createButton(testTypeUpButtonElementInitInfo, testTypeUpButtonInitInfo, contextType, state.elements);

	UiElementInitInfo testTypeDownButtonElementInitInfo;
	testTypeDownButtonElementInitInfo.position = testTypeDownButtonRect.getTopLeft();
	testTypeDownButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	testTypeDownButtonElementInitInfo.size = testTypeDownButtonRect.getSize();

	UiButtonInitInfo testTypeDownButtonInitInfo;
	testTypeDownButtonInitInfo.callback = [&state, updateTypeTextBox, updateNameTextBox](MouseButtonType)
	{
		MainMenuUiController::onTestTypeDownButtonSelected(&state.testType, &state.testIndex, &state.testIndex2, &state.testWeather);
		updateTypeTextBox();
		updateNameTextBox();
	};

	uiManager.createButton(testTypeDownButtonElementInitInfo, testTypeDownButtonInitInfo, contextType, state.elements);

	UiElementInitInfo testIndexUpButtonElementInitInfo;
	testIndexUpButtonElementInitInfo.position = testIndexUpButtonRect.getTopLeft();
	testIndexUpButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	testIndexUpButtonElementInitInfo.size = testIndexUpButtonRect.getSize();

	UiButtonInitInfo testIndexUpButtonInitInfo;
	testIndexUpButtonInitInfo.callback = [&state, updateNameTextBox](MouseButtonType)
	{
		MainMenuUiController::onTestIndexUpButtonSelected(&state.testType, &state.testIndex, &state.testIndex2);
		updateNameTextBox();
	};

	uiManager.createButton(testIndexUpButtonElementInitInfo, testIndexUpButtonInitInfo, contextType, state.elements);

	UiElementInitInfo testIndexDownButtonElementInitInfo;
	testIndexDownButtonElementInitInfo.position = testIndexDownButtonRect.getTopLeft();
	testIndexDownButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	testIndexDownButtonElementInitInfo.size = testIndexDownButtonRect.getSize();

	UiButtonInitInfo testIndexDownButtonInitInfo;
	testIndexDownButtonInitInfo.callback = [&state, updateNameTextBox](MouseButtonType)
	{
		MainMenuUiController::onTestIndexDownButtonSelected(&state.testType, &state.testIndex, &state.testIndex2);
		updateNameTextBox();
	};

	uiManager.createButton(testIndexDownButtonElementInitInfo, testIndexDownButtonInitInfo, contextType, state.elements);

	UiElementInitInfo testIndex2UpButtonElementInitInfo;
	testIndex2UpButtonElementInitInfo.position = testIndex2UpButtonRect.getTopLeft();
	testIndex2UpButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	testIndex2UpButtonElementInitInfo.size = testIndex2UpButtonRect.getSize();

	UiButtonInitInfo testIndex2UpButtonInitInfo;
	testIndex2UpButtonInitInfo.callback = [&state, updateNameTextBox](MouseButtonType)
	{
		MainMenuUiController::onTestIndex2UpButtonSelected(state.testType, state.testIndex, &state.testIndex2);
		updateNameTextBox();
	};

	state.testIndex2UpButtonElementInstID = uiManager.createButton(testIndex2UpButtonElementInitInfo, testIndex2UpButtonInitInfo, contextType, state.elements);

	UiElementInitInfo testIndex2DownButtonElementInitInfo;
	testIndex2DownButtonElementInitInfo.position = testIndex2DownButtonRect.getTopLeft();
	testIndex2DownButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	testIndex2DownButtonElementInitInfo.size = testIndex2DownButtonRect.getSize();

	UiButtonInitInfo testIndex2DownButtonInitInfo;
	testIndex2DownButtonInitInfo.callback = [&state, updateNameTextBox](MouseButtonType)
	{
		MainMenuUiController::onTestIndex2DownButtonSelected(state.testType, state.testIndex, &state.testIndex2);
		updateNameTextBox();
	};

	state.testIndex2DownButtonElementInstID = uiManager.createButton(testIndex2DownButtonElementInitInfo, testIndex2DownButtonInitInfo, contextType, state.elements);

	UiElementInitInfo testWeatherUpButtonElementInitInfo;
	testWeatherUpButtonElementInitInfo.position = testWeatherUpButtonRect.getTopLeft();
	testWeatherUpButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	testWeatherUpButtonElementInitInfo.size = testWeatherUpButtonRect.getSize();

	UiButtonInitInfo testWeatherUpButtonInitInfo;
	testWeatherUpButtonInitInfo.callback = [&state, updateWeatherTextBox](MouseButtonType)
	{
		MainMenuUiController::onTestWeatherUpButtonSelected(state.testType, &state.testWeather);
		updateWeatherTextBox();
	};

	state.testWeatherUpButtonElementInstID = uiManager.createButton(testWeatherUpButtonElementInitInfo, testWeatherUpButtonInitInfo, contextType, state.elements);

	UiElementInitInfo testWeatherDownButtonElementInitInfo;
	testWeatherDownButtonElementInitInfo.position = testWeatherDownButtonRect.getTopLeft();
	testWeatherDownButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	testWeatherDownButtonElementInitInfo.size = testWeatherDownButtonRect.getSize();

	UiButtonInitInfo testWeatherDownButtonInitInfo;
	testWeatherDownButtonInitInfo.callback = [&state, updateWeatherTextBox](MouseButtonType)
	{
		MainMenuUiController::onTestWeatherDownButtonSelected(state.testType, &state.testWeather);
		updateWeatherTextBox();
	};

	state.testWeatherDownButtonElementInstID = uiManager.createButton(testWeatherDownButtonElementInitInfo, testWeatherDownButtonInitInfo, contextType, state.elements);*/

	/*auto loadGameInputActionCallback = [&uiManager, &state](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			uiManager.getButtonCallback(state.loadGameButtonElementInstID)(MouseButtonType::Left);
		}
	};

	auto startGameInputActionCallback = [&uiManager, &state](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			uiManager.getButtonCallback(state.newGameButtonElementInstID)(MouseButtonType::Left);
		}
	};

	auto exitGameInputActionCallback = [&uiManager, &state](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			uiManager.getButtonCallback(state.exitGameButtonElementInstID)(MouseButtonType::Left);
		}
	};

	auto testGameInputActionCallback = [&uiManager, &state](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			uiManager.getButtonCallback(state.testGameButtonElementInstID)(MouseButtonType::Left);
		}
	};

	InputManager &inputManager = game.inputManager;
	uiManager.addInputActionListener(InputActionName::LoadGame, loadGameInputActionCallback, inputManager, state.inputListeners);
	uiManager.addInputActionListener(InputActionName::StartNewGame, startGameInputActionCallback, inputManager, state.inputListeners);
	uiManager.addInputActionListener(InputActionName::ExitGame, exitGameInputActionCallback, inputManager, state.inputListeners);
	uiManager.addInputActionListener(InputActionName::TestGame, testGameInputActionCallback, inputManager, state.inputListeners);*/
}

void MainMenuUI::destroy(Game &game)
{
	MainMenuUiState &state = MainMenuUI::state;
	state.free(game.uiManager, game.inputManager, game.renderer);

	state.testType = -1;
	state.testIndex = -1;
	state.testIndex2 = -1;
	state.testWeather = -1;
}

void MainMenuUI::update(double dt, Game &game)
{
	UiManager &uiManager = game.uiManager;
	const MainMenuUiState &state = MainMenuUI::state;

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
