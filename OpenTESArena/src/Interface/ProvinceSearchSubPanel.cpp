#include <algorithm>

#include "SDL.h"

#include "CursorAlignment.h"
#include "ProvinceMapPanel.h"
#include "ProvinceSearchSubPanel.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextEntry.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/CityDataFile.h"
#include "../Game/Game.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"

#include "components/utilities/String.h"

const Int2 ProvinceSearchSubPanel::DEFAULT_TEXT_CURSOR_POSITION(85, 100);

ProvinceSearchSubPanel::ProvinceSearchSubPanel(Game &game,
	ProvinceMapPanel &provinceMapPanel, int provinceID)
	: Panel(game), provinceMapPanel(provinceMapPanel)
{
	// Don't initialize the locations list box until it's reached, since its contents
	// may depend on the search results.
	this->parchment = TextureUtils::generate(TextureUtils::PatternType::Parchment, 280, 40,
		game.getTextureManager(), game.getRenderer());

	this->textTitleTextBox = [&game]()
	{
		const int x = 30;
		const int y = 89;

		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
		const std::string &text = exeData.travel.searchTitleText;

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			FontName::Arena,
			Color(52, 24, 8),
			TextAlignment::Left,
			fontLibrary);

		return std::make_unique<TextBox>(x, y, richText, fontLibrary, game.getRenderer());
	}();

	this->textEntryTextBox = [&game]()
	{
		const int x = ProvinceSearchSubPanel::DEFAULT_TEXT_CURSOR_POSITION.x;
		const int y = ProvinceSearchSubPanel::DEFAULT_TEXT_CURSOR_POSITION.y;

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			std::string(),
			FontName::Arena,
			Color(52, 24, 8),
			TextAlignment::Left,
			fontLibrary);

		return std::make_unique<TextBox>(x, y, richText, fontLibrary, game.getRenderer());
	}();

	this->textAcceptButton = []()
	{
		auto function = [](Game &game, ProvinceSearchSubPanel &panel)
		{
			SDL_StopTextInput();

			// Determine what to do with the current location name. If it is a valid match
			// with one of the visible locations in the province, then select that location.
			// Otherwise, display the list box of locations sorted by their location index.
			const int *exactLocationIndex = nullptr;
			panel.locationsListIndices = ProvinceSearchSubPanel::getMatchingLocations(game,
				panel.locationName, panel.provinceID, &exactLocationIndex);

			if (exactLocationIndex != nullptr)
			{
				// The location name is an exact match. Try to select the location in the province
				// map panel based on whether the player is already there.
				panel.provinceMapPanel.trySelectLocation(*exactLocationIndex);

				// Return to the province map panel.
				game.popSubPanel();
			}
			else
			{
				// No exact match. Change to list mode.
				panel.initLocationsListBox();
				panel.mode = ProvinceSearchSubPanel::Mode::List;
			}
		};
		return Button<Game&, ProvinceSearchSubPanel&>(function);
	}();

	this->listAcceptButton = []()
	{
		auto function = [](Game &game, ProvinceSearchSubPanel &panel, int locationID)
		{
			// Try to select the location in the province map panel based on whether the
			// player is already there.
			panel.provinceMapPanel.trySelectLocation(locationID);

			// Return to the province map panel.
			game.popSubPanel();
		};
		return Button<Game&, ProvinceSearchSubPanel&, int>(function);
	}();

	this->listUpButton = []
	{
		const Int2 center(70, 24);
		const int w = 8;
		const int h = 8;
		auto function = [](ProvinceSearchSubPanel &panel)
		{
			// Scroll the list box up one if able.
			if (panel.locationsListBox->getScrollIndex() > 0)
			{
				panel.locationsListBox->scrollUp();
			}
		};
		return Button<ProvinceSearchSubPanel&>(center, w, h, function);
	}();

	this->listDownButton = []
	{
		const Int2 center(70, 97);
		const int w = 8;
		const int h = 8;
		auto function = [](ProvinceSearchSubPanel &panel)
		{
			// Scroll the list box down one if able.
			const int scrollIndex = panel.locationsListBox->getScrollIndex();
			const int elementCount = panel.locationsListBox->getElementCount();
			const int maxDisplayedCount = panel.locationsListBox->getMaxDisplayedCount();
			if (scrollIndex < (elementCount - maxDisplayedCount))
			{
				panel.locationsListBox->scrollDown();
			}
		};
		return Button<ProvinceSearchSubPanel&>(center, w, h, function);
	}();

	this->mode = ProvinceSearchSubPanel::Mode::TextEntry;
	this->provinceID = provinceID;

	// Start with text input enabled (see handleTextEntryEvent()).
	SDL_StartTextInput();
}

