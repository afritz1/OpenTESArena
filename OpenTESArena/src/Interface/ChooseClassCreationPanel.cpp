#include "SDL.h"

#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseClassCreationPanel.h"
#include "ChooseClassPanel.h"
#include "MainMenuPanel.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ExeData.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Input/InputActionName.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/CursorData.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"

#include "components/utilities/String.h"

ChooseClassCreationPanel::ChooseClassCreationPanel(Game &game)
	: Panel(game) { }

bool ChooseClassCreationPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();

	this->parchment = TextureUtils::generate(
		ChooseClassCreationUiView::PopUpPatternType,
		ChooseClassCreationUiView::PopUpTextureWidth,
		ChooseClassCreationUiView::PopUpTextureHeight,
		game.getTextureManager(),
		renderer);

	const auto &fontLibrary = game.getFontLibrary();
	const std::string titleText = ChooseClassCreationUiModel::getTitleText(game);
	const TextBox::InitInfo titleTextBoxInitInfo =
		ChooseClassCreationUiView::getTitleTextBoxInitInfo(titleText, fontLibrary);
	if (!this->titleTextBox.init(titleTextBoxInitInfo, titleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	const std::string generateText = ChooseClassCreationUiModel::getGenerateButtonText(game);
	const TextBox::InitInfo generateTextBoxInitInfo =
		ChooseClassCreationUiView::getGenerateTextBoxInitInfo(generateText, fontLibrary);
	if (!this->generateTextBox.init(generateTextBoxInitInfo, generateText, renderer))
	{
		DebugLogError("Couldn't init generate class text box.");
		return false;
	}

	const std::string selectText = ChooseClassCreationUiModel::getSelectButtonText(game);
	const TextBox::InitInfo selectTextBoxInitInfo =
		ChooseClassCreationUiView::getSelectTextBoxInitInfo(selectText, fontLibrary);
	if (!this->selectTextBox.init(selectTextBoxInitInfo, selectText, renderer))
	{
		DebugLogError("Couldn't init select class text box.");
		return false;
	}

	this->generateButton = Button<Game&>(
		ChooseClassCreationUiView::GenerateButtonCenterPoint,
		ChooseClassCreationUiView::GenerateButtonWidth,
		ChooseClassCreationUiView::GenerateButtonHeight,
		ChooseClassCreationUiController::onGenerateButtonSelected);
	this->selectButton = Button<Game&>(
		ChooseClassCreationUiView::SelectButtonCenterPoint,
		ChooseClassCreationUiView::SelectButtonWidth,
		ChooseClassCreationUiView::SelectButtonHeight,
		ChooseClassCreationUiController::onSelectButtonSelected);

	this->addButtonProxy(MouseButtonType::Left, this->generateButton.getRect(),
		[this, &game]() { this->generateButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->selectButton.getRect(),
		[this, &game]() { this->selectButton.click(game); });

	this->addInputActionListener(InputActionName::Back, ChooseClassCreationUiController::onBackToMainMenuInputAction);

	return true;
}

std::optional<CursorData> ChooseClassCreationPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void ChooseClassCreationPanel::drawTooltip(const std::string &text, Renderer &renderer)
{
	const Texture tooltip = TextureUtils::createTooltip(text, this->getGame().getFontLibrary(), renderer);

	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);
	const int mouseX = originalPosition.x;
	const int mouseY = originalPosition.y;
	const int x = ((mouseX + 8 + tooltip.getWidth()) < ArenaRenderUtils::SCREEN_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < ArenaRenderUtils::SCREEN_HEIGHT) ?
		(mouseY - 1) : (mouseY - tooltip.getHeight());

	renderer.drawOriginal(tooltip, x, y);
}

void ChooseClassCreationPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw background.
	auto &textureManager = this->getGame().getTextureManager();
	const TextureAssetReference backgroundTextureAssetRef = CharacterCreationUiView::getNightSkyTextureAssetRef();
	const std::optional<PaletteID> backgroundPaletteID = textureManager.tryGetPaletteID(backgroundTextureAssetRef);
	if (!backgroundPaletteID.has_value())
	{
		DebugLogError("Couldn't get background palette ID for \"" + backgroundTextureAssetRef.filename + "\".");
		return;
	}

	const std::optional<TextureBuilderID> backgroundTextureBuilderID = textureManager.tryGetTextureBuilderID(backgroundTextureAssetRef);
	if (!backgroundTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get background texture builder ID for \"" + backgroundTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*backgroundTextureBuilderID, *backgroundPaletteID, textureManager);

	// Draw parchments: title, generate, select.
	const int parchmentX = ChooseClassCreationUiView::getTitleTextureX(this->parchment.getWidth());
	const int parchmentY = ChooseClassCreationUiView::getTitleTextureY(this->parchment.getHeight());
	renderer.drawOriginal(this->parchment, parchmentX, parchmentY - 20);
	renderer.drawOriginal(this->parchment, parchmentX, parchmentY + 20);
	renderer.drawOriginal(this->parchment, parchmentX, parchmentY + 60);

	// Draw text: title, generate, select.
	const Rect &titleTextBoxRect = this->titleTextBox.getRect();
	const Rect &generateTextBoxRect = this->generateTextBox.getRect();
	const Rect &selectTextBoxRect = this->selectTextBox.getRect();
	renderer.drawOriginal(this->titleTextBox.getTexture(), titleTextBoxRect.getLeft(), titleTextBoxRect.getTop());
	renderer.drawOriginal(this->generateTextBox.getTexture(), generateTextBoxRect.getLeft(), generateTextBoxRect.getTop());
	renderer.drawOriginal(this->selectTextBox.getTexture(), selectTextBoxRect.getLeft(), selectTextBoxRect.getTop());

	// Check if the mouse is hovered over one of the boxes for tooltips.
	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPoint = renderer.nativeToOriginal(mousePosition);

	if (this->generateButton.contains(originalPoint))
	{
		this->drawTooltip(ChooseClassCreationUiModel::getGenerateButtonTooltipText(), renderer);
	}
	else if (this->selectButton.contains(originalPoint))
	{
		this->drawTooltip(ChooseClassCreationUiModel::getSelectButtonTooltipText(), renderer);
	}
}
