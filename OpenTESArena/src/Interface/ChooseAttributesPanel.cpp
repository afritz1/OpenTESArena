#include <algorithm>
#include <array>

#include "SDL.h"

#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "CharacterSheetUiController.h"
#include "CharacterSheetUiView.h"
#include "ChooseAttributesPanel.h"
#include "TextSubPanel.h"
#include "../Game/Game.h"
#include "../UI/RichTextString.h"

ChooseAttributesPanel::ChooseAttributesPanel(Game &game)
	: Panel(game)
{
	this->nameTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			CharacterCreationUiModel::getPlayerName(game),
			CharacterSheetUiView::PlayerNameTextBoxFontName,
			CharacterSheetUiView::PlayerNameTextBoxColor,
			CharacterSheetUiView::PlayerNameTextBoxAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			CharacterSheetUiView::PlayerNameTextBoxX,
			CharacterSheetUiView::PlayerNameTextBoxY,
			richText,
			fontLibrary,
			game.getRenderer());
	}();

	this->raceTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			CharacterCreationUiModel::getPlayerRaceName(game),
			CharacterSheetUiView::PlayerRaceTextBoxFontName,
			CharacterSheetUiView::PlayerRaceTextBoxColor,
			CharacterSheetUiView::PlayerRaceTextBoxAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			CharacterSheetUiView::PlayerRaceTextBoxX,
			CharacterSheetUiView::PlayerRaceTextBoxY,
			richText,
			fontLibrary,
			game.getRenderer());
	}();

	this->classTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			CharacterCreationUiModel::getPlayerClassName(game),
			CharacterSheetUiView::PlayerClassTextBoxFontName,
			CharacterSheetUiView::PlayerClassTextBoxColor,
			CharacterSheetUiView::PlayerClassTextBoxAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			CharacterSheetUiView::PlayerClassTextBoxX,
			CharacterSheetUiView::PlayerClassTextBoxY,
			richText,
			fontLibrary,
			game.getRenderer());
	}();

	this->backToRaceButton = []()
	{
		return Button<Game&>(CharacterCreationUiController::onBackToRaceSelectionButtonSelected);
	}();

	this->doneButton = []()
	{
		return Button<Game&, bool*>(
			CharacterSheetUiView::DoneButtonCenterPoint,
			CharacterSheetUiView::DoneButtonWidth,
			CharacterSheetUiView::DoneButtonHeight,
			CharacterCreationUiController::onAttributesDoneButtonSelected);
	}();

	this->portraitButton = []()
	{
		return Button<Game&, bool>(
			CharacterCreationUiView::AppearancePortraitButtonCenterPoint,
			CharacterCreationUiView::AppearancePortraitButtonWidth,
			CharacterCreationUiView::AppearancePortraitButtonHeight,
			CharacterCreationUiController::onAppearancePortraitButtonSelected);
	}();

	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.setPortraitIndex(0);

	this->attributesAreSaved = false;

	// Push the initial text pop-up onto the sub-panel stack.
	std::unique_ptr<Panel> textSubPanel = [&game]()
	{
		const RichTextString richText(
			CharacterCreationUiModel::getChooseAttributesText(game),
			CharacterCreationUiView::ChooseAttributesTextFontName,
			CharacterCreationUiView::ChooseAttributesTextColor,
			CharacterCreationUiView::ChooseAttributesTextAlignment,
			CharacterCreationUiView::ChooseAttributesTextLineSpacing,
			game.getFontLibrary());

		Texture texture = TextureUtils::generate(
			CharacterCreationUiView::ChooseAttributesTextPatternType,
			CharacterCreationUiView::getChooseAttributesTextureWidth(),
			CharacterCreationUiView::getChooseAttributesTextureHeight(),
			game.getTextureManager(),
			game.getRenderer());

		return std::make_unique<TextSubPanel>(
			game,
			CharacterCreationUiView::ChooseAttributesTextCenterPoint,
			richText,
			CharacterCreationUiController::onChooseAttributesPopUpSelected,
			std::move(texture),
			CharacterCreationUiView::ChooseAttributesTextureCenterPoint);
	}();

	game.pushSubPanel(std::move(textSubPanel));
}