std::optional<Panel::CursorData> ProvinceSearchSubPanel::getCurrentCursor() const
{
	if (this->mode == ProvinceSearchSubPanel::Mode::TextEntry)
	{
		// No mouse cursor when typing.
		return std::nullopt;
	}
	else
	{
		return this->getDefaultCursor();
	}
}

std::vector<int> ProvinceSearchSubPanel::getMatchingLocations(Game &game,
	const std::string &locationName, int provinceIndex, const int **exactLocationIndex)
{
	auto &gameData = game.getGameData();
	const WorldMapDefinition &worldMapDef = gameData.getWorldMapDefinition();
	const WorldMapInstance &worldMapInst = gameData.getWorldMapInstance();

	const ProvinceInstance &provinceInst = worldMapInst.getProvinceInstance(provinceIndex);
	const int provinceDefIndex = provinceInst.getProvinceDefIndex();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceDefIndex);

	// Iterate through all locations in the province. If any visible location's name has
	// a match with the one entered, then add the location to the matching IDs.
	std::vector<int> locationIndices;
	for (int i = 0; i < provinceInst.getLocationCount(); i++)
	{
		const LocationInstance &locationInst = provinceInst.getLocationInstance(i);

		// Only check visible locations.
		if (locationInst.isVisible())
		{
			const int locationDefIndex = locationInst.getLocationDefIndex();
			const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);
			const std::string &curLocationName = locationInst.getName(locationDef);

			// See if the location names are an exact match.
			const bool isExactMatch = String::caseInsensitiveEquals(locationName, curLocationName);

			if (isExactMatch)
			{
				locationIndices.push_back(i);
				*exactLocationIndex = &locationIndices.back();
				break;
			}
			else
			{
				// Approximate match behavior. If the given location name is a case-insensitive
				// substring of the current location, it's a match.
				const std::string locNameLower = String::toLowercase(locationName);
				const std::string locDataNameLower = String::toLowercase(curLocationName);
				const bool isApproxMatch = locDataNameLower.find(locNameLower) != std::string::npos;

				if (isApproxMatch)
				{
					locationIndices.push_back(i);
				}
			}
		}
	}

	// If no exact or approximate matches, just fill the list with all visible location IDs.
	if (locationIndices.empty())
	{
		for (int i = 0; i < provinceInst.getLocationCount(); i++)
		{
			const LocationInstance &locationInst = provinceInst.getLocationInstance(i);
			if (locationInst.isVisible())
			{
				locationIndices.push_back(i);
			}
		}
	}

	// If one approximate match was found and no exact match was found, treat the approximate
	// match as the nearest.
	if ((locationIndices.size() == 1) && (*exactLocationIndex == nullptr))
	{
		*exactLocationIndex = &locationIndices.front();
	}

	// The original game orders locations by their location ID, but that's hardly helpful for the
	// player because they memorize places by name. Therefore, this feature will deviate from
	// the original behavior for the sake of convenience. If the list isn't sorted alphabetically,
	// then it takes the player linear time to find a location in it, which essentially isn't any
	// faster than hovering over each location individually.
	std::sort(locationIndices.begin(), locationIndices.end(),
		[&provinceInst, &provinceDef](int a, int b)
	{
		const LocationInstance &locationInstA = provinceInst.getLocationInstance(a);
		const LocationInstance &locationInstB = provinceInst.getLocationInstance(b);
		const int locationDefIndexA = locationInstA.getLocationDefIndex();
		const int locationDefIndexB = locationInstB.getLocationDefIndex();
		const LocationDefinition &locationDefA = provinceDef.getLocationDef(locationDefIndexA);
		const LocationDefinition &locationDefB = provinceDef.getLocationDef(locationDefIndexB);

		const std::string &aName = locationInstA.getName(locationDefA);
		const std::string &bName = locationInstB.getName(locationDefB);
		return aName.compare(bName) < 0;
	});

	return locationIndices;
}

