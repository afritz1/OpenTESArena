#include "MainMenuPanel.h"
#include "MainMenuUiController.h"
#include "MainMenuUiModel.h"
#include "MainMenuUiView.h"
#include "../Game/Game.h"
#include "../UI/ArenaFontName.h"
#include "../UI/CursorData.h"
#include "../UI/FontLibrary.h"
#include "../UI/TextBox.h"
#include "../World/MapType.h"
#include "../WorldMap/LocationUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

MainMenuPanel::MainMenuPanel(Game &game)
	: Panel(game) { }

bool MainMenuPanel::init()
{
	this->loadButton = Button<Game&>(
		MainMenuUiView::LoadButtonCenterPoint,
		MainMenuUiView::LoadButtonWidth,
		MainMenuUiView::LoadButtonHeight,
		MainMenuUiController::onLoadGameButtonSelected);
	this->newButton = Button<Game&>(
		MainMenuUiView::NewGameButtonCenterPoint,
		MainMenuUiView::NewGameButtonWidth,
		MainMenuUiView::NewGameButtonHeight,
		MainMenuUiController::onNewGameButtonSelected);
	this->exitButton = Button<>(
		MainMenuUiView::ExitButtonCenterPoint,
		MainMenuUiView::ExitButtonWidth,
		MainMenuUiView::ExitButtonHeight,
		MainMenuUiController::onExitGameButtonSelected);
	this->quickStartButton = Button<Game&, int, int, const std::string&, const std::optional<ArenaTypes::InteriorType>&,
		ArenaTypes::WeatherType, MapType>(MainMenuUiController::onQuickStartButtonSelected);
	this->testTypeUpButton = Button<int*, int*, int*, int*>(
		MainMenuUiView::TestTypeUpButtonX,
		MainMenuUiView::TestTypeUpButtonY,
		MainMenuUiView::TestTypeUpButtonWidth,
		MainMenuUiView::TestTypeUpButtonHeight,
		MainMenuUiController::onTestTypeUpButtonSelected);
	this->testTypeDownButton = Button<int*, int*, int*, int*>(
		MainMenuUiView::TestTypeDownButtonX,
		MainMenuUiView::TestTypeDownButtonY,
		MainMenuUiView::TestTypeDownButtonWidth,
		MainMenuUiView::TestTypeDownButtonHeight,
		MainMenuUiController::onTestTypeDownButtonSelected);
	this->testIndexUpButton = Button<int*, int*, int*>(
		MainMenuUiView::TestIndexUpButtonX,
		MainMenuUiView::TestIndexUpButtonY,
		MainMenuUiView::TestIndexUpButtonWidth,
		MainMenuUiView::TestIndexUpButtonHeight,
		MainMenuUiController::onTestIndexUpButtonSelected);
	this->testIndexDownButton = Button<int*, int*, int*>(
		MainMenuUiView::TestIndexDownButtonX,
		MainMenuUiView::TestIndexDownButtonY,
		MainMenuUiView::TestIndexDownButtonWidth,
		MainMenuUiView::TestIndexDownButtonHeight,
		MainMenuUiController::onTestIndexDownButtonSelected);
	this->testIndex2UpButton = Button<int, int, int*>(
		MainMenuUiView::TestIndex2UpButtonX,
		MainMenuUiView::TestIndex2UpButtonY,
		MainMenuUiView::TestIndex2UpButtonWidth,
		MainMenuUiView::TestIndex2UpButtonHeight,
		MainMenuUiController::onTestIndex2UpButtonSelected);
	this->testIndex2DownButton = Button<int, int, int*>(
		MainMenuUiView::TestIndex2DownButtonX,
		MainMenuUiView::TestIndex2DownButtonY,
		MainMenuUiView::TestIndex2DownButtonWidth,
		MainMenuUiView::TestIndex2DownButtonHeight,
		MainMenuUiController::onTestIndex2DownButtonSelected);
	this->testWeatherUpButton = Button<int, int*>(
		MainMenuUiView::TestWeatherUpButtonX,
		MainMenuUiView::TestWeatherUpButtonY,
		MainMenuUiView::TestWeatherUpButtonWidth,
		MainMenuUiView::TestWeatherUpButtonHeight,
		MainMenuUiController::onTestWeatherUpButtonSelected);
	this->testWeatherDownButton = Button<int, int*>(
		MainMenuUiView::TestWeatherDownButtonX,
		MainMenuUiView::TestWeatherDownButtonY,
		MainMenuUiView::TestWeatherDownButtonWidth,
		MainMenuUiView::TestWeatherDownButtonHeight,
		MainMenuUiController::onTestWeatherDownButtonSelected);

	this->testType = 0;
	this->testIndex = 0;
	this->testIndex2 = 1;
	this->testWeather = 0;

	if (this->getGame().gameStateIsActive())
	{
		DebugLogError("The game state should not be active on the main menu.");
		return false;
	}

	return true;
}

