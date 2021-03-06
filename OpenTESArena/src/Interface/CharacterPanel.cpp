#include "SDL.h"

#include "CharacterPanel.h"
#include "CharacterSheetUiController.h"
#include "CharacterSheetUiModel.h"
#include "CharacterSheetUiView.h"
#include "../Game/Game.h"
#include "../UI/CursorData.h"

#include "components/debug/Debug.h"

CharacterPanel::CharacterPanel(Game &game)
	: Panel(game) { }

bool CharacterPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	const auto &fontLibrary = game.getFontLibrary();

	const std::string playerNameText = CharacterSheetUiModel::getPlayerName(game);
	const TextBox::InitInfo playerNameTextBoxInitInfo =
		CharacterSheetUiView::getPlayerNameTextBoxInitInfo(playerNameText, fontLibrary);
	if (!this->playerNameTextBox.init(playerNameTextBoxInitInfo, playerNameText, renderer))
	{
		DebugLogError("Couldn't init player name text box.");
		return false;
	}

	const std::string playerRaceText = CharacterSheetUiModel::getPlayerRaceName(game);
	const TextBox::InitInfo playerRaceTextBoxInitInfo =
		CharacterSheetUiView::getPlayerRaceTextBoxInitInfo(playerRaceText, fontLibrary);
	if (!this->playerRaceTextBox.init(playerRaceTextBoxInitInfo, playerRaceText, renderer))
	{
		DebugLogError("Couldn't init player race text box.");
		return false;
	}

	const std::string playerClassText = CharacterSheetUiModel::getPlayerClassName(game);
	const TextBox::InitInfo playerClassTextBoxInitInfo =
		CharacterSheetUiView::getPlayerClassTextBoxInitInfo(playerClassText, fontLibrary);
	if (!this->playerClassTextBox.init(playerClassTextBoxInitInfo, playerClassText, renderer))
	{
		DebugLogError("Couldn't init player class text box.");
		return false;
	}

	this->doneButton = Button<Game&>(
		CharacterSheetUiView::DoneButtonCenterPoint,
		CharacterSheetUiView::DoneButtonWidth,
		CharacterSheetUiView::DoneButtonHeight,
		CharacterSheetUiController::onDoneButtonSelected);
	this->nextPageButton = Button<Game&>(
		CharacterSheetUiView::NextPageButtonX,
		CharacterSheetUiView::NextPageButtonY,
		CharacterSheetUiView::NextPageButtonWidth,
		CharacterSheetUiView::NextPageButtonHeight,
		CharacterSheetUiController::onNextPageButtonSelected);

	return true;
}

std::optional<CursorData> CharacterPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void CharacterPanel::handleEvent(const SDL_Event &e)
{
	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool tabPressed = inputManager.keyPressed(e, SDLK_TAB);

	if (escapePressed || tabPressed)
	{
		this->doneButton.click(game);
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = game.getRenderer().nativeToOriginal(mousePosition);

		if (this->doneButton.contains(mouseOriginalPoint))
		{
			this->doneButton.click(game);
		}
		else if (this->nextPageButton.contains(mouseOriginalPoint))
		{
			this->nextPageButton.click(game);
		}
	}
}

