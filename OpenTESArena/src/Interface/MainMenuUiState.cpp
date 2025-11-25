#include "MainMenuUiController.h"
#include "MainMenuUiModel.h"
#include "MainMenuUiState.h"
#include "MainMenuUiView.h"
#include "../Game/Game.h"
#include "../Rendering/Renderer.h"
#include "../UI/ArenaFontName.h"
#include "../UI/UiButton.h"
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
	this->bgTextureID = -1;
	this->testArrowsTextureID = -1;
	this->testButtonTextureID = -1;
	this->testType = -1;
	this->testIndex = -1;
	this->testIndex2 = -1;
	this->testWeather = -1;
	this->testIndex2ImageElementInstID = -1;
	this->testIndex2UpButtonElementInstID = -1;
	this->testIndex2DownButtonElementInstID = -1;
	this->testWeatherImageElementInstID = -1;
	this->testWeatherTextBoxElementInstID = -1;
	this->testWeatherUpButtonElementInstID = -1;
	this->testWeatherDownButtonElementInstID = -1;
}

void MainMenuUiState::allocate(UiManager &uiManager, TextureManager &textureManager, Renderer &renderer)
{
	this->bgTextureID = MainMenuUiView::allocBackgroundTexture(textureManager, renderer);
	this->testArrowsTextureID = MainMenuUiView::allocTestArrowsTexture(textureManager, renderer);
	this->testButtonTextureID = MainMenuUiView::allocTestButtonTexture(textureManager, renderer);
}

void MainMenuUiState::free(UiManager &uiManager, Renderer &renderer)
{
	this->elements.free(uiManager, renderer);

	if (this->bgTextureID >= 0)
	{
		renderer.freeUiTexture(this->bgTextureID);
		this->bgTextureID = -1;
	}

	if (this->testArrowsTextureID >= 0)
	{
		renderer.freeUiTexture(this->testArrowsTextureID);
		this->testArrowsTextureID = -1;
	}

	if (this->testButtonTextureID >= 0)
	{
		renderer.freeUiTexture(this->testButtonTextureID);
		this->testButtonTextureID = -1;
	}
}