std::string MainMenuPanel::getSelectedTestName() const
{
	if (this->testType == MainMenuUiModel::TestType_MainQuest)
	{
		auto &game = this->getGame();
		const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
		const auto &exeData = binaryAssetLibrary.getExeData();

		// Decide how to get the main quest dungeon name.
		if (this->testIndex == 0)
		{
			// Start dungeon.
			return String::toUppercase(exeData.locations.startDungeonMifName);
		}
		else if (this->testIndex == (MainMenuUiModel::MainQuestLocationCount - 1))
		{
			// Final dungeon.
			return String::toUppercase(exeData.locations.finalDungeonMifName);
		}
		else
		{
			// Generate the location from the executable data, fetching data from a
			// global function.
			int locationID, provinceID;
			MainMenuUiModel::SpecialCaseType specialCaseType;
			MainMenuUiModel::getMainQuestLocationFromIndex(this->testIndex, exeData, &locationID, &provinceID, &specialCaseType);
			DebugAssert(specialCaseType == MainMenuUiModel::SpecialCaseType::None);

			// Calculate the .MIF name from the dungeon seed.
			const auto &cityData = binaryAssetLibrary.getCityDataFile();
			const uint32_t dungeonSeed = [&cityData, locationID, provinceID]()
			{
				const auto &province = cityData.getProvinceData(provinceID);
				const int localDungeonID = locationID - 32;
				return LocationUtils::getDungeonSeed(localDungeonID, provinceID, province);
			}();

			const std::string mifName = LocationUtils::getMainQuestDungeonMifName(dungeonSeed);
			return String::toUppercase(mifName);
		}
	}
	else if (this->testType == MainMenuUiModel::TestType_Interior)
	{
		const auto &interior = MainMenuUiModel::InteriorLocations.at(this->testIndex);
		return std::get<0>(interior) + std::to_string(this->testIndex2) + ".MIF";
	}
	else if (this->testType == MainMenuUiModel::TestType_City)
	{
		return MainMenuUiModel::CityLocations.at(this->testIndex);
	}
	else if (this->testType == MainMenuUiModel::TestType_Wilderness)
	{
		return MainMenuUiModel::WildernessLocations.at(this->testIndex);
	}
	else if (this->testType == MainMenuUiModel::TestType_Dungeon)
	{
		return MainMenuUiModel::DungeonLocations.at(this->testIndex);
	}
	else
	{
		DebugUnhandledReturnMsg(std::string, std::to_string(this->testType));
	}
}

std::optional<ArenaTypes::InteriorType> MainMenuPanel::getSelectedTestInteriorType() const
{
	if ((this->testType == MainMenuUiModel::TestType_MainQuest) ||
		(this->testType == MainMenuUiModel::TestType_Dungeon))
	{
		return ArenaTypes::InteriorType::Dungeon;
	}
	else if (this->testType == MainMenuUiModel::TestType_Interior)
	{
		const auto &interior = MainMenuUiModel::InteriorLocations.at(this->testIndex);
		return std::get<2>(interior);
	}
	else if ((this->testType == MainMenuUiModel::TestType_City) ||
		(this->testType == MainMenuUiModel::TestType_Wilderness))
	{
		return std::nullopt;
	}
	else
	{
		DebugUnhandledReturnMsg(std::optional<ArenaTypes::InteriorType>, std::to_string(this->testType));
	}
}

ArenaTypes::WeatherType MainMenuPanel::getSelectedTestWeatherType() const
{
	DebugAssertIndex(MainMenuUiModel::Weathers, this->testWeather);
	return MainMenuUiModel::Weathers[this->testWeather];
}