std::optional<Panel::CursorData> ChooseAttributesPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void ChooseAttributesPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);

	if (escapePressed)
	{
		this->backToRaceButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);
		
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 mouseOriginalPoint = this->getGame().getRenderer()
		.nativeToOriginal(mousePosition);

	if (leftClick)
	{
		if (this->doneButton.contains(mouseOriginalPoint))
		{
			this->doneButton.click(this->getGame(), &this->attributesAreSaved);
		}
		else if (this->portraitButton.contains(mouseOriginalPoint) && this->attributesAreSaved)
		{
			// Pass 'true' to increment the portrait ID.
			this->portraitButton.click(this->getGame(), true);
		}
	}

	if (rightClick)
	{
		if (this->portraitButton.contains(mouseOriginalPoint) && this->attributesAreSaved)
		{
			// Pass 'false' to decrement the portrait ID.
			this->portraitButton.click(this->getGame(), false);
		}
	}	
}

void ChooseAttributesPanel::render(Renderer &renderer)
{
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

	const TextureAssetReference headTextureAssetRef = CharacterCreationUiView::getHeadTextureAssetRef(game);
	const TextureAssetReference bodyTextureAssetRef = CharacterCreationUiView::getBodyTextureAssetRef(game);
	const TextureAssetReference shirtTextureAssetRef = CharacterCreationUiView::getShirtTextureAssetRef(game);
	const TextureAssetReference pantsTextureAssetRef = CharacterCreationUiView::getPantsTextureAssetRef(game);
	const TextureAssetReference statsBackgroundTextureAssetRef = CharacterSheetUiView::getStatsBackgroundTextureAssetRef();
	const std::optional<TextureBuilderID> headTextureBuilderID = textureManager.tryGetTextureBuilderID(headTextureAssetRef);
	const std::optional<TextureBuilderID> bodyTextureBuilderID = textureManager.tryGetTextureBuilderID(bodyTextureAssetRef);
	const std::optional<TextureBuilderID> shirtTextureBuilderID = textureManager.tryGetTextureBuilderID(shirtTextureAssetRef);
	const std::optional<TextureBuilderID> pantsTextureBuilderID = textureManager.tryGetTextureBuilderID(pantsTextureAssetRef);
	const std::optional<TextureBuilderID> statsBackgroundTextureID = textureManager.tryGetTextureBuilderID(statsBackgroundTextureAssetRef);
	DebugAssert(headTextureBuilderID.has_value());
	DebugAssert(bodyTextureBuilderID.has_value());
	DebugAssert(shirtTextureBuilderID.has_value());
	DebugAssert(pantsTextureBuilderID.has_value());
	DebugAssert(statsBackgroundTextureID.has_value());

	const int bodyOffsetX = CharacterCreationUiView::getBodyOffsetX(game);
	const Int2 headOffset = CharacterCreationUiView::getHeadOffset(game);
	const Int2 shirtOffset = CharacterCreationUiView::getShirtOffset(game);
	const Int2 pantsOffset = CharacterCreationUiView::getPantsOffset(game);

	// Draw the current portrait and clothes.
	renderer.drawOriginal(*bodyTextureBuilderID, *charSheetPaletteID, bodyOffsetX, 0, textureManager);
	renderer.drawOriginal(*pantsTextureBuilderID, *charSheetPaletteID, pantsOffset.x, pantsOffset.y, textureManager);
	renderer.drawOriginal(*headTextureBuilderID, *charSheetPaletteID, headOffset.x, headOffset.y, textureManager);
	renderer.drawOriginal(*shirtTextureBuilderID, *charSheetPaletteID, shirtOffset.x, shirtOffset.y, textureManager);

	// Draw attributes texture.
	renderer.drawOriginal(*statsBackgroundTextureID, *charSheetPaletteID, textureManager);

	// Draw text boxes: player name, race, class.
	renderer.drawOriginal(this->nameTextBox->getTexture(), this->nameTextBox->getX(), this->nameTextBox->getY());
	renderer.drawOriginal(this->raceTextBox->getTexture(), this->raceTextBox->getX(), this->raceTextBox->getY());
	renderer.drawOriginal(this->classTextBox->getTexture(), this->classTextBox->getX(), this->classTextBox->getY());
}
