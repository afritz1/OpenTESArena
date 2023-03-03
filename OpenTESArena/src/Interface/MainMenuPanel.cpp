#include "CommonUiView.h"
#include "MainMenuPanel.h"
#include "MainMenuUiController.h"
#include "MainMenuUiModel.h"
#include "MainMenuUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../UI/ArenaFontName.h"
#include "../UI/CursorData.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextBox.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

MainMenuPanel::MainMenuPanel(Game &game)
	: Panel(game) { }

MainMenuPanel::~MainMenuPanel()
{
	auto &inputManager = this->getGame().getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::MainMenu, false);
}

bool MainMenuPanel::init()
{
	Game &game = this->getGame();
	auto &inputManager = game.getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::MainMenu, true);

	this->loadButton = Button<Game&>(MainMenuUiView::getLoadButtonRect(), MainMenuUiController::onLoadGameButtonSelected);
	this->newButton = Button<Game&>(MainMenuUiView::getNewGameButtonRect(), MainMenuUiController::onNewGameButtonSelected);
	this->exitButton = Button<>(MainMenuUiView::getExitButtonRect(), MainMenuUiController::onExitGameButtonSelected);
	this->quickStartButton = Button<Game&, int, int, const std::string&, const std::optional<ArenaTypes::InteriorType>&, ArenaTypes::WeatherType, MapType>(
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
		const ArenaTypes::WeatherType testWeatherType = MainMenuUiModel::getSelectedTestWeatherType(this->testWeather);
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

	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();
	const UiTextureID backgroundTextureID = MainMenuUiView::allocBackgroundTexture(textureManager, renderer);
	this->backgroundTextureRef.init(backgroundTextureID, renderer);
	this->addDrawCall(
		this->backgroundTextureRef.get(),
		Int2::Zero,
		Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT),
		PivotType::TopLeft);
	
	this->initTestUI();
	
	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), PivotType::TopLeft);

	GameState &gameState = game.getGameState();
	if (gameState.hasActiveMapInst())
	{
		DebugLogError("An active map instance should not exist when on the main menu."); // @todo: this will be okay eventually if empty chunks are created in Game::init().
		return false;
	}

	RenderChunkManager &renderChunkManager = game.getRenderChunkManager();
	renderChunkManager.unloadScene(renderer); // In case we are returning from a game session.

	return true;
}