MapType MainMenuPanel::getSelectedTestMapType() const
{
	if ((this->testType == MainMenuUiModel::TestType_MainQuest) ||
		(this->testType == MainMenuUiModel::TestType_Interior) ||
		(this->testType == MainMenuUiModel::TestType_Dungeon))
	{
		return MapType::Interior;
	}
	else if (this->testType == MainMenuUiModel::TestType_City)
	{
		return MapType::City;
	}
	else if (this->testType == MainMenuUiModel::TestType_Wilderness)
	{
		return MapType::Wilderness;
	}
	else
	{
		DebugUnhandledReturnMsg(MapType, std::to_string(this->testType));
	}
}

std::optional<CursorData> MainMenuPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void MainMenuPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool lPressed = inputManager.keyPressed(e, SDLK_l);
	bool sPressed = inputManager.keyPressed(e, SDLK_s);
	bool ePressed = inputManager.keyPressed(e, SDLK_e);
	bool fPressed = inputManager.keyPressed(e, SDLK_f);

	if (lPressed)
	{
		this->loadButton.click(this->getGame());
	}
	else if (sPressed)
	{
		this->newButton.click(this->getGame());
	}
	else if (ePressed)
	{
		this->exitButton.click();
	}
	else if (fPressed)
	{
		// Enter the game world immediately (for testing purposes). Use the test traits
		// selected on the main menu.
		this->quickStartButton.click(this->getGame(),
			this->testType,
			this->testIndex,
			this->getSelectedTestName(),
			this->getSelectedTestInteriorType(),
			this->getSelectedTestWeatherType(),
			this->getSelectedTestMapType());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		if (this->loadButton.contains(originalPoint))
		{
			this->loadButton.click(this->getGame());
		}
		else if (this->newButton.contains(originalPoint))
		{
			this->newButton.click(this->getGame());
		}
		else if (this->exitButton.contains(originalPoint))
		{
			this->exitButton.click();
		}
		else if (MainMenuUiView::TestButtonRect.contains(originalPoint))
		{
			// Enter the game world immediately (for testing purposes). Use the test traits
			// selected on the main menu.
			this->quickStartButton.click(this->getGame(),
				this->testType,
				this->testIndex,
				this->getSelectedTestName(),
				this->getSelectedTestInteriorType(),
				this->getSelectedTestWeatherType(),
				this->getSelectedTestMapType());
		}
		else if (this->testTypeUpButton.contains(originalPoint))
		{
			this->testTypeUpButton.click(&this->testType, &this->testIndex, &this->testIndex2, &this->testWeather);
		}
		else if (this->testTypeDownButton.contains(originalPoint))
		{
			this->testTypeDownButton.click(&this->testType, &this->testIndex, &this->testIndex2, &this->testWeather);
		}
		else if (this->testIndexUpButton.contains(originalPoint))
		{
			this->testIndexUpButton.click(&this->testType, &this->testIndex, &this->testIndex2);
		}
		else if (this->testIndexDownButton.contains(originalPoint))
		{
			this->testIndexDownButton.click(&this->testType, &this->testIndex, &this->testIndex2);
		}
		else if (this->testType == MainMenuUiModel::TestType_Interior)
		{
			// These buttons are only available when selecting interior names.
			if (this->testIndex2UpButton.contains(originalPoint))
			{
				this->testIndex2UpButton.click(this->testType, this->testIndex, &this->testIndex2);
			}
			else if (this->testIndex2DownButton.contains(originalPoint))
			{
				this->testIndex2DownButton.click(this->testType, this->testIndex, &this->testIndex2);
			}
		}
		else if (this->testType == MainMenuUiModel::TestType_City)
		{
			// These buttons are only available when selecting city names.
			if (this->testWeatherUpButton.contains(originalPoint))
			{
				this->testWeatherUpButton.click(this->testType, &this->testWeather);
			}
			else if (this->testWeatherDownButton.contains(originalPoint))
			{
				this->testWeatherDownButton.click(this->testType, &this->testWeather);
			}
		}
		else if (this->testType == MainMenuUiModel::TestType_Wilderness)
		{
			// These buttons are only available when selecting wilderness names.
			if (this->testWeatherUpButton.contains(originalPoint))
			{
				this->testWeatherUpButton.click(this->testType, &this->testWeather);
			}
			else if (this->testWeatherDownButton.contains(originalPoint))
			{
				this->testWeatherDownButton.click(this->testType, &this->testWeather);
			}
		}
	}
}