void CharacterPanel::render(Renderer &renderer)
{
	DebugAssert(this->getGame().gameStateIsActive());

	// Clear full screen.
	renderer.clear();

	auto &game = this->getGame();
	auto &textureManager = game.getTextureManager();
	const TextureAssetReference charSheetPaletteTextureAssetRef = CharacterSheetUiView::getPaletteTextureAssetRef();
	const std::optional<PaletteID> charSheetPaletteID = textureManager.tryGetPaletteID(charSheetPaletteTextureAssetRef);
	if (!charSheetPaletteID.has_value())
	{
		DebugLogError("Couldn't get character sheet palette ID \"" + charSheetPaletteTextureAssetRef.filename + "\".");
		return;
	}

	const TextureAssetReference headTextureAssetRef = CharacterSheetUiView::getHeadTextureAssetRef(game);
	const TextureAssetReference bodyTextureAssetRef = CharacterSheetUiView::getBodyTextureAssetRef(game);
	const TextureAssetReference shirtTextureAssetRef = CharacterSheetUiView::getShirtTextureAssetRef(game);
	const TextureAssetReference pantsTextureAssetRef = CharacterSheetUiView::getPantsTextureAssetRef(game);
	const TextureAssetReference statsBackgroundTextureAssetRef = CharacterSheetUiView::getStatsBackgroundTextureAssetRef();
	const TextureAssetReference nextPageTextureAssetRef = CharacterSheetUiView::getNextPageButtonTextureAssetRef();

	const std::optional<TextureBuilderID> headTextureBuilderID = textureManager.tryGetTextureBuilderID(headTextureAssetRef);
	const std::optional<TextureBuilderID> bodyTextureBuilderID = textureManager.tryGetTextureBuilderID(bodyTextureAssetRef);
	const std::optional<TextureBuilderID> shirtTextureBuilderID = textureManager.tryGetTextureBuilderID(shirtTextureAssetRef);
	const std::optional<TextureBuilderID> pantsTextureBuilderID = textureManager.tryGetTextureBuilderID(pantsTextureAssetRef);
	const std::optional<TextureBuilderID> statsBackgroundTextureID = textureManager.tryGetTextureBuilderID(statsBackgroundTextureAssetRef);
	const std::optional<TextureBuilderID> nextPageTextureID = textureManager.tryGetTextureBuilderID(nextPageTextureAssetRef);
	DebugAssert(headTextureBuilderID.has_value());
	DebugAssert(bodyTextureBuilderID.has_value());
	DebugAssert(shirtTextureBuilderID.has_value());
	DebugAssert(pantsTextureBuilderID.has_value());
	DebugAssert(statsBackgroundTextureID.has_value());
	DebugAssert(nextPageTextureID.has_value());

	const int bodyOffsetX = CharacterSheetUiView::getBodyOffsetX(game);
	const Int2 headOffset = CharacterSheetUiView::getHeadOffset(game);
	const Int2 shirtOffset = CharacterSheetUiView::getShirtOffset(game);
	const Int2 pantsOffset = CharacterSheetUiView::getPantsOffset(game);

	// Draw the current portrait and clothes.
	renderer.drawOriginal(*bodyTextureBuilderID, *charSheetPaletteID, bodyOffsetX, 0, textureManager);
	renderer.drawOriginal(*pantsTextureBuilderID, *charSheetPaletteID, pantsOffset.x, pantsOffset.y, textureManager);
	renderer.drawOriginal(*headTextureBuilderID, *charSheetPaletteID, headOffset.x, headOffset.y, textureManager);
	renderer.drawOriginal(*shirtTextureBuilderID, *charSheetPaletteID, shirtOffset.x, shirtOffset.y, textureManager);

	// Draw character stats background.
	renderer.drawOriginal(*statsBackgroundTextureID, *charSheetPaletteID, textureManager);

	// Draw "Next Page" texture.
	renderer.drawOriginal(*nextPageTextureID, *charSheetPaletteID, 108, 179, textureManager);

	// Draw text boxes: player name, race, class.
	const Rect &playerNameTextBoxRect = this->playerNameTextBox.getRect();
	const Rect &playerRaceTextBoxRect = this->playerRaceTextBox.getRect();
	const Rect &playerClassTextBoxRect = this->playerClassTextBox.getRect();
	renderer.drawOriginal(this->playerNameTextBox.getTexture(), playerNameTextBoxRect.getLeft(), playerNameTextBoxRect.getTop());
	renderer.drawOriginal(this->playerRaceTextBox.getTexture(), playerRaceTextBoxRect.getLeft(), playerRaceTextBoxRect.getTop());
	renderer.drawOriginal(this->playerClassTextBox.getTexture(), playerClassTextBoxRect.getLeft(), playerClassTextBoxRect.getTop());
}