void MainMenuPanel::initTestUI()
{
	auto &game = this->getGame();
	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();
	const UiTextureID testArrowsTextureID = MainMenuUiView::allocTestArrowsTexture(textureManager, renderer);
	this->testArrowsTextureRef.init(testArrowsTextureID, renderer);

	const Rect testTypeUpRect = MainMenuUiView::getTestTypeUpButtonRect();
	const Rect testIndexUpRect = MainMenuUiView::getTestIndexUpButtonRect();
	const Rect testIndex2UpRect = MainMenuUiView::getTestIndex2UpButtonRect();
	const Rect testWeatherUpRect = MainMenuUiView::getTestWeatherUpButtonRect();

	UiDrawCall::TextureFunc testArrowTextureFunc = [this]()
	{
		return this->testArrowsTextureRef.get();
	};

	UiDrawCall::SizeFunc testArrowSizeFunc = [this]()
	{
		return Int2(this->testArrowsTextureRef.getWidth(), this->testArrowsTextureRef.getHeight());
	};

	UiDrawCall::PivotFunc testArrowPivotFunc = []()
	{
		return PivotType::TopLeft;
	};

	UiDrawCall::PositionFunc testTypePositionFunc = [testTypeUpRect]()
	{
		return testTypeUpRect.getTopLeft();
	};

	UiDrawCall::PositionFunc testIndexPositionFunc = [testIndexUpRect]()
	{
		return testIndexUpRect.getTopLeft();
	};

	UiDrawCall::PositionFunc testIndex2PositionFunc = [testIndex2UpRect]()
	{
		return testIndex2UpRect.getTopLeft();
	};

	UiDrawCall::PositionFunc testWeatherPositionFunc = [testWeatherUpRect]()
	{
		return testWeatherUpRect.getTopLeft();
	};

	UiDrawCall::ActiveFunc testIndex2ActiveFunc = [this]()
	{
		return this->testType == MainMenuUiModel::TestType_Interior;
	};

	UiDrawCall::ActiveFunc testWeatherActiveFunc = [this]()
	{
		return (this->testType == MainMenuUiModel::TestType_City) ||
			(this->testType == MainMenuUiModel::TestType_Wilderness);
	};

	this->addDrawCall(
		testArrowTextureFunc,
		testTypePositionFunc,
		testArrowSizeFunc,
		testArrowPivotFunc,
		UiDrawCall::defaultActiveFunc);
	this->addDrawCall(
		testArrowTextureFunc,
		testIndexPositionFunc,
		testArrowSizeFunc,
		testArrowPivotFunc,
		UiDrawCall::defaultActiveFunc);
	this->addDrawCall(
		testArrowTextureFunc,
		testIndex2PositionFunc,
		testArrowSizeFunc,
		testArrowPivotFunc,
		testIndex2ActiveFunc);
	this->addDrawCall(
		testArrowTextureFunc,
		testWeatherPositionFunc,
		testArrowSizeFunc,
		testArrowPivotFunc,
		testWeatherActiveFunc);

	const UiTextureID testButtonTextureID = MainMenuUiView::allocTestButtonTexture(textureManager, renderer);
	this->testButtonTextureRef.init(testButtonTextureID, renderer);

	const Rect testButtonRect = MainMenuUiView::getTestButtonRect();
	this->addDrawCall(
		this->testButtonTextureRef.get(),
		testButtonRect.getTopLeft(),
		Int2(testButtonRect.getWidth(), testButtonRect.getHeight()),
		PivotType::TopLeft);

	const auto &fontLibrary = game.getFontLibrary();
	const std::string testButtonText = MainMenuUiModel::getTestButtonText();
	const TextBox::InitInfo testButtonInitInfo = MainMenuUiView::getTestButtonTextBoxInitInfo(testButtonText, fontLibrary);
	if (!this->testButtonTextBox.init(testButtonInitInfo, testButtonText, renderer))
	{
		DebugCrash("Couldn't init test button text box.");
	}

	const Rect &testButtonTextBoxRect = this->testButtonTextBox.getRect();
	this->addDrawCall(
		this->testButtonTextBox.getTextureID(),
		testButtonTextBoxRect.getCenter(),
		Int2(testButtonTextBoxRect.getWidth(), testButtonTextBoxRect.getHeight()),
		PivotType::Middle);

	const std::string testTypeText = "Test type: " + MainMenuUiModel::getTestTypeName(this->testType);
	const TextBox::InitInfo testTypeInitInfo = MainMenuUiView::getTestTypeTextBoxInitInfo(fontLibrary);
	if (!this->testTypeTextBox.init(testTypeInitInfo, testTypeText, renderer))
	{
		DebugCrash("Couldn't init test type text box.");
	}

	const Rect &testTypeTextBoxRect = this->testTypeTextBox.getRect();
	this->addDrawCall(
		[this]() { return this->testTypeTextBox.getTextureID(); },
		Int2(testTypeTextBoxRect.getRight(), testTypeTextBoxRect.getTop()),
		Int2(testTypeTextBoxRect.getWidth(), testTypeTextBoxRect.getHeight()),
		PivotType::MiddleRight);

	const std::string testNameText = "Test location: " +
		MainMenuUiModel::getSelectedTestName(game, this->testType, this->testIndex, this->testIndex2);
	const TextBox::InitInfo testNameInitInfo = MainMenuUiView::getTestNameTextBoxInitInfo(fontLibrary);
	if (!this->testNameTextBox.init(testNameInitInfo, testNameText, renderer))
	{
		DebugCrash("Couldn't init test name text box.");
	}

	const Rect &testNameTextBoxRect = this->testNameTextBox.getRect();
	this->addDrawCall(
		[this]() { return this->testNameTextBox.getTextureID(); },
		Int2(testNameTextBoxRect.getRight(), testNameTextBoxRect.getTop()),
		Int2(testNameTextBoxRect.getWidth(), testNameTextBoxRect.getHeight()),
		PivotType::MiddleRight);

	const ArenaTypes::WeatherType testWeatherType = MainMenuUiModel::getSelectedTestWeatherType(this->testWeather);
	const std::string testWeatherText = "Test weather: " + MainMenuUiModel::WeatherTypeNames.at(testWeatherType);
	const TextBox::InitInfo testWeatherInitInfo = MainMenuUiView::getTestWeatherTextBoxInitInfo(fontLibrary);
	if (!this->testWeatherTextBox.init(testWeatherInitInfo, testWeatherText, renderer))
	{
		DebugCrash("Couldn't init test weather text box.");
	}

	const Rect &testWeatherTextBoxRect = this->testWeatherTextBox.getRect();
	this->addDrawCall(
		[this]() { return this->testWeatherTextBox.getTextureID(); },
		[testWeatherTextBoxRect]() { return Int2(testWeatherTextBoxRect.getRight(), testWeatherTextBoxRect.getTop()); },
		[testWeatherTextBoxRect]() { return Int2(testWeatherTextBoxRect.getWidth(), testWeatherTextBoxRect.getHeight()); },
		[]() { return PivotType::MiddleRight; },
		testWeatherActiveFunc);
}
