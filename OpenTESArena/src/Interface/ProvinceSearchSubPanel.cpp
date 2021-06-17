#include <algorithm>

#include "SDL.h"

#include "ProvinceMapPanel.h"
#include "ProvinceMapUiController.h"
#include "ProvinceMapUiModel.h"
#include "ProvinceMapUiView.h"
#include "ProvinceSearchSubPanel.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/CityDataFile.h"
#include "../Game/Game.h"
#include "../Media/Color.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/CursorData.h"
#include "../UI/FontName.h"
#include "../UI/RichTextString.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextEntry.h"

#include "components/utilities/String.h"

ProvinceSearchSubPanel::ProvinceSearchSubPanel(Game &game)
	: Panel(game) { }

bool ProvinceSearchSubPanel::init(ProvinceMapPanel &provinceMapPanel, int provinceID)
{
	auto &game = this->getGame();

	// Don't initialize the locations list box until it's reached, since its contents
	// may depend on the search results.
	this->parchment = TextureUtils::generate(
		ProvinceMapUiView::SearchSubPanelTexturePattern,
		ProvinceMapUiView::SearchSubPanelTextureWidth,
		ProvinceMapUiView::SearchSubPanelTextureHeight,
		game.getTextureManager(),
		game.getRenderer());

	this->textTitleTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			ProvinceMapUiModel::getSearchSubPanelTitleText(game),
			ProvinceMapUiView::SearchSubPanelTitleFontName,
			ProvinceMapUiView::SearchSubPanelTitleColor,
			ProvinceMapUiView::SearchSubPanelTitleTextAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			ProvinceMapUiView::SearchSubPanelTitleTextBoxX,
			ProvinceMapUiView::SearchSubPanelTitleTextBoxY,
			richText,
			fontLibrary,
			game.getRenderer());
	}();

	this->textEntryTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			std::string(),
			ProvinceMapUiView::SearchSubPanelTextEntryFontName,
			ProvinceMapUiView::SearchSubPanelTextEntryColor,
			ProvinceMapUiView::SearchSubPanelTextEntryTextAlignment,
			fontLibrary);

		const Int2 &position = ProvinceMapUiView::SearchSubPanelDefaultTextCursorPosition;
		return std::make_unique<TextBox>(
			position.x,
			position.y,
			richText,
			fontLibrary,
			game.getRenderer());
	}();

	this->textAcceptButton = Button<Game&, ProvinceSearchSubPanel&>(ProvinceMapUiController::onSearchTextAccepted);
	this->listUpButton = Button<ListBox&>(
		ProvinceMapUiView::SearchSubPanelListUpButtonCenterPoint,
		ProvinceMapUiView::SearchSubPanelListUpButtonWidth,
		ProvinceMapUiView::SearchSubPanelListUpButtonHeight,
		ProvinceMapUiController::onSearchListUpButtonSelected);
	this->listDownButton = Button<ListBox&>(
		ProvinceMapUiView::SearchSubPanelListDownButtonCenterPoint,
		ProvinceMapUiView::SearchSubPanelListDownButtonWidth,
		ProvinceMapUiView::SearchSubPanelListDownButtonHeight,
		ProvinceMapUiController::onSearchListDownButtonSelected);

	this->provinceMapPanel = &provinceMapPanel;
	this->mode = ProvinceMapUiModel::SearchMode::TextEntry;
	this->provinceID = provinceID;

	// Start with text input enabled (see handleTextEntryEvent()).
	SDL_StartTextInput();

	return true;
}

std::optional<CursorData> ProvinceSearchSubPanel::getCurrentCursor() const
{
	if (this->mode == ProvinceMapUiModel::SearchMode::TextEntry)
	{
		// No mouse cursor when typing.
		return std::nullopt;
	}
	else
	{
		return this->getDefaultCursor();
	}
}

