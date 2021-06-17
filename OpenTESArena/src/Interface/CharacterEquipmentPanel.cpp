#include "SDL.h"

#include "CharacterEquipmentPanel.h"
#include "CharacterSheetUiController.h"
#include "CharacterSheetUiModel.h"
#include "CharacterSheetUiView.h"
#include "InventoryUiModel.h"
#include "InventoryUiView.h"
#include "../Game/Game.h"
#include "../UI/CursorData.h"

#include "components/debug/Debug.h"

CharacterEquipmentPanel::CharacterEquipmentPanel(Game &game)
	: Panel(game) { }

bool CharacterEquipmentPanel::init()
{
	auto &game = this->getGame();

	this->playerNameTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			CharacterSheetUiModel::getPlayerName(game),
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

	this->playerRaceTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			CharacterSheetUiModel::getPlayerRaceName(game),
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

	this->playerClassTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			CharacterSheetUiModel::getPlayerClassName(game),
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

	Buffer<InventoryUiModel::ItemUiDefinition> itemUiDefs = InventoryUiModel::getPlayerInventoryItems(game);
	std::vector<std::pair<std::string, Color>> elements;
	for (int i = 0; i < itemUiDefs.getCount(); i++)
	{
		const InventoryUiModel::ItemUiDefinition &itemUiDef = itemUiDefs.get(i);
		elements.emplace_back(std::make_pair(std::move(itemUiDef.text), itemUiDef.color));
	}

	this->inventoryListBox.init(InventoryUiView::PlayerInventoryRect,
		InventoryUiView::makePlayerInventoryListBoxProperties(game.getFontLibrary()), game.getRenderer());
	for (int i = 0; i < static_cast<int>(elements.size()); i++)
	{
		auto &pair = elements[i];
		this->inventoryListBox.add(std::move(pair.first));
		this->inventoryListBox.setOverrideColor(i, pair.second);
	}

	this->backToStatsButton = Button<Game&>(
		CharacterSheetUiView::BackToStatsButtonX,
		CharacterSheetUiView::BackToStatsButtonY,
		CharacterSheetUiView::BackToStatsButtonWidth,
		CharacterSheetUiView::BackToStatsButtonHeight,
		CharacterSheetUiController::onBackToStatsButtonSelected);
	this->spellbookButton = Button<Game&>(
		CharacterSheetUiView::SpellbookButtonX,
		CharacterSheetUiView::SpellbookButtonY,
		CharacterSheetUiView::SpellbookButtonWidth,
		CharacterSheetUiView::SpellbookButtonHeight,
		CharacterSheetUiController::onSpellbookButtonSelected);
	this->dropButton = Button<Game&, int>(
		CharacterSheetUiView::DropButtonX,
		CharacterSheetUiView::DropButtonY,
		CharacterSheetUiView::DropButtonWidth,
		CharacterSheetUiView::DropButtonHeight,
		CharacterSheetUiController::onDropButtonSelected);
	this->scrollDownButton = Button<ListBox&>(
		CharacterSheetUiView::ScrollDownButtonCenterPoint,
		CharacterSheetUiView::ScrollDownButtonWidth,
		CharacterSheetUiView::ScrollDownButtonHeight,
		CharacterSheetUiController::onInventoryScrollDownButtonSelected);
	this->scrollUpButton = Button<ListBox&>(
		CharacterSheetUiView::ScrollUpButtonCenterPoint,
		CharacterSheetUiView::ScrollUpButtonWidth,
		CharacterSheetUiView::ScrollUpButtonHeight,
		CharacterSheetUiController::onInventoryScrollUpButtonSelected);

	return true;
}