void MainMenuUI::create(Game &game)
{
	Renderer &renderer = game.renderer;
	UiManager &uiManager = game.uiManager;
	MainMenuUiState &state = MainMenuUI::state;
	state.allocate(uiManager, game.textureManager, renderer);

	state.testType = 0;
	state.testIndex = 0;
	state.testIndex2 = 1;
	state.testWeather = 0;

	UiElementInitInfo bgImageElementInitInfo;
	bgImageElementInitInfo.contextType = UiContextType::MainMenu;
	uiManager.createImage(bgImageElementInitInfo, state.bgTextureID, state.elements);

	const Rect testButtonRect = MainMenuUiView::getTestButtonRect();

	UiElementInitInfo testButtonImageElementInitInfo;
	testButtonImageElementInitInfo.position = testButtonRect.getCenter();
	testButtonImageElementInitInfo.pivotType = UiPivotType::Middle;
	testButtonImageElementInitInfo.contextType = UiContextType::MainMenu;
	testButtonImageElementInitInfo.drawOrder = 1;
	const UiElementInstanceID testButtonImageElementInstID = uiManager.createImage(testButtonImageElementInitInfo, state.testButtonTextureID, state.elements);

	UiElementInitInfo testButtonTextBoxElementInitInfo;
	testButtonTextBoxElementInitInfo.position = testButtonRect.getCenter();
	testButtonTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	testButtonTextBoxElementInitInfo.contextType = UiContextType::MainMenu;
	testButtonTextBoxElementInitInfo.drawOrder = 2;

	UiTextBoxInitInfo testButtonTextBoxInitInfo;
	testButtonTextBoxInitInfo.worstCaseText = std::string(5, TextRenderUtils::LARGEST_CHAR);
	testButtonTextBoxInitInfo.text = "Test";
	testButtonTextBoxInitInfo.fontName = MainMenuUiView::TestButtonFontName.c_str();
	testButtonTextBoxInitInfo.defaultColor = MainMenuUiView::getTestButtonTextColor();
	testButtonTextBoxInitInfo.alignment = MainMenuUiView::TestButtonTextAlignment;
	uiManager.createTextBox(testButtonTextBoxElementInitInfo, testButtonTextBoxInitInfo, state.elements, renderer);

	const Rect testTypeUpButtonRect = MainMenuUiView::getTestTypeUpButtonRect();
	const Rect testTypeDownButtonRect = MainMenuUiView::getTestTypeDownButtonRect();
	const Rect testIndexUpButtonRect = MainMenuUiView::getTestIndexUpButtonRect();
	const Rect testIndexDownButtonRect = MainMenuUiView::getTestIndexDownButtonRect();
	const Rect testIndex2UpButtonRect = MainMenuUiView::getTestIndex2UpButtonRect();
	const Rect testIndex2DownButtonRect = MainMenuUiView::getTestIndex2DownButtonRect();
	const Rect testWeatherUpButtonRect = MainMenuUiView::getTestWeatherUpButtonRect();
	const Rect testWeatherDownButtonRect = MainMenuUiView::getTestWeatherDownButtonRect();

	UiElementInitInfo testTypeArrowImageElementInitInfo;
	testTypeArrowImageElementInitInfo.position = testTypeUpButtonRect.getTopLeft();
	testTypeArrowImageElementInitInfo.contextType = UiContextType::MainMenu;
	testTypeArrowImageElementInitInfo.drawOrder = 2;
	uiManager.createImage(testTypeArrowImageElementInitInfo, state.testArrowsTextureID, state.elements);

	UiElementInitInfo testIndexArrowImageElementInitInfo;
	testIndexArrowImageElementInitInfo.position = testIndexUpButtonRect.getTopLeft();
	testIndexArrowImageElementInitInfo.contextType = UiContextType::MainMenu;
	testIndexArrowImageElementInitInfo.drawOrder = 2;
	uiManager.createImage(testIndexArrowImageElementInitInfo, state.testArrowsTextureID, state.elements);

	UiElementInitInfo testIndex2ArrowImageElementInitInfo;
	testIndex2ArrowImageElementInitInfo.position = testIndex2UpButtonRect.getTopLeft();
	testIndex2ArrowImageElementInitInfo.contextType = UiContextType::MainMenu;
	testIndex2ArrowImageElementInitInfo.drawOrder = 2;
	state.testIndex2ImageElementInstID = uiManager.createImage(testIndex2ArrowImageElementInitInfo, state.testArrowsTextureID, state.elements);

	UiElementInitInfo testWeatherArrowImageElementInitInfo;
	testWeatherArrowImageElementInitInfo.position = testWeatherUpButtonRect.getTopLeft();
	testWeatherArrowImageElementInitInfo.contextType = UiContextType::MainMenu;
	testWeatherArrowImageElementInitInfo.drawOrder = 2;
	state.testWeatherImageElementInstID = uiManager.createImage(testWeatherArrowImageElementInitInfo, state.testArrowsTextureID, state.elements);

	UiElementInitInfo testTypeTextBoxElementInitInfo;
	testTypeTextBoxElementInitInfo.position = testTypeUpButtonRect.getBottomLeft() - Int2(2, 0);
	testTypeTextBoxElementInitInfo.pivotType = UiPivotType::MiddleRight;
	testTypeTextBoxElementInitInfo.contextType = UiContextType::MainMenu;
	testTypeTextBoxElementInitInfo.drawOrder = 3;

	UiTextBoxInitInfo testTypeTextBoxInitInfo;
	testTypeTextBoxInitInfo.worstCaseText = TextRenderUtils::makeWorstCaseText(15);
	testTypeTextBoxInitInfo.text = MakeTestTypeText(state.testType);
	testTypeTextBoxInitInfo.fontName = MainMenuUiView::TestButtonFontName.c_str();
	testTypeTextBoxInitInfo.defaultColor = MainMenuUiView::getTestButtonTextColor();
	testTypeTextBoxInitInfo.alignment = TextAlignment::MiddleRight;
	const UiElementInstanceID testTypeTextBoxElementInstID = uiManager.createTextBox(testTypeTextBoxElementInitInfo, testTypeTextBoxInitInfo, state.elements, renderer);

	UiElementInitInfo testNameTextBoxElementInitInfo;
	testNameTextBoxElementInitInfo.position = testIndexUpButtonRect.getBottomLeft() - Int2(2, 0);
	testNameTextBoxElementInitInfo.pivotType = UiPivotType::MiddleRight;
	testNameTextBoxElementInitInfo.contextType = UiContextType::MainMenu;
	testNameTextBoxElementInitInfo.drawOrder = 3;

	UiTextBoxInitInfo testNameTextBoxInitInfo;
	testNameTextBoxInitInfo.worstCaseText = TextRenderUtils::makeWorstCaseText(15);
	testNameTextBoxInitInfo.text = MakeTestLocationText(game, state.testType, state.testIndex, state.testIndex2);
	testNameTextBoxInitInfo.fontName = MainMenuUiView::TestButtonFontName.c_str();
	testNameTextBoxInitInfo.defaultColor = MainMenuUiView::getTestButtonTextColor();
	testNameTextBoxInitInfo.alignment = TextAlignment::MiddleRight;
	const UiElementInstanceID testNameTextBoxElementInstID = uiManager.createTextBox(testNameTextBoxElementInitInfo, testNameTextBoxInitInfo, state.elements, renderer);

	UiElementInitInfo testWeatherTextBoxElementInitInfo;
	testWeatherTextBoxElementInitInfo.position = testWeatherUpButtonRect.getBottomLeft() - Int2(2, 0);
	testWeatherTextBoxElementInitInfo.pivotType = UiPivotType::MiddleRight;
	testWeatherTextBoxElementInitInfo.contextType = UiContextType::MainMenu;
	testWeatherTextBoxElementInitInfo.drawOrder = 3;

	UiTextBoxInitInfo testWeatherTextBoxInitInfo;
	testWeatherTextBoxInitInfo.worstCaseText = TextRenderUtils::makeWorstCaseText(16);
	testWeatherTextBoxInitInfo.text = MakeTestWeatherText(state.testWeather);
	testWeatherTextBoxInitInfo.fontName = MainMenuUiView::TestButtonFontName.c_str();
	testWeatherTextBoxInitInfo.defaultColor = MainMenuUiView::getTestButtonTextColor();
	testWeatherTextBoxInitInfo.alignment = TextAlignment::MiddleRight;
	state.testWeatherTextBoxElementInstID = uiManager.createTextBox(testWeatherTextBoxElementInitInfo, testWeatherTextBoxInitInfo, state.elements, renderer);

	const Rect loadButtonRect = MainMenuUiView::getLoadButtonRect();
	const Rect newGameButtonRect = MainMenuUiView::getNewGameButtonRect();
	const Rect exitButtonRect = MainMenuUiView::getExitButtonRect();

	UiElementInitInfo loadButtonElementInitInfo;
	loadButtonElementInitInfo.position = loadButtonRect.getTopLeft();
	loadButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	loadButtonElementInitInfo.size = loadButtonRect.getSize();
	loadButtonElementInitInfo.contextType = UiContextType::MainMenu;

	UiButtonInitInfo loadButtonInitInfo;
	loadButtonInitInfo.callback = [&game](MouseButtonType) { MainMenuUiController::onLoadGameButtonSelected(game); };
	uiManager.createButton(loadButtonElementInitInfo, loadButtonInitInfo, state.elements);

	UiElementInitInfo newGameButtonElementInitInfo;
	newGameButtonElementInitInfo.position = newGameButtonRect.getTopLeft();
	newGameButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	newGameButtonElementInitInfo.size = newGameButtonRect.getSize();
	newGameButtonElementInitInfo.contextType = UiContextType::MainMenu;

	UiButtonInitInfo newGameButtonInitInfo;
	newGameButtonInitInfo.callback = [&game](MouseButtonType) { MainMenuUiController::onNewGameButtonSelected(game); };
	uiManager.createButton(newGameButtonElementInitInfo, newGameButtonInitInfo, state.elements);

	UiElementInitInfo exitButtonElementInitInfo;
	exitButtonElementInitInfo.position = exitButtonRect.getTopLeft();
	exitButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	exitButtonElementInitInfo.size = exitButtonRect.getSize();
	exitButtonElementInitInfo.contextType = UiContextType::MainMenu;

	UiButtonInitInfo exitButtonInitInfo;
	exitButtonInitInfo.callback = [](MouseButtonType) { MainMenuUiController::onExitGameButtonSelected(); };
	uiManager.createButton(exitButtonElementInitInfo, exitButtonInitInfo, state.elements);

	UiElementInitInfo testButtonElementInitInfo;
	testButtonElementInitInfo.position = testButtonRect.getTopLeft();
	testButtonElementInitInfo.contextType = UiContextType::MainMenu;

	UiButtonInitInfo testButtonInitInfo;
	testButtonInitInfo.callback = [&game, &state](MouseButtonType)
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

	testButtonInitInfo.contentElementInstID = testButtonImageElementInstID;
	uiManager.createButton(testButtonElementInitInfo, testButtonInitInfo, state.elements);

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
	testTypeUpButtonElementInitInfo.contextType = UiContextType::MainMenu;

	UiButtonInitInfo testTypeUpButtonInitInfo;
	testTypeUpButtonInitInfo.callback = [&state, updateTypeTextBox, updateNameTextBox](MouseButtonType)
	{
		MainMenuUiController::onTestTypeUpButtonSelected(&state.testType, &state.testIndex, &state.testIndex2, &state.testWeather);
		updateTypeTextBox();
		updateNameTextBox();
	};

	uiManager.createButton(testTypeUpButtonElementInitInfo, testTypeUpButtonInitInfo, state.elements);

	UiElementInitInfo testTypeDownButtonElementInitInfo;
	testTypeDownButtonElementInitInfo.position = testTypeDownButtonRect.getTopLeft();
	testTypeDownButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	testTypeDownButtonElementInitInfo.size = testTypeDownButtonRect.getSize();
	testTypeDownButtonElementInitInfo.contextType = UiContextType::MainMenu;

	UiButtonInitInfo testTypeDownButtonInitInfo;
	testTypeDownButtonInitInfo.callback = [&state, updateTypeTextBox, updateNameTextBox](MouseButtonType)
	{
		MainMenuUiController::onTestTypeDownButtonSelected(&state.testType, &state.testIndex, &state.testIndex2, &state.testWeather);
		updateTypeTextBox();
		updateNameTextBox();
	};

	uiManager.createButton(testTypeDownButtonElementInitInfo, testTypeDownButtonInitInfo, state.elements);

	UiElementInitInfo testIndexUpButtonElementInitInfo;
	testIndexUpButtonElementInitInfo.position = testIndexUpButtonRect.getTopLeft();
	testIndexUpButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	testIndexUpButtonElementInitInfo.size = testIndexUpButtonRect.getSize();
	testIndexUpButtonElementInitInfo.contextType = UiContextType::MainMenu;

	UiButtonInitInfo testIndexUpButtonInitInfo;
	testIndexUpButtonInitInfo.callback = [&state, updateNameTextBox](MouseButtonType)
	{
		MainMenuUiController::onTestIndexUpButtonSelected(&state.testType, &state.testIndex, &state.testIndex2);
		updateNameTextBox();
	};

	uiManager.createButton(testIndexUpButtonElementInitInfo, testIndexUpButtonInitInfo, state.elements);

	UiElementInitInfo testIndexDownButtonElementInitInfo;
	testIndexDownButtonElementInitInfo.position = testIndexDownButtonRect.getTopLeft();
	testIndexDownButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	testIndexDownButtonElementInitInfo.size = testIndexDownButtonRect.getSize();
	testIndexDownButtonElementInitInfo.contextType = UiContextType::MainMenu;

	UiButtonInitInfo testIndexDownButtonInitInfo;
	testIndexDownButtonInitInfo.callback = [&state, updateNameTextBox](MouseButtonType)
	{
		MainMenuUiController::onTestIndexDownButtonSelected(&state.testType, &state.testIndex, &state.testIndex2);
		updateNameTextBox();
	};

	uiManager.createButton(testIndexDownButtonElementInitInfo, testIndexDownButtonInitInfo, state.elements);

	UiElementInitInfo testIndex2UpButtonElementInitInfo;
	testIndex2UpButtonElementInitInfo.position = testIndex2UpButtonRect.getTopLeft();
	testIndex2UpButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	testIndex2UpButtonElementInitInfo.size = testIndex2UpButtonRect.getSize();
	testIndex2UpButtonElementInitInfo.contextType = UiContextType::MainMenu;

	UiButtonInitInfo testIndex2UpButtonInitInfo;
	testIndex2UpButtonInitInfo.callback = [&state, updateNameTextBox](MouseButtonType)
	{
		MainMenuUiController::onTestIndex2UpButtonSelected(state.testType, state.testIndex, &state.testIndex2);
		updateNameTextBox();
	};

	state.testIndex2UpButtonElementInstID = uiManager.createButton(testIndex2UpButtonElementInitInfo, testIndex2UpButtonInitInfo, state.elements);

	UiElementInitInfo testIndex2DownButtonElementInitInfo;
	testIndex2DownButtonElementInitInfo.position = testIndex2DownButtonRect.getTopLeft();
	testIndex2DownButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	testIndex2DownButtonElementInitInfo.size = testIndex2DownButtonRect.getSize();
	testIndex2DownButtonElementInitInfo.contextType = UiContextType::MainMenu;

	UiButtonInitInfo testIndex2DownButtonInitInfo;
	testIndex2DownButtonInitInfo.callback = [&state, updateNameTextBox](MouseButtonType)
	{
		MainMenuUiController::onTestIndex2DownButtonSelected(state.testType, state.testIndex, &state.testIndex2);
		updateNameTextBox();
	};

	state.testIndex2DownButtonElementInstID = uiManager.createButton(testIndex2DownButtonElementInitInfo, testIndex2DownButtonInitInfo, state.elements);

	UiElementInitInfo testWeatherUpButtonElementInitInfo;
	testWeatherUpButtonElementInitInfo.position = testWeatherUpButtonRect.getTopLeft();
	testWeatherUpButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	testWeatherUpButtonElementInitInfo.size = testWeatherUpButtonRect.getSize();
	testWeatherUpButtonElementInitInfo.contextType = UiContextType::MainMenu;

	UiButtonInitInfo testWeatherUpButtonInitInfo;
	testWeatherUpButtonInitInfo.callback = [&state, updateWeatherTextBox](MouseButtonType)
	{
		MainMenuUiController::onTestWeatherUpButtonSelected(state.testType, &state.testWeather);
		updateWeatherTextBox();
	};

	state.testWeatherUpButtonElementInstID = uiManager.createButton(testWeatherUpButtonElementInitInfo, testWeatherUpButtonInitInfo, state.elements);

	UiElementInitInfo testWeatherDownButtonElementInitInfo;
	testWeatherDownButtonElementInitInfo.position = testWeatherDownButtonRect.getTopLeft();
	testWeatherDownButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	testWeatherDownButtonElementInitInfo.size = testWeatherDownButtonRect.getSize();
	testWeatherDownButtonElementInitInfo.contextType = UiContextType::MainMenu;

	UiButtonInitInfo testWeatherDownButtonInitInfo;
	testWeatherDownButtonInitInfo.callback = [&state, updateWeatherTextBox](MouseButtonType)
	{
		MainMenuUiController::onTestWeatherDownButtonSelected(state.testType, &state.testWeather);
		updateWeatherTextBox();
	};

	state.testWeatherDownButtonElementInstID = uiManager.createButton(testWeatherDownButtonElementInitInfo, testWeatherDownButtonInitInfo, state.elements);

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

void MainMenuUI::destroy(Game &game)
{
	MainMenuUiState &state = MainMenuUI::state;
	state.free(game.uiManager, game.renderer);

	state.testType = -1;
	state.testIndex = -1;
	state.testIndex2 = -1;
	state.testWeather = -1;
	state.testIndex2ImageElementInstID = -1;
	state.testIndex2UpButtonElementInstID = -1;
	state.testIndex2DownButtonElementInstID = -1;
	state.testWeatherImageElementInstID = -1;
	state.testWeatherTextBoxElementInstID = -1;
	state.testWeatherUpButtonElementInstID = -1;
	state.testWeatherDownButtonElementInstID = -1;
}

void MainMenuUI::update(double dt, Game &game)
{
	UiManager &uiManager = game.uiManager;
	const MainMenuUiState &state = MainMenuUI::state;

	const bool currentTestIsInterior = state.testType == MainMenuUiModel::TestType_Interior;
	uiManager.setElementActive(state.testIndex2ImageElementInstID, currentTestIsInterior);
	uiManager.setElementActive(state.testIndex2UpButtonElementInstID, currentTestIsInterior);
	uiManager.setElementActive(state.testIndex2DownButtonElementInstID, currentTestIsInterior);

	const bool currentTestHasWeather = (state.testType == MainMenuUiModel::TestType_City) || (state.testType == MainMenuUiModel::TestType_Wilderness);
	uiManager.setElementActive(state.testWeatherImageElementInstID, currentTestHasWeather);
	uiManager.setElementActive(state.testWeatherTextBoxElementInstID, currentTestHasWeather);
	uiManager.setElementActive(state.testWeatherUpButtonElementInstID, currentTestHasWeather);
	uiManager.setElementActive(state.testWeatherDownButtonElementInstID, currentTestHasWeather);
}
