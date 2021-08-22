#include "SDL.h"

#include "LoadSavePanel.h"
#include "LoadSaveUiController.h"
#include "LoadSaveUiModel.h"
#include "LoadSaveUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../UI/CursorData.h"

LoadSavePanel::LoadSavePanel(Game &game)
	: Panel(game) { }

bool LoadSavePanel::init(LoadSavePanel::Type type)
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	const auto &fontLibrary = game.getFontLibrary();

	// Populate save slots.
	const std::vector<LoadSaveUiModel::Entry> entries = LoadSaveUiModel::getSaveEntries(game);
	for (int i = 0; i < static_cast<int>(entries.size()); i++)
	{
		const LoadSaveUiModel::Entry &entry = entries[i];
		const std::string &text = entry.displayText;
		const TextBox::InitInfo textBoxInitInfo = TextBox::InitInfo::makeWithCenter(
			text,
			LoadSaveUiView::getEntryCenterPoint(i),
			LoadSaveUiView::EntryFontName,
			LoadSaveUiView::EntryTextColor,
			LoadSaveUiView::EntryTextAlignment,
			fontLibrary);

		TextBox textBox;
		if (!textBox.init(textBoxInitInfo, text, renderer))
		{
			DebugLogError("Couldn't init load/save text box " + std::to_string(i) + ".");
			continue;
		}

		this->saveTextBoxes.emplace_back(std::move(textBox));
	}

	// Each save slot is a button proxy with its own callback based on its index.
	for (int i = 0; i < LoadSaveUiModel::SlotCount; i++)
	{
		const Rect slotRect = LoadSaveUiModel::getSlotRect(i);
		this->addButtonProxy(MouseButtonType::Left, slotRect,
			[this, &game, i]() { LoadSaveUiController::onEntryButtonSelected(game, i); });
	}

	this->addInputActionListener(InputActionName::Back, LoadSaveUiController::onBackInputAction);

	this->type = type;

	return true;
}

std::optional<CursorData> LoadSavePanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void LoadSavePanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw slots background.
	auto &textureManager = this->getGame().getTextureManager();
	const TextureAssetReference paletteTextureAssetRef = LoadSaveUiView::getPaletteTextureAssetRef();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteTextureAssetRef);
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for \"" + paletteTextureAssetRef.filename + "\".");
		return;
	}

	const TextureAssetReference loadSaveTextureAssetRef = LoadSaveUiView::getLoadSaveTextureAssetRef();
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(loadSaveTextureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugLogError("Couldn't get texture builder ID for \"" + loadSaveTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*textureBuilderID, *paletteID, textureManager);

	// Draw save text.
	for (TextBox &textBox : this->saveTextBoxes)
	{
		const Rect &textBoxRect = textBox.getRect();
		renderer.drawOriginal(textBox.getTextureID(), textBoxRect.getLeft(), textBoxRect.getTop());
	}
}