void ProvinceSearchSubPanel::initLocationsListBox()
{
	// @todo: move the locationNames into UiModel
	auto &game = this->getGame();
	auto &gameState = game.getGameState();
	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
	const WorldMapInstance &worldMapInst = gameState.getWorldMapInstance();
	const ProvinceInstance &provinceInst = worldMapInst.getProvinceInstance(this->provinceID);
	const int provinceDefIndex = provinceInst.getProvinceDefIndex();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceDefIndex);

	this->locationsListBox.init(ProvinceMapUiView::SearchSubPanelListBoxRect,
		ProvinceMapUiView::makeSearchSubPanelListBoxProperties(game.getFontLibrary()), game.getRenderer());

	for (int i = 0; i < static_cast<int>(this->locationsListIndices.size()); i++)
	{
		const int locationIndex = this->locationsListIndices[i];
		const LocationInstance &locationInst = provinceInst.getLocationInstance(locationIndex);
		const int locationDefIndex = locationInst.getLocationDefIndex();
		const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);
		std::string locationName = locationInst.getName(locationDef);

		this->locationsListBox.add(std::move(locationName));
		this->locationsListBox.setCallback(i, [this, &game, i]()
		{
			DebugAssertIndex(this->locationsListIndices, i);
			const int locationsListIndex = this->locationsListIndices[i];
			ProvinceMapUiController::onSearchListLocationSelected(game, *this, locationsListIndex);
		});
	}
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
		// Listen for SDL text input and changes in text.
		const bool textChanged = TextEntry::updateText(this->locationName, e, backspacePressed,
			ProvinceMapUiModel::isCharAllowedInSearchText, ProvinceMapUiModel::SearchSubPanelMaxNameLength);

		if (textChanged)
		{
			// Update the displayed location name.
			this->textEntryTextBox = [this, &game]()
			{
				const RichTextString &oldRichText = this->textEntryTextBox->getRichText();

				const auto &fontLibrary = game.getFontLibrary();
				const RichTextString richText(
					this->locationName,
					oldRichText.getFontName(),
					oldRichText.getColor(),
					oldRichText.getAlignment(),
					fontLibrary);

				const Int2 &position = ProvinceMapUiView::SearchSubPanelDefaultTextCursorPosition;
				return std::make_unique<TextBox>(
					position.x,
					position.y,
					richText,
					fontLibrary,
					game.getRenderer());
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
		const Int2 originalPoint = this->getGame().getRenderer().nativeToOriginal(mousePosition);

		// Custom width to better fill the screen-space.
		const Rect &listBoxRect = this->locationsListBox.getRect();
		if (listBoxRect.contains(originalPoint))
		{
			if (leftClick)
			{
				for (int i = 0; i < this->locationsListBox.getCount(); i++)
				{
					const Rect &itemGlobalRect = this->locationsListBox.getItemGlobalRect(i);
					if (itemGlobalRect.contains(originalPoint))
					{
						const ListBox::ItemCallback &itemCallback = this->locationsListBox.getCallback(i);
						itemCallback();
						break;
					}
				}
			}
			else if (mouseWheelUp)
			{
				this->listUpButton.click(this->locationsListBox);
			}
			else if (mouseWheelDown)
			{
				this->listDownButton.click(this->locationsListBox);
			}
		}
		else if (leftClick)
		{
			// Check scroll buttons (they are outside the list box to the left).
			if (this->listUpButton.contains(originalPoint))
			{
				this->listUpButton.click(this->locationsListBox);
			}
			else if (this->listDownButton.contains(originalPoint))
			{
				this->listDownButton.click(this->locationsListBox);
			}
		}
	}
}

void ProvinceSearchSubPanel::handleEvent(const SDL_Event &e)
{
	if (this->mode == ProvinceMapUiModel::SearchMode::TextEntry)
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
	const int parchmentX = ProvinceMapUiView::getSearchSubPanelTextEntryTextureX(this->parchment.getWidth());
	const int parchmentY = ProvinceMapUiView::getSearchSubPanelTextEntryTextureY(this->parchment.getHeight());
	renderer.drawOriginal(this->parchment, parchmentX, parchmentY);

	// Draw text: title, location name.
	renderer.drawOriginal(this->textTitleTextBox->getTexture(), this->textTitleTextBox->getX(), this->textTitleTextBox->getY());
	renderer.drawOriginal(this->textEntryTextBox->getTexture(), this->textEntryTextBox->getX(), this->textEntryTextBox->getY());

	// @todo: draw blinking cursor.
}

void ProvinceSearchSubPanel::renderList(Renderer &renderer)
{
	// Draw list background.
	auto &game = this->getGame();
	auto &textureManager = game.getTextureManager();
	const TextureAssetReference listBackgroundPaletteTextureAssetRef =
		ProvinceMapUiView::getSearchSubPanelListPaletteTextureAssetRef(game, this->provinceID);
	const std::optional<PaletteID> listBackgroundPaletteID =
		textureManager.tryGetPaletteID(listBackgroundPaletteTextureAssetRef);
	if (!listBackgroundPaletteID.has_value())
	{
		DebugLogError("Couldn't get list background palette ID for \"" + listBackgroundPaletteTextureAssetRef.filename + "\".");
		return;
	}
	
	const TextureAssetReference listBackgroundTextureAssetRef = ProvinceMapUiView::getSearchSubPanelListTextureAssetRef();
	const std::optional<TextureBuilderID> listBackgroundTextureBuilderID =
		textureManager.tryGetTextureBuilderID(listBackgroundTextureAssetRef);
	if (!listBackgroundTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get list background texture builder ID for \"" + listBackgroundTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*listBackgroundTextureBuilderID, *listBackgroundPaletteID,
		ProvinceMapUiView::SearchSubPanelListTextureX, ProvinceMapUiView::SearchSubPanelListTextureY,
		textureManager);

	// Draw list box text.
	const Rect &locationsListBoxRect = this->locationsListBox.getRect();
	renderer.drawOriginal(this->locationsListBox.getTexture(),
		locationsListBoxRect.getLeft(), locationsListBoxRect.getTop());
}

void ProvinceSearchSubPanel::render(Renderer &renderer)
{
	if (this->mode == ProvinceMapUiModel::SearchMode::TextEntry)
	{
		this->renderTextEntry(renderer);
	}
	else
	{
		this->renderList(renderer);
	}
}
