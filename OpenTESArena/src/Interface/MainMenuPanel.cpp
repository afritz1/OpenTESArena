#include "MainMenuPanel.h"
#include "MainMenuUiController.h"
#include "MainMenuUiModel.h"
#include "MainMenuUiView.h"
#include "../Game/Game.h"
#include "../World/MapType.h"
#include "../WorldMap/LocationUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

MainMenuPanel::MainMenuPanel(Game &game)
	: Panel(game)
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

	// The game state should not be active on the main menu.
	DebugAssert(!game.gameStateIsActive());
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
	else
	{
		return MainMenuUiModel::DungeonLocations.at(this->testIndex);
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
		DebugCrash("Unimplemented test type \"" + std::to_string(this->testType) + "\".");
		return std::nullopt;
	}
}

ArenaTypes::WeatherType MainMenuPanel::getSelectedTestWeatherType() const
{
	return MainMenuUiModel::Weathers.at(this->testWeather);
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
	else
	{
		return MapType::Wilderness;
	}
}

std::optional<Panel::CursorData> MainMenuPanel::getCurrentCursor() const
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

	// Draw test text.
	const auto &fontLibrary = this->getGame().getFontLibrary();
	const RichTextString testButtonText(
		"Test",
		MainMenuUiView::TestButtonFontName,
		MainMenuUiView::getTestButtonTextColor(),
		MainMenuUiView::TestButtonTextAlignment,
		fontLibrary);

	const TextBox testButtonTextBox(MainMenuUiView::TestButtonTextBoxPoint, testButtonText, fontLibrary, renderer);
	renderer.drawOriginal(testButtonTextBox.getTexture(), testButtonTextBox.getX(), testButtonTextBox.getY());

	const std::string testTypeName = MainMenuUiModel::getTestTypeName(this->testType);
	const RichTextString testTypeText(
		"Test type: " + testTypeName,
		testButtonText.getFontName(),
		testButtonText.getColor(),
		TextAlignment::Left,
		fontLibrary);

	const int testTypeTextBoxX = this->testTypeUpButton.getX() - testTypeText.getDimensions().x - 2;
	const int testTypeTextBoxY = this->testTypeUpButton.getY() + (testTypeText.getDimensions().y / 2);
	const TextBox testTypeTextBox(testTypeTextBoxX, testTypeTextBoxY, testTypeText, fontLibrary, renderer);
	renderer.drawOriginal(testTypeTextBox.getTexture(), testTypeTextBox.getX(), testTypeTextBox.getY());

	const RichTextString testNameText(
		"Test location: " + this->getSelectedTestName(),
		testTypeText.getFontName(),
		testTypeText.getColor(),
		testTypeText.getAlignment(),
		fontLibrary);
	const int testNameTextBoxX = this->testIndexUpButton.getX() - testNameText.getDimensions().x - 2;
	const int testNameTextBoxY = this->testIndexUpButton.getY() + (testNameText.getDimensions().y / 2);
	const TextBox testNameTextBox(testNameTextBoxX, testNameTextBoxY, testNameText, fontLibrary, renderer);
	renderer.drawOriginal(testNameTextBox.getTexture(), testNameTextBox.getX(), testNameTextBox.getY());

	// Draw weather text if applicable.
	if ((this->testType == MainMenuUiModel::TestType_City) || (this->testType == MainMenuUiModel::TestType_Wilderness))
	{
		const ArenaTypes::WeatherType weatherType = this->getSelectedTestWeatherType();
		const std::string &weatherName = MainMenuUiModel::WeatherTypeNames.at(weatherType);

		const RichTextString testWeatherText(
			"Test weather: " + weatherName,
			testTypeText.getFontName(),
			testTypeText.getColor(),
			testTypeText.getAlignment(),
			fontLibrary);

		const int testWeatherTextBoxX = this->testWeatherUpButton.getX() - testWeatherText.getDimensions().x - 2;
		const int testWeatherTextBoxY = this->testWeatherUpButton.getY() + (testWeatherText.getDimensions().y / 2);
		const TextBox testWeatherTextBox(testWeatherTextBoxX, testWeatherTextBoxY,
			testWeatherText, fontLibrary, renderer);

		renderer.drawOriginal(testWeatherTextBox.getTexture(), testWeatherTextBox.getX(), testWeatherTextBox.getY());
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