std::string ProvinceSearchSubPanel::getBackgroundFilename() const
{
	const auto &exeData = this->getGame().getBinaryAssetLibrary().getExeData();
	const auto &provinceImgFilenames = exeData.locations.provinceImgFilenames;
	const std::string &filename = provinceImgFilenames.at(this->provinceID);

	// Set all characters to uppercase because the texture manager expects 
	// extensions to be uppercase, and most filenames in A.EXE are lowercase.
	return String::toUppercase(filename);
}

void ProvinceSearchSubPanel::initLocationsListBox()
{
	auto &game = this->getGame();

	this->locationsListBox = [this, &game]()
	{
		const int x = 85;
		const int y = 34;

		auto &gameData = game.getGameData();
		const WorldMapDefinition &worldMapDef = gameData.getWorldMapDefinition();
		const WorldMapInstance &worldMapInst = gameData.getWorldMapInstance();
		const ProvinceInstance &provinceInst = worldMapInst.getProvinceInstance(this->provinceID);
		const int provinceDefIndex = provinceInst.getProvinceDefIndex();
		const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceDefIndex);

		std::vector<std::string> locationNames;
		for (const int locationIndex : this->locationsListIndices)
		{
			const LocationInstance &locationInst = provinceInst.getLocationInstance(locationIndex);
			const int locationDefIndex = locationInst.getLocationDefIndex();
			const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);
			const std::string &locationName = locationInst.getName(locationDef);

			locationNames.push_back(locationName);
		}

		const int maxDisplayed = 6;
		return std::make_unique<ListBox>(
			x,
			y,
			Color(52, 24, 8),
			locationNames,
			FontName::Arena,
			maxDisplayed,
			game.getFontLibrary(),
			game.getRenderer());
	}();
}

void ProvinceSearchSubPanel::handleTextEntryEvent(const SDL_Event &e)
{
	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool enterPressed = inputManager.keyPressed(e, SDLK_RETURN) ||
		inputManager.keyPressed(e, SDLK_KP_ENTER);
	const bool backspacePressed = inputManager.keyPressed(e, SDLK_BACKSPACE) ||
		inputManager.keyPressed(e, SDLK_KP_BACKSPACE);
	const bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	// In the original game, right clicking here is registered as enter, but that seems
	// inconsistent/unintuitive, so I'm making it register as escape instead.
	if (escapePressed || rightClick)
	{
		// Return to the province map panel.
		game.popSubPanel();
	}
	else if (enterPressed)
	{
		// Begin the next step in the location search. Run the entered text through some
		// checks to see if it matches any location names.
		this->textAcceptButton.click(this->getGame(), *this);
	}
	else
	{
		// Listen for SDL text input and changes in text. Letters, numbers, spaces, and
		// symbols are allowed.
		auto charIsAllowed = [](char c)
		{
			return (c >= ' ') && (c < 127);
		};

		const bool textChanged = TextEntry::updateText(this->locationName, e,
			backspacePressed, charIsAllowed, ProvinceSearchSubPanel::MAX_NAME_LENGTH);

		if (textChanged)
		{
			// Update the displayed location name.
			this->textEntryTextBox = [this, &game]()
			{
				const int x = ProvinceSearchSubPanel::DEFAULT_TEXT_CURSOR_POSITION.x;
				const int y = ProvinceSearchSubPanel::DEFAULT_TEXT_CURSOR_POSITION.y;

				const RichTextString &oldRichText = this->textEntryTextBox->getRichText();

				const auto &fontLibrary = game.getFontLibrary();
				const RichTextString richText(
					this->locationName,
					oldRichText.getFontName(),
					oldRichText.getColor(),
					oldRichText.getAlignment(),
					fontLibrary);

				return std::make_unique<TextBox>(x, y, richText, fontLibrary, game.getRenderer());
			}();
		}
	}
}