std::optional<CursorData> CharacterEquipmentPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void CharacterEquipmentPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool tabPressed = inputManager.keyPressed(e, SDLK_TAB);

	if (escapePressed || tabPressed)
	{
		this->backToStatsButton.click(this->getGame());
	}

	const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	const bool mouseWheeledUp = inputManager.mouseWheeledUp(e);
	const bool mouseWheeledDown = inputManager.mouseWheeledDown(e);

	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 mouseOriginalPoint = this->getGame().getRenderer().nativeToOriginal(mousePosition);

	if (leftClick)
	{
		if (this->backToStatsButton.contains(mouseOriginalPoint))
		{
			this->backToStatsButton.click(this->getGame());
		}
		else if (this->spellbookButton.contains(mouseOriginalPoint))
		{
			this->spellbookButton.click(this->getGame());
		}
		else if (this->dropButton.contains(mouseOriginalPoint))
		{
			// Eventually give the index of the clicked item instead.
			this->dropButton.click(this->getGame(), 0);
		}
		else if (this->scrollUpButton.contains(mouseOriginalPoint))
		{
			this->scrollUpButton.click(this->inventoryListBox);
		}
		else if (this->scrollDownButton.contains(mouseOriginalPoint))
		{
			this->scrollDownButton.click(this->inventoryListBox);
		}
	}
	else if (mouseWheeledUp)
	{
		this->scrollUpButton.click(this->inventoryListBox);
	}
	else if (mouseWheeledDown)
	{
		this->scrollDownButton.click(this->inventoryListBox);
	}
}

void CharacterEquipmentPanel::render(Renderer &renderer)
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
	const TextureAssetReference equipmentBgTextureAssetRef = CharacterSheetUiView::getEquipmentBackgroundTextureAssetRef();

	const std::optional<TextureBuilderID> headTextureBuilderID = textureManager.tryGetTextureBuilderID(headTextureAssetRef);
	const std::optional<TextureBuilderID> bodyTextureBuilderID = textureManager.tryGetTextureBuilderID(bodyTextureAssetRef);
	const std::optional<TextureBuilderID> shirtTextureBuilderID = textureManager.tryGetTextureBuilderID(shirtTextureAssetRef);
	const std::optional<TextureBuilderID> pantsTextureBuilderID = textureManager.tryGetTextureBuilderID(pantsTextureAssetRef);
	const std::optional<TextureBuilderID> equipmentBgTextureBuilderID = textureManager.tryGetTextureBuilderID(equipmentBgTextureAssetRef);
	DebugAssert(headTextureBuilderID.has_value());
	DebugAssert(bodyTextureBuilderID.has_value());
	DebugAssert(shirtTextureBuilderID.has_value());
	DebugAssert(pantsTextureBuilderID.has_value());
	DebugAssert(equipmentBgTextureBuilderID.has_value());

	const int bodyOffsetX = CharacterSheetUiView::getBodyOffsetX(game);
	const Int2 headOffset = CharacterSheetUiView::getHeadOffset(game);
	const Int2 shirtOffset = CharacterSheetUiView::getShirtOffset(game);
	const Int2 pantsOffset = CharacterSheetUiView::getPantsOffset(game);

	// Draw the current portrait and clothes.
	renderer.drawOriginal(*bodyTextureBuilderID, *charSheetPaletteID, bodyOffsetX, 0, textureManager);
	renderer.drawOriginal(*pantsTextureBuilderID, *charSheetPaletteID, pantsOffset.x, pantsOffset.y, textureManager);
	renderer.drawOriginal(*headTextureBuilderID, *charSheetPaletteID, headOffset.x, headOffset.y, textureManager);
	renderer.drawOriginal(*shirtTextureBuilderID, *charSheetPaletteID, shirtOffset.x, shirtOffset.y, textureManager);

	// Draw character equipment background.
	renderer.drawOriginal(*equipmentBgTextureBuilderID, *charSheetPaletteID, textureManager);

	// Draw text boxes: player name, race, class.
	renderer.drawOriginal(this->playerNameTextBox->getTexture(), this->playerNameTextBox->getX(), this->playerNameTextBox->getY());
	renderer.drawOriginal(this->playerRaceTextBox->getTexture(), this->playerRaceTextBox->getX(), this->playerRaceTextBox->getY());
	renderer.drawOriginal(this->playerClassTextBox->getTexture(), this->playerClassTextBox->getX(), this->playerClassTextBox->getY());
	
	// Draw inventory list box.
	const Rect &inventoryListBoxRect = this->inventoryListBox.getRect();
	renderer.drawOriginal(this->inventoryListBox.getTexture(), inventoryListBoxRect.getLeft(), inventoryListBoxRect.getTop());
}
