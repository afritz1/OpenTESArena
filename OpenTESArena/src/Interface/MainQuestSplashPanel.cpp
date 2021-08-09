#include "MainQuestSplashPanel.h"
#include "MainQuestSplashUiController.h"
#include "MainQuestSplashUiModel.h"
#include "MainQuestSplashUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../UI/CursorData.h"

#include "components/utilities/String.h"

MainQuestSplashPanel::MainQuestSplashPanel(Game &game)
	: Panel(game) { }

bool MainQuestSplashPanel::init(int provinceID)
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	const auto &fontLibrary = game.getFontLibrary();

	const std::string descriptionText = MainQuestSplashUiModel::getDungeonText(game, provinceID);
	const TextBox::InitInfo descriptionTextBoxInitInfo =
		MainQuestSplashUiView::getDescriptionTextBoxInitInfo(descriptionText, fontLibrary);
	if (!this->textBox.init(descriptionTextBoxInitInfo, descriptionText, renderer))
	{
		DebugLogError("Couldn't init description text box.");
		return false;
	}

	this->exitButton = Button<Game&>(
		MainQuestSplashUiView::ExitButtonX,
		MainQuestSplashUiView::ExitButtonY,
		MainQuestSplashUiView::ExitButtonWidth,
		MainQuestSplashUiView::ExitButtonHeight,
		MainQuestSplashUiController::onExitButtonSelected);

	this->addButtonProxy(MouseButtonType::Left, this->exitButton.getRect(),
		[this, &game]() { this->exitButton.click(game); });

	this->addInputActionListener(InputActionName::Back,
		[this, &game](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->exitButton.click(game);
		}
	});

	// Get the texture filename of the staff dungeon splash image.
	this->splashTextureAssetRef = TextureAssetReference(MainQuestSplashUiModel::getSplashFilename(game, provinceID));

	return true;
}

std::optional<CursorData> MainQuestSplashPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void MainQuestSplashPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw staff dungeon splash image.
	auto &textureManager = this->getGame().getTextureManager();
	const TextureAssetReference &paletteTextureAssetRef = this->splashTextureAssetRef;
	const std::optional<PaletteID> splashPaletteID = textureManager.tryGetPaletteID(paletteTextureAssetRef);
	if (!splashPaletteID.has_value())
	{
		DebugLogError("Couldn't get splash palette ID for \"" + paletteTextureAssetRef.filename + "\".");
		return;
	}

	const TextureAssetReference &textureAssetRef = this->splashTextureAssetRef;
	const std::optional<TextureBuilderID> splashTextureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!splashTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get splash texture builder ID for \"" + textureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*splashTextureBuilderID, *splashPaletteID, textureManager);

	// Draw text.
	const Rect &textBoxRect = this->textBox.getRect();
	renderer.drawOriginal(this->textBox.getTexture(), textBoxRect.getLeft(), textBoxRect.getTop());
}
