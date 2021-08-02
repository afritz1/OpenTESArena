#include "SDL.h"

#include "CharacterEquipmentPanel.h"
#include "CharacterSheetUiController.h"
#include "CharacterSheetUiModel.h"
#include "CharacterSheetUiView.h"
#include "InventoryUiModel.h"
#include "InventoryUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../UI/CursorData.h"

#include "components/debug/Debug.h"

CharacterEquipmentPanel::CharacterEquipmentPanel(Game &game)
	: Panel(game) { }

bool CharacterEquipmentPanel::init()
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

	this->addButtonProxy(MouseButtonType::Left, this->backToStatsButton.getRect(),
		[this, &game]() { this->backToStatsButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->spellbookButton.getRect(),
		[this, &game]() { this->spellbookButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->dropButton.getRect(),
		[this, &game]()
	{
		// @todo: give the index of the clicked item instead.
		const int itemIndex = 0;
		this->dropButton.click(game, itemIndex);
	});

	this->addButtonProxy(MouseButtonType::Left, this->scrollDownButton.getRect(),
		[this, &game]() { this->scrollDownButton.click(this->inventoryListBox); });
	this->addButtonProxy(MouseButtonType::Left, this->scrollUpButton.getRect(),
		[this, &game]() { this->scrollUpButton.click(this->inventoryListBox); });

	auto backToStatsInputActionFunc = CharacterSheetUiController::onBackToStatsInputAction;
	this->addInputActionListener(InputActionName::Back, backToStatsInputActionFunc);
	this->addInputActionListener(InputActionName::CharacterSheet, backToStatsInputActionFunc); // @todo: make sure input action map is active

	this->addMouseScrollChangedListener([this](Game &game, MouseWheelScrollType type, const Int2 &position)
	{
		if (type == MouseWheelScrollType::Down)
		{
			this->scrollDownButton.click(this->inventoryListBox);
		}
		else if (type == MouseWheelScrollType::Up)
		{
			this->scrollUpButton.click(this->inventoryListBox);
		}
	});

	return true;
}

std::optional<CursorData> CharacterEquipmentPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
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
	const Rect &playerNameTextBoxRect = this->playerNameTextBox.getRect();
	const Rect &playerRaceTextBoxRect = this->playerRaceTextBox.getRect();
	const Rect &playerClassTextBoxRect = this->playerClassTextBox.getRect();
	renderer.drawOriginal(this->playerNameTextBox.getTexture(), playerNameTextBoxRect.getLeft(), playerNameTextBoxRect.getTop());
	renderer.drawOriginal(this->playerRaceTextBox.getTexture(), playerRaceTextBoxRect.getLeft(), playerRaceTextBoxRect.getTop());
	renderer.drawOriginal(this->playerClassTextBox.getTexture(), playerClassTextBoxRect.getLeft(), playerClassTextBoxRect.getTop());

	// Draw inventory list box.
	const Rect &inventoryListBoxRect = this->inventoryListBox.getRect();
	renderer.drawOriginal(this->inventoryListBox.getTexture(), inventoryListBoxRect.getLeft(), inventoryListBoxRect.getTop());
}
