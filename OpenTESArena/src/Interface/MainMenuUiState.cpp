#include "MainMenuUiController.h"
#include "MainMenuUiModel.h"
#include "MainMenuUiState.h"
#include "MainMenuUiView.h"
#include "../Game/Game.h"
#include "../Rendering/Renderer.h"
#include "../UI/ArenaFontName.h"
#include "../UI/UiButton.h"
#include "../UI/UiTextBox.h"

MainMenuUiState::MainMenuUiState()
{
	this->bgTextureID = -1;
	this->testArrowsTextureID = -1;
	this->testButtonTextureID = -1;
	this->testType = -1;
	this->testIndex = -1;
	this->testIndex2 = -1;
	this->testWeather = -1;
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
	uiManager.createImage(testButtonImageElementInitInfo, state.testButtonTextureID, state.elements);

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

	const Rect testTypeUpRect = MainMenuUiView::getTestTypeUpButtonRect();
	const Rect testIndexUpRect = MainMenuUiView::getTestIndexUpButtonRect();
	const Rect testIndex2UpRect = MainMenuUiView::getTestIndex2UpButtonRect();
	const Rect testWeatherUpRect = MainMenuUiView::getTestWeatherUpButtonRect();

	UiElementInitInfo testTypeArrowImageElementInitInfo;
	testTypeArrowImageElementInitInfo.position = testTypeUpRect.getTopLeft();
	testTypeArrowImageElementInitInfo.contextType = UiContextType::MainMenu;
	testTypeArrowImageElementInitInfo.drawOrder = 2;
	uiManager.createImage(testTypeArrowImageElementInitInfo, state.testArrowsTextureID, state.elements);

	UiElementInitInfo testIndexArrowImageElementInitInfo;
	testIndexArrowImageElementInitInfo.position = testIndexUpRect.getTopLeft();
	testIndexArrowImageElementInitInfo.contextType = UiContextType::MainMenu;
	testIndexArrowImageElementInitInfo.drawOrder = 2;
	uiManager.createImage(testIndexArrowImageElementInitInfo, state.testArrowsTextureID, state.elements);

	UiElementInitInfo testIndex2ArrowImageElementInitInfo;
	testIndex2ArrowImageElementInitInfo.position = testIndex2UpRect.getTopLeft();
	testIndex2ArrowImageElementInitInfo.contextType = UiContextType::MainMenu;
	testIndex2ArrowImageElementInitInfo.drawOrder = 2;
	uiManager.createImage(testIndex2ArrowImageElementInitInfo, state.testArrowsTextureID, state.elements);

	UiElementInitInfo testWeatherArrowImageElementInitInfo;
	testWeatherArrowImageElementInitInfo.position = testWeatherUpRect.getTopLeft();
	testWeatherArrowImageElementInitInfo.contextType = UiContextType::MainMenu;
	testWeatherArrowImageElementInitInfo.drawOrder = 2;
	uiManager.createImage(testWeatherArrowImageElementInitInfo, state.testArrowsTextureID, state.elements);

	UiElementInitInfo testTypeTextBoxElementInitInfo;
	testTypeTextBoxElementInitInfo.position = testTypeUpRect.getBottomLeft() - Int2(2, 0);
	testTypeTextBoxElementInitInfo.pivotType = UiPivotType::MiddleRight;
	testTypeTextBoxElementInitInfo.contextType = UiContextType::MainMenu;
	testTypeTextBoxElementInitInfo.drawOrder = 3;

	UiTextBoxInitInfo testTypeTextBoxInitInfo;
	testTypeTextBoxInitInfo.worstCaseText = TextRenderUtils::makeWorstCaseText(15);
	testTypeTextBoxInitInfo.text = "Test type: " + MainMenuUiModel::getTestTypeName(state.testType);
	testTypeTextBoxInitInfo.fontName = MainMenuUiView::TestButtonFontName.c_str();
	testTypeTextBoxInitInfo.defaultColor = MainMenuUiView::getTestButtonTextColor();
	testTypeTextBoxInitInfo.alignment = TextAlignment::MiddleRight;
	uiManager.createTextBox(testTypeTextBoxElementInitInfo, testTypeTextBoxInitInfo, state.elements, renderer);

	UiElementInitInfo testNameTextBoxElementInitInfo;
	testNameTextBoxElementInitInfo.position = testIndexUpRect.getBottomLeft() - Int2(2, 0);
	testNameTextBoxElementInitInfo.pivotType = UiPivotType::MiddleRight;
	testNameTextBoxElementInitInfo.contextType = UiContextType::MainMenu;
	testNameTextBoxElementInitInfo.drawOrder = 3;

	UiTextBoxInitInfo testNameTextBoxInitInfo;
	testNameTextBoxInitInfo.worstCaseText = TextRenderUtils::makeWorstCaseText(15);
	testNameTextBoxInitInfo.text = "Test location: " + MainMenuUiModel::getSelectedTestName(game, state.testType, state.testIndex, state.testIndex2);
	testNameTextBoxInitInfo.fontName = MainMenuUiView::TestButtonFontName.c_str();
	testNameTextBoxInitInfo.defaultColor = MainMenuUiView::getTestButtonTextColor();
	testNameTextBoxInitInfo.alignment = TextAlignment::MiddleRight;
	uiManager.createTextBox(testNameTextBoxElementInitInfo, testNameTextBoxInitInfo, state.elements, renderer);

	UiElementInitInfo testWeatherTextBoxElementInitInfo;
	testWeatherTextBoxElementInitInfo.position = testWeatherUpRect.getBottomLeft() - Int2(2, 0);
	testWeatherTextBoxElementInitInfo.pivotType = UiPivotType::MiddleRight;
	testWeatherTextBoxElementInitInfo.contextType = UiContextType::MainMenu;
	testWeatherTextBoxElementInitInfo.drawOrder = 3;

	const ArenaWeatherType testWeatherType = MainMenuUiModel::getSelectedTestWeatherType(state.testWeather);

	UiTextBoxInitInfo testWeatherTextBoxInitInfo;
	testWeatherTextBoxInitInfo.worstCaseText = TextRenderUtils::makeWorstCaseText(16);
	testWeatherTextBoxInitInfo.text = "Test weather: " + MainMenuUiModel::WeatherTypeNames.at(testWeatherType);
	testWeatherTextBoxInitInfo.fontName = MainMenuUiView::TestButtonFontName.c_str();
	testWeatherTextBoxInitInfo.defaultColor = MainMenuUiView::getTestButtonTextColor();
	testWeatherTextBoxInitInfo.alignment = TextAlignment::MiddleRight;
	uiManager.createTextBox(testWeatherTextBoxElementInitInfo, testWeatherTextBoxInitInfo, state.elements, renderer);

	const Rect loadButtonRect = MainMenuUiView::getLoadButtonRect();
	const Rect newGameButtonRect = MainMenuUiView::getNewGameButtonRect();
	const Rect exitButtonRect = MainMenuUiView::getExitButtonRect();

	UiElementInitInfo loadButtonElementInitInfo;
	loadButtonElementInitInfo.position = loadButtonRect.getTopLeft();
	loadButtonElementInitInfo.size = loadButtonRect.getSize();
	loadButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	loadButtonElementInitInfo.contextType = UiContextType::MainMenu;

	UiButtonInitInfo loadButtonInitInfo;
	loadButtonInitInfo.callback = [&game](MouseButtonType) { MainMenuUiController::onLoadGameButtonSelected(game); };
	uiManager.createButton(loadButtonElementInitInfo, loadButtonInitInfo, state.elements);

	UiElementInitInfo newGameButtonElementInitInfo;
	newGameButtonElementInitInfo.position = newGameButtonRect.getTopLeft();
	newGameButtonElementInitInfo.size = newGameButtonRect.getSize();
	newGameButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	newGameButtonElementInitInfo.contextType = UiContextType::MainMenu;

	UiButtonInitInfo newGameButtonInitInfo;
	newGameButtonInitInfo.callback = [&game](MouseButtonType) { MainMenuUiController::onNewGameButtonSelected(game); };
	uiManager.createButton(newGameButtonElementInitInfo, newGameButtonInitInfo, state.elements);

	UiElementInitInfo exitButtonElementInitInfo;
	exitButtonElementInitInfo.position = exitButtonRect.getTopLeft();
	exitButtonElementInitInfo.size = exitButtonRect.getSize();
	exitButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	exitButtonElementInitInfo.contextType = UiContextType::MainMenu;

	UiButtonInitInfo exitButtonInitInfo;
	exitButtonInitInfo.callback = [](MouseButtonType) { MainMenuUiController::onExitGameButtonSelected(); };
	uiManager.createButton(exitButtonElementInitInfo, exitButtonInitInfo, state.elements);

	// @todo implement test buttons, update text boxes on test clicks
	// @todo hook up UiButton to input manager, need to know if left or right click for some things (automap/world map)
	// @todo comment out old Buttons in MainMenuPanel

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
}