void MainMenuPanel::renderTestUI(Renderer &renderer)
{
	// Draw test buttons.
	auto &textureManager = this->getGame().getTextureManager();
	const TextureAssetReference &arrowsPaletteTextureAssetRef = MainMenuUiView::getTestArrowsPaletteTextureAssetRef();
	const std::optional<PaletteID> arrowsPaletteID = textureManager.tryGetPaletteID(arrowsPaletteTextureAssetRef);
	if (!arrowsPaletteID.has_value())
	{
		DebugLogError("Couldn't get arrows palette ID for \"" + arrowsPaletteTextureAssetRef.filename + "\".");
		return;
	}

	const TextureAssetReference &arrowsTextureAssetRef = MainMenuUiView::getTestArrowsTextureAssetRef();
	const std::optional<TextureBuilderID> arrowsTextureBuilderID = textureManager.tryGetTextureBuilderID(arrowsTextureAssetRef);
	if (!arrowsTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get arrows texture builder ID for \"" + arrowsTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*arrowsTextureBuilderID, *arrowsPaletteID,
		this->testTypeUpButton.getX(), this->testTypeUpButton.getY(), textureManager);
	renderer.drawOriginal(*arrowsTextureBuilderID, *arrowsPaletteID,
		this->testIndexUpButton.getX(), this->testIndexUpButton.getY(), textureManager);

	if (this->testType == MainMenuUiModel::TestType_Interior)
	{
		renderer.drawOriginal(*arrowsTextureBuilderID, *arrowsPaletteID,
			this->testIndex2UpButton.getX(), this->testIndex2UpButton.getY(), textureManager);
	}
	else if ((this->testType == MainMenuUiModel::TestType_City) ||
		(this->testType == MainMenuUiModel::TestType_Wilderness))
	{
		renderer.drawOriginal(*arrowsTextureBuilderID, *arrowsPaletteID,
			this->testWeatherUpButton.getX(), this->testWeatherUpButton.getY(), textureManager);
	}

	const Rect &testButtonRect = MainMenuUiView::TestButtonRect;
	const Texture testButton = TextureUtils::generate(MainMenuUiView::TestButtonPatternType,
		testButtonRect.getWidth(), testButtonRect.getHeight(), textureManager, renderer);
	renderer.drawOriginal(testButton, testButtonRect.getLeft(), testButtonRect.getTop(),
		testButton.getWidth(), testButton.getHeight());

	const std::string &testFontName = MainMenuUiView::TestButtonFontName;
	const Color testTextColor = MainMenuUiView::getTestButtonTextColor();
	const auto &fontLibrary = this->getGame().getFontLibrary();
	// @todo: need right-aligned text support so this workaround isn't needed.
	// - all other TextureGenInfo's in this scope can be removed at that point too
	const FontDefinition &testFontDef = [&fontLibrary, &testFontName]() -> const FontDefinition&
	{
		int fontDefIndex;
		if (!fontLibrary.tryGetDefinitionIndex(testFontName.c_str(), &fontDefIndex))
		{
			DebugCrash("Couldn't get font definition for \"" + testFontName + "\".");
		}

		return fontLibrary.getDefinition(fontDefIndex);
	}();

	// Draw test text.
	const std::string testButtonText = "Test";
	const TextBox::InitInfo testButtonTextBoxInitInfo = TextBox::InitInfo::makeWithCenter(
		testButtonText,
		MainMenuUiView::TestButtonTextBoxPoint,
		testFontName,
		testTextColor,
		MainMenuUiView::TestButtonTextAlignment,
		fontLibrary);

	TextBox testButtonTextBox;
	if (!testButtonTextBox.init(testButtonTextBoxInitInfo, testButtonText, renderer))
	{
		DebugLogError("Couldn't init test button text box.");
		return;
	}

	const Rect &testButtonTextBoxRect = testButtonTextBox.getRect();
	renderer.drawOriginal(testButtonTextBox.getTexture(), testButtonTextBoxRect.getLeft(), testButtonTextBoxRect.getTop());

	const std::string testTypeText = "Test type: " + MainMenuUiModel::getTestTypeName(this->testType);
	const TextRenderUtils::TextureGenInfo testTypeTextBoxTextureGenInfo =
		TextRenderUtils::makeTextureGenInfo(testTypeText, testFontDef);
	const TextBox::InitInfo testTypeTextBoxInitInfo = TextBox::InitInfo::makeWithXY(
		testTypeText,
		this->testTypeUpButton.getX() - testTypeTextBoxTextureGenInfo.width - 2,
		this->testTypeUpButton.getY() + (testTypeTextBoxTextureGenInfo.height / 2),
		testFontName,
		testTextColor,
		TextAlignment::TopRight,
		fontLibrary);

	TextBox testTypeTextBox;
	if (!testTypeTextBox.init(testTypeTextBoxInitInfo, testTypeText, renderer))
	{
		DebugLogError("Couldn't init test type text box.");
		return;
	}

	const Rect &testTypeTextBoxRect = testTypeTextBox.getRect();
	renderer.drawOriginal(testTypeTextBox.getTexture(), testTypeTextBoxRect.getLeft(), testTypeTextBoxRect.getTop());

	const std::string testNameText = "Test location: " + this->getSelectedTestName();
	const TextRenderUtils::TextureGenInfo testNameTextBoxTextureGenInfo =
		TextRenderUtils::makeTextureGenInfo(testNameText, testFontDef);
	const TextBox::InitInfo testNameTextBoxInitInfo = TextBox::InitInfo::makeWithXY(
		testNameText,
		this->testIndexUpButton.getX() - testNameTextBoxTextureGenInfo.width - 2,
		this->testIndexUpButton.getY() + (testNameTextBoxTextureGenInfo.height / 2),
		testFontName,
		testTextColor,
		TextAlignment::TopRight,
		fontLibrary);
	
	TextBox testNameTextBox;
	if (!testNameTextBox.init(testNameTextBoxInitInfo, testNameText, renderer))
	{
		DebugLogError("Couldn't init test name text box.");
		return;
	}

	const Rect &testNameTextBoxRect = testNameTextBox.getRect();
	renderer.drawOriginal(testNameTextBox.getTexture(), testNameTextBoxRect.getLeft(), testNameTextBoxRect.getTop());

	// Draw weather text if applicable.
	if ((this->testType == MainMenuUiModel::TestType_City) || (this->testType == MainMenuUiModel::TestType_Wilderness))
	{
		const ArenaTypes::WeatherType weatherType = this->getSelectedTestWeatherType();
		const std::string &weatherName = MainMenuUiModel::WeatherTypeNames.at(weatherType);
		const std::string testWeatherText = "Test weather: " + weatherName;
		const TextRenderUtils::TextureGenInfo testWeatherTextBoxTextureGenInfo =
			TextRenderUtils::makeTextureGenInfo(testWeatherText, testFontDef);
		const TextBox::InitInfo testWeatherTextBoxInitInfo = TextBox::InitInfo::makeWithXY(
			testWeatherText,
			this->testWeatherUpButton.getX() - testWeatherTextBoxTextureGenInfo.width - 2,
			this->testWeatherUpButton.getY() + (testWeatherTextBoxTextureGenInfo.height / 2),
			testFontName,
			testTextColor,
			TextAlignment::TopRight,
			fontLibrary);

		TextBox testWeatherTextBox;
		if (!testWeatherTextBox.init(testWeatherTextBoxInitInfo, testWeatherText, renderer))
		{
			DebugLogError("Couldn't init test weather text box.");
			return;
		}

		const Rect &testWeatherTextBoxRect = testWeatherTextBox.getRect();
		renderer.drawOriginal(testWeatherTextBox.getTexture(), testWeatherTextBoxRect.getLeft(), testWeatherTextBoxRect.getTop());
	}
}

void MainMenuPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw main menu.
	auto &textureManager = this->getGame().getTextureManager();
	const TextureAssetReference &backgroundPaletteTextureAssetRef = MainMenuUiView::getPaletteTextureAssetRef();
	const std::optional<PaletteID> mainMenuPaletteID = textureManager.tryGetPaletteID(backgroundPaletteTextureAssetRef);
	if (!mainMenuPaletteID.has_value())
	{
		DebugLogError("Couldn't get main menu palette ID for \"" + backgroundPaletteTextureAssetRef.filename + "\".");
		return;
	}

	const TextureAssetReference &backgroundTextureAssetRef = MainMenuUiView::getBackgroundTextureAssetRef();
	const std::optional<TextureBuilderID> mainMenuTextureBuilderID = textureManager.tryGetTextureBuilderID(backgroundTextureAssetRef);
	if (!mainMenuTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get main menu texture builder ID for \"" + backgroundTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*mainMenuTextureBuilderID, *mainMenuPaletteID, textureManager);
	this->renderTestUI(renderer);
}
