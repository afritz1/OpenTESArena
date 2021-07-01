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
		CharacterCreationUiView::ChooseClassCreationPopUpPatternType,
		CharacterCreationUiView::ChooseClassCreationPopUpTextureWidth,
		CharacterCreationUiView::ChooseClassCreationPopUpTextureHeight,
		game.getTextureManager(),
		renderer);

	const auto &fontLibrary = game.getFontLibrary();
	const std::string titleText = CharacterCreationUiModel::getChooseClassCreationTitleText(game);
	const TextBox::InitInfo titleTextBoxInitInfo =
		CharacterCreationUiView::getChooseClassCreationTitleTextBoxInitInfo(titleText, fontLibrary);
	if (!this->titleTextBox.init(titleTextBoxInitInfo, titleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	const std::string generateText = CharacterCreationUiModel::getGenerateClassButtonText(game);
	const TextBox::InitInfo generateTextBoxInitInfo =
		CharacterCreationUiView::getChooseClassCreationGenerateTextBoxInitInfo(generateText, fontLibrary);
	if (!this->generateTextBox.init(generateTextBoxInitInfo, generateText, renderer))
	{
		DebugLogError("Couldn't init generate class text box.");
		return false;
	}

	const std::string selectText = CharacterCreationUiModel::getSelectClassButtonText(game);
	const TextBox::InitInfo selectTextBoxInitInfo =
		CharacterCreationUiView::getChooseClassCreationSelectTextBoxInitInfo(selectText, fontLibrary);
	if (!this->selectTextBox.init(selectTextBoxInitInfo, selectText, renderer))
	{
		DebugLogError("Couldn't init select class text box.");
		return false;
	}

	this->backToMainMenuButton = Button<Game&>(CharacterCreationUiController::onBackToMainMenuButtonSelected);
	this->generateButton = Button<Game&>(
		CharacterCreationUiView::GenerateClassButtonCenterPoint,
		CharacterCreationUiView::GenerateClassButtonWidth,
		CharacterCreationUiView::GenerateClassButtonHeight,
		CharacterCreationUiController::onGenerateClassButtonSelected);
	this->selectButton = Button<Game&>(
		CharacterCreationUiView::SelectClassButtonCenterPoint,
		CharacterCreationUiView::SelectClassButtonWidth,
		CharacterCreationUiView::SelectClassButtonHeight,
		CharacterCreationUiController::onSelectClassButtonSelected);

	return true;
}

std::optional<CursorData> ChooseClassCreationPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void ChooseClassCreationPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);

	if (escapePressed)
	{
		this->backToMainMenuButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		if (this->generateButton.contains(mouseOriginalPoint))
		{
			this->generateButton.click(this->getGame());
		}
		else if (this->selectButton.contains(mouseOriginalPoint))
		{
			this->selectButton.click(this->getGame());
		}
	}
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
	const int parchmentX = CharacterCreationUiView::getChooseClassCreationTitleTextureX(this->parchment.getWidth());
	const int parchmentY = CharacterCreationUiView::getChooseClassCreationTitleTextureY(this->parchment.getHeight());
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
		this->drawTooltip(CharacterCreationUiModel::getGenerateClassButtonTooltipText(), renderer);
	}
	else if (this->selectButton.contains(originalPoint))
	{
		this->drawTooltip(CharacterCreationUiModel::getSelectClassButtonTooltipText(), renderer);
	}
}
