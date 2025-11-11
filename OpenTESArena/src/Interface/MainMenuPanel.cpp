#include "CommonUiView.h"
#include "MainMenuPanel.h"
#include "MainMenuUiController.h"
#include "MainMenuUiModel.h"
#include "MainMenuUiState.h"
#include "MainMenuUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../UI/ArenaFontName.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextBox.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

MainMenuPanel::MainMenuPanel(Game &game)
	: Panel(game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.beginContext(MainMenuUI::ContextType, game);
}

MainMenuPanel::~MainMenuPanel()
{
	Game &game = this->getGame();
	InputManager &inputManager = game.inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::MainMenu, false);

	// @todo this causes an error when exiting application because UiManager is destructed before MainMenuPanel
	UiManager &uiManager = game.uiManager;
	uiManager.endContext(MainMenuUI::ContextType, game);
}

bool MainMenuPanel::init()
{
	Game &game = this->getGame();
	auto &inputManager = game.inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::MainMenu, true);

	this->loadButton = Button<Game&>(MainMenuUiView::getLoadButtonRect(), MainMenuUiController::onLoadGameButtonSelected);
	this->newButton = Button<Game&>(MainMenuUiView::getNewGameButtonRect(), MainMenuUiController::onNewGameButtonSelected);
	this->exitButton = Button<>(MainMenuUiView::getExitButtonRect(), MainMenuUiController::onExitGameButtonSelected);
	this->quickStartButton = Button<Game&, int, int, const std::string&, const std::optional<ArenaInteriorType>&, ArenaWeatherType, MapType>(
		MainMenuUiView::getTestButtonRect(), MainMenuUiController::onQuickStartButtonSelected);
	this->testTypeUpButton = Button<int*, int*, int*, int*>(
		MainMenuUiView::getTestTypeUpButtonRect(), MainMenuUiController::onTestTypeUpButtonSelected);
	this->testTypeDownButton = Button<int*, int*, int*, int*>(
		MainMenuUiView::getTestTypeDownButtonRect(), MainMenuUiController::onTestTypeDownButtonSelected);
	this->testIndexUpButton = Button<int*, int*, int*>(
		MainMenuUiView::getTestIndexUpButtonRect(), MainMenuUiController::onTestIndexUpButtonSelected);
	this->testIndexDownButton = Button<int*, int*, int*>(
		MainMenuUiView::getTestIndexDownButtonRect(), MainMenuUiController::onTestIndexDownButtonSelected);
	this->testIndex2UpButton = Button<int, int, int*>(
		MainMenuUiView::getTestIndex2UpButtonRect(), MainMenuUiController::onTestIndex2UpButtonSelected);
	this->testIndex2DownButton = Button<int, int, int*>(
		MainMenuUiView::getTestIndex2DownButtonRect(), MainMenuUiController::onTestIndex2DownButtonSelected);
	this->testWeatherUpButton = Button<int, int*>(
		MainMenuUiView::getTestWeatherUpButtonRect(), MainMenuUiController::onTestWeatherUpButtonSelected);
	this->testWeatherDownButton = Button<int, int*>(
		MainMenuUiView::getTestWeatherDownButtonRect(), MainMenuUiController::onTestWeatherDownButtonSelected);

	this->addButtonProxy(MouseButtonType::Left, this->loadButton.getRect(),
		[this, &game]() { this->loadButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->newButton.getRect(),
		[this, &game]() { this->newButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->exitButton.getRect(),
		[this]() { this->exitButton.click(); });
	this->addButtonProxy(MouseButtonType::Left, this->quickStartButton.getRect(),
		[this, &game]()
	{
		this->quickStartButton.click(game,
			this->testType,
			this->testIndex,
			MainMenuUiModel::getSelectedTestName(game, this->testType, this->testIndex, this->testIndex2),
			MainMenuUiModel::getSelectedTestInteriorType(this->testType, this->testIndex),
			MainMenuUiModel::getSelectedTestWeatherType(this->testWeather),
			MainMenuUiModel::getSelectedTestMapType(this->testType));
	});

	auto updateTypeTextBox = [this]()
	{
		const std::string text = "Test type: " + MainMenuUiModel::getTestTypeName(this->testType);
		this->testTypeTextBox.setText(text);
	};

	auto updateNameTextBox = [this, &game]()
	{
		const std::string text = "Test location: " +
			MainMenuUiModel::getSelectedTestName(game, this->testType, this->testIndex, this->testIndex2);
		this->testNameTextBox.setText(text);
	};

	auto updateWeatherTextBox = [this]()
	{
		const ArenaWeatherType testWeatherType = MainMenuUiModel::getSelectedTestWeatherType(this->testWeather);
		const std::string text = "Test weather: " + MainMenuUiModel::WeatherTypeNames.at(testWeatherType);
		this->testWeatherTextBox.setText(text);
	};

	this->addButtonProxy(MouseButtonType::Left, this->testTypeUpButton.getRect(),
		[this, updateTypeTextBox, updateNameTextBox]()
	{
		this->testTypeUpButton.click(&this->testType, &this->testIndex, &this->testIndex2, &this->testWeather);
		updateTypeTextBox();
		updateNameTextBox();
	});

	this->addButtonProxy(MouseButtonType::Left, this->testTypeDownButton.getRect(),
		[this, updateTypeTextBox, updateNameTextBox]()
	{
		this->testTypeDownButton.click(&this->testType, &this->testIndex, &this->testIndex2, &this->testWeather);
		updateTypeTextBox();
		updateNameTextBox();
	});

	this->addButtonProxy(MouseButtonType::Left, this->testIndexUpButton.getRect(),
		[this, updateNameTextBox]()
	{
		this->testIndexUpButton.click(&this->testType, &this->testIndex, &this->testIndex2);
		updateNameTextBox();
	});

	this->addButtonProxy(MouseButtonType::Left, this->testIndexDownButton.getRect(),
		[this, updateNameTextBox]()
	{
		this->testIndexDownButton.click(&this->testType, &this->testIndex, &this->testIndex2);
		updateNameTextBox();
	});

	this->addButtonProxy(MouseButtonType::Left, this->testIndex2UpButton.getRect(),
		[this, updateNameTextBox]()
	{
		if (this->testType == MainMenuUiModel::TestType_Interior)
		{
			this->testIndex2UpButton.click(this->testType, this->testIndex, &this->testIndex2);
			updateNameTextBox();
		}
	});

	this->addButtonProxy(MouseButtonType::Left, this->testIndex2DownButton.getRect(),
		[this, updateNameTextBox]()
	{
		if (this->testType == MainMenuUiModel::TestType_Interior)
		{
			this->testIndex2DownButton.click(this->testType, this->testIndex, &this->testIndex2);
			updateNameTextBox();
		}
	});

	this->addButtonProxy(MouseButtonType::Left, this->testWeatherUpButton.getRect(),
		[this, updateWeatherTextBox]()
	{
		if ((this->testType == MainMenuUiModel::TestType_City) ||
			(this->testType == MainMenuUiModel::TestType_Wilderness))
		{
			this->testWeatherUpButton.click(this->testType, &this->testWeather);
			updateWeatherTextBox();
		}
	});

	this->addButtonProxy(MouseButtonType::Left, this->testWeatherDownButton.getRect(),
		[this, updateWeatherTextBox]()
	{
		if ((this->testType == MainMenuUiModel::TestType_City) ||
			(this->testType == MainMenuUiModel::TestType_Wilderness))
		{
			this->testWeatherDownButton.click(this->testType, &this->testWeather);
			updateWeatherTextBox();
		}
	});

	this->addInputActionListener(InputActionName::LoadGame,
		[this, &game](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->loadButton.click(game);
		}
	});

	this->addInputActionListener(InputActionName::StartNewGame,
		[this, &game](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->newButton.click(game);
		}
	});

	this->addInputActionListener(InputActionName::ExitGame,
		[this](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->exitButton.click();
		}
	});

	this->addInputActionListener(InputActionName::TestGame,
		[this, &game](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->quickStartButton.click(game,
				this->testType,
				this->testIndex,
				MainMenuUiModel::getSelectedTestName(game, this->testType, this->testIndex, this->testIndex2),
				MainMenuUiModel::getSelectedTestInteriorType(this->testType, this->testIndex),
				MainMenuUiModel::getSelectedTestWeatherType(this->testWeather),
				MainMenuUiModel::getSelectedTestMapType(this->testType));
		}
	});

	this->testType = 0;
	this->testIndex = 0;
	this->testIndex2 = 1;
	this->testWeather = 0;

	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	this->initTestUI();

	// Unload in case we are returning from a game session.
	SceneManager &sceneManager = game.sceneManager;
	sceneManager.renderVoxelChunkManager.unloadScene(renderer);
	sceneManager.renderEntityManager.unloadScene(renderer);

	return true;
}