void ProvinceSearchSubPanel::handleListEvent(const SDL_Event &e)
{
	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	if (escapePressed || rightClick)
	{
		// Return to the province map panel.
		game.popSubPanel();
	}
	else
	{
		const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
		const bool mouseWheelUp = inputManager.mouseWheeledUp(e);
		const bool mouseWheelDown = inputManager.mouseWheeledDown(e);

		// If over one of the text elements, select it and perform the travel
		// behavior depending on whether the player is already there.
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		// Custom width to better fill the screen-space.
		const int listBoxWidth = 147;
		const Rect listBoxRect(
			this->locationsListBox->getPoint().x,
			this->locationsListBox->getPoint().y,
			listBoxWidth,
			this->locationsListBox->getDimensions().y);

		if (listBoxRect.contains(originalPoint))
		{
			if (leftClick)
			{
				// Verify that the clicked index is valid. If so, get the location ID and
				// try to select the location.
				const int index = this->locationsListBox->getClickedIndex(originalPoint);
				if ((index >= 0) && (index < this->locationsListBox->getElementCount()))
				{
					this->listAcceptButton.click(this->getGame(), *this,
						this->locationsListIndices.at(index));
				}
			}
			else if (mouseWheelUp)
			{
				this->listUpButton.click(*this);
			}
			else if (mouseWheelDown)
			{
				this->listDownButton.click(*this);
			}
		}
		else if (leftClick)
		{
			// Check scroll buttons (they are outside the list box to the left).
			if (this->listUpButton.contains(originalPoint))
			{
				this->listUpButton.click(*this);
			}
			else if (this->listDownButton.contains(originalPoint))
			{
				this->listDownButton.click(*this);
			}
		}
	}
}

void ProvinceSearchSubPanel::handleEvent(const SDL_Event &e)
{
	if (this->mode == ProvinceSearchSubPanel::Mode::TextEntry)
	{
		this->handleTextEntryEvent(e);
	}
	else
	{
		this->handleListEvent(e);
	}
}

void ProvinceSearchSubPanel::tick(double dt)
{
	// @todo: eventually blink text input cursor in text entry, and listen for
	// scrolling in list.
	static_cast<void>(dt);
}

void ProvinceSearchSubPanel::renderTextEntry(Renderer &renderer)
{
	// Draw parchment.
	renderer.drawOriginal(this->parchment,
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - (this->parchment.getWidth() / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - (this->parchment.getHeight() / 2) - 1);

	// Draw text: title, location name.
	renderer.drawOriginal(this->textTitleTextBox->getTexture(),
		this->textTitleTextBox->getX(), this->textTitleTextBox->getY());
	renderer.drawOriginal(this->textEntryTextBox->getTexture(),
		this->textEntryTextBox->getX(), this->textEntryTextBox->getY());

	// @todo: draw blinking cursor.
}

void ProvinceSearchSubPanel::renderList(Renderer &renderer)
{
	// Draw list background.
	auto &game = this->getGame();
	auto &textureManager = game.getTextureManager();
	const std::string listBackgroundPaletteFilename = this->getBackgroundFilename();
	const std::optional<PaletteID> listBackgroundPaletteID =
		textureManager.tryGetPaletteID(listBackgroundPaletteFilename.c_str());
	if (!listBackgroundPaletteID.has_value())
	{
		DebugLogError("Couldn't get list background palette ID for \"" + listBackgroundPaletteFilename + "\".");
		return;
	}
	
	const std::string &listBackgroundTextureFilename = ArenaTextureName::PopUp8;
	const std::optional<TextureBuilderID> listBackgroundTextureBuilderID =
		textureManager.tryGetTextureBuilderID(listBackgroundTextureFilename.c_str());
	if (!listBackgroundTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get list background texture builder ID for \"" + listBackgroundTextureFilename + "\".");
		return;
	}

	constexpr int listBackgroundX = 57;
	constexpr int listBackgroundY = 11;
	renderer.drawOriginal(*listBackgroundTextureBuilderID, *listBackgroundPaletteID,
		listBackgroundX, listBackgroundY, textureManager);

	// Draw list box text.
	renderer.drawOriginal(this->locationsListBox->getTexture(),
		this->locationsListBox->getPoint().x, this->locationsListBox->getPoint().y);
}

void ProvinceSearchSubPanel::render(Renderer &renderer)
{
	if (this->mode == ProvinceSearchSubPanel::Mode::TextEntry)
	{
		this->renderTextEntry(renderer);
	}
	else
	{
		this->renderList(renderer);
	}
}