void MainMenuPanel::initTestUI()
{
	auto &game = this->getGame();
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;
	const UiTextureID testArrowsTextureID = MainMenuUiView::allocTestArrowsTexture(textureManager, renderer);
	this->testArrowsTextureRef.init(testArrowsTextureID, renderer);

	const Rect testTypeUpRect = MainMenuUiView::getTestTypeUpButtonRect();
	const Rect testIndexUpRect = MainMenuUiView::getTestIndexUpButtonRect();
	const Rect testIndex2UpRect = MainMenuUiView::getTestIndex2UpButtonRect();
	const Rect testWeatherUpRect = MainMenuUiView::getTestWeatherUpButtonRect();

	const UiTextureID testArrowTextureID = this->testArrowsTextureRef.get();
	const Int2 testArrowSize = this->testArrowsTextureRef.getDimensions();

	UiDrawCallInitInfo testTypeArrowDrawCallInitInfo;
	testTypeArrowDrawCallInitInfo.textureID = testArrowTextureID;
	testTypeArrowDrawCallInitInfo.position = testTypeUpRect.getTopLeft();
	testTypeArrowDrawCallInitInfo.size = testArrowSize;
	//this->addDrawCall(testTypeArrowDrawCallInitInfo);

	UiDrawCallInitInfo testIndexArrowDrawCallInitInfo;
	testIndexArrowDrawCallInitInfo.textureID = testArrowTextureID;
	testIndexArrowDrawCallInitInfo.position = testIndexUpRect.getTopLeft();
	testIndexArrowDrawCallInitInfo.size = testArrowSize;
	//this->addDrawCall(testIndexArrowDrawCallInitInfo);

	UiDrawCallInitInfo testIndex2ArrowDrawCallInitInfo;
	testIndex2ArrowDrawCallInitInfo.textureID = testArrowTextureID;
	testIndex2ArrowDrawCallInitInfo.position = testIndex2UpRect.getTopLeft();
	testIndex2ArrowDrawCallInitInfo.size = testArrowSize;
	testIndex2ArrowDrawCallInitInfo.activeFunc = [this]() { return this->testType == MainMenuUiModel::TestType_Interior; };
	//this->addDrawCall(testIndex2ArrowDrawCallInitInfo);

	UiDrawCallInitInfo testWeatherArrowDrawCallInitInfo;
	testWeatherArrowDrawCallInitInfo.textureID = testArrowTextureID;
	testWeatherArrowDrawCallInitInfo.position = testWeatherUpRect.getTopLeft();
	testWeatherArrowDrawCallInitInfo.size = testArrowSize;
	testWeatherArrowDrawCallInitInfo.activeFunc = [this]() { return (this->testType == MainMenuUiModel::TestType_City) || (this->testType == MainMenuUiModel::TestType_Wilderness); };
	//this->addDrawCall(testWeatherArrowDrawCallInitInfo);

	const UiTextureID testButtonTextureID = MainMenuUiView::allocTestButtonTexture(textureManager, renderer);
	this->testButtonTextureRef.init(testButtonTextureID, renderer);

	const Rect testButtonRect = MainMenuUiView::getTestButtonRect();
	UiDrawCallInitInfo testButtonTextureDrawCallInitInfo;
	testButtonTextureDrawCallInitInfo.textureID = this->testButtonTextureRef.get();
	testButtonTextureDrawCallInitInfo.position = testButtonRect.getTopLeft();
	testButtonTextureDrawCallInitInfo.size = testButtonRect.getSize();
	//this->addDrawCall(testButtonTextureDrawCallInitInfo);

	const FontLibrary &fontLibrary = FontLibrary::getInstance();
	const std::string testButtonText = MainMenuUiModel::getTestButtonText();
	const TextBoxInitInfo testButtonInitInfo = MainMenuUiView::getTestButtonTextBoxInitInfo(testButtonText, fontLibrary);
	if (!this->testButtonTextBox.init(testButtonInitInfo, testButtonText, renderer))
	{
		DebugCrash("Couldn't init test button text box.");
	}

	const Rect testButtonTextBoxRect = this->testButtonTextBox.getRect();
	UiDrawCallInitInfo testButtonTextDrawCallInitInfo;
	testButtonTextDrawCallInitInfo.textureID = this->testButtonTextBox.getTextureID();
	testButtonTextDrawCallInitInfo.position = testButtonTextBoxRect.getCenter();
	testButtonTextDrawCallInitInfo.size = testButtonTextBoxRect.getSize();
	testButtonTextDrawCallInitInfo.pivotType = UiPivotType::Middle;
	//this->addDrawCall(testButtonTextDrawCallInitInfo);

	const std::string testTypeText = "Test type: " + MainMenuUiModel::getTestTypeName(this->testType);
	const TextBoxInitInfo testTypeInitInfo = MainMenuUiView::getTestTypeTextBoxInitInfo(fontLibrary);
	if (!this->testTypeTextBox.init(testTypeInitInfo, testTypeText, renderer))
	{
		DebugCrash("Couldn't init test type text box.");
	}

	const Rect testTypeTextBoxRect = this->testTypeTextBox.getRect();
	UiDrawCallInitInfo testTypeTextDrawCallInitInfo;
	testTypeTextDrawCallInitInfo.textureFunc = [this]() { return this->testTypeTextBox.getTextureID(); };
	testTypeTextDrawCallInitInfo.position = Int2(testTypeTextBoxRect.getRight(), testTypeTextBoxRect.getTop());
	testTypeTextDrawCallInitInfo.size = testTypeTextBoxRect.getSize();
	testTypeTextDrawCallInitInfo.pivotType = UiPivotType::MiddleRight;
	//this->addDrawCall(testTypeTextDrawCallInitInfo);

	const std::string testNameText = "Test location: " + MainMenuUiModel::getSelectedTestName(game, this->testType, this->testIndex, this->testIndex2);
	const TextBoxInitInfo testNameInitInfo = MainMenuUiView::getTestNameTextBoxInitInfo(fontLibrary);
	if (!this->testNameTextBox.init(testNameInitInfo, testNameText, renderer))
	{
		DebugCrash("Couldn't init test name text box.");
	}

	const Rect testNameTextBoxRect = this->testNameTextBox.getRect();
	UiDrawCallInitInfo testNameTextDrawCallInitInfo;
	testNameTextDrawCallInitInfo.textureFunc = [this]() { return this->testNameTextBox.getTextureID(); };
	testNameTextDrawCallInitInfo.position = Int2(testNameTextBoxRect.getRight(), testNameTextBoxRect.getTop());
	testNameTextDrawCallInitInfo.size = testNameTextBoxRect.getSize();
	testNameTextDrawCallInitInfo.pivotType = UiPivotType::MiddleRight;
	//this->addDrawCall(testNameTextDrawCallInitInfo);

	const ArenaWeatherType testWeatherType = MainMenuUiModel::getSelectedTestWeatherType(this->testWeather);
	const std::string testWeatherText = "Test weather: " + MainMenuUiModel::WeatherTypeNames.at(testWeatherType);
	const TextBoxInitInfo testWeatherInitInfo = MainMenuUiView::getTestWeatherTextBoxInitInfo(fontLibrary);
	if (!this->testWeatherTextBox.init(testWeatherInitInfo, testWeatherText, renderer))
	{
		DebugCrash("Couldn't init test weather text box.");
	}

	const Rect testWeatherTextBoxRect = this->testWeatherTextBox.getRect();
	UiDrawCallInitInfo testWeatherTextDrawCallInitInfo;
	testWeatherTextDrawCallInitInfo.textureFunc = [this]() { return this->testWeatherTextBox.getTextureID(); };
	testWeatherTextDrawCallInitInfo.position = Int2(testWeatherTextBoxRect.getRight(), testWeatherTextBoxRect.getTop());
	testWeatherTextDrawCallInitInfo.size = testWeatherTextBoxRect.getSize();
	testWeatherTextDrawCallInitInfo.pivotType = UiPivotType::MiddleRight;
	testWeatherTextDrawCallInitInfo.activeFunc = testWeatherArrowDrawCallInitInfo.activeFunc;
	//this->addDrawCall(testWeatherTextDrawCallInitInfo);
}
