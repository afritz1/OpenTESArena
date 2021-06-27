#include "SDL.h"

#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseRacePanel.h"
#include "TextSubPanel.h"
#include "../Game/Game.h"
#include "../UI/CursorData.h"
#include "../UI/RichTextString.h"

ChooseRacePanel::ChooseRacePanel(Game &game)
	: Panel(game) { }

bool ChooseRacePanel::init()
{
	auto &game = this->getGame();

	this->backToGenderButton = Button<Game&>(CharacterCreationUiController::onBackToChooseGenderButtonSelected);
	this->selectProvinceButton = Button<Game&, int>(CharacterCreationUiController::onChooseRaceProvinceButtonSelected);

	// Push the initial text sub-panel.
	// @todo: allocate std::function for unravelling the map with "push initial parchment sub-panel" on finished,
	// setting the std::function to empty at that time?
	std::unique_ptr<Panel> textSubPanel = ChooseRacePanel::getInitialSubPanel(game);
	game.pushSubPanel(std::move(textSubPanel));

	return true;
}

std::unique_ptr<Panel> ChooseRacePanel::getInitialSubPanel(Game &game)
{
	const auto &fontLibrary = game.getFontLibrary();
	const std::string text = CharacterCreationUiModel::getChooseRaceTitleText(game);
	const TextBox::InitInfo textBoxInitInfo = TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::ChooseRaceInitialPopUpTextCenterPoint,
		CharacterCreationUiView::ChooseRaceInitialPopUpFontName,
		CharacterCreationUiView::ChooseRaceInitialPopUpColor,
		CharacterCreationUiView::ChooseRaceInitialPopUpAlignment,
		std::nullopt,
		CharacterCreationUiView::ChooseRaceInitialPopUpLineSpacing,
		fontLibrary);

	Texture texture = TextureUtils::generate(
		CharacterCreationUiView::ChooseRaceInitialPopUpPatternType,
		CharacterCreationUiView::ChooseRaceInitialPopUpTextureWidth,
		CharacterCreationUiView::ChooseRaceInitialPopUpTextureHeight,
		game.getTextureManager(),
		game.getRenderer());

	std::unique_ptr<TextSubPanel> subPanel = std::make_unique<TextSubPanel>(game);
	if (!subPanel->init(textBoxInitInfo, text, CharacterCreationUiController::onChooseRaceInitialPopUpButtonSelected,
		std::move(texture), CharacterCreationUiView::ChooseRaceInitialPopUpTextureCenterPoint))
	{
		DebugCrash("Couldn't init choose race initial sub-panel.");
	}

	return subPanel;
}

std::optional<CursorData> ChooseRacePanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void ChooseRacePanel::handleEvent(const SDL_Event &e)
{
	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	// Interact with the map screen.
	if (escapePressed)
	{
		this->backToGenderButton.click(game);
	}
	else if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPoint = game.getRenderer().nativeToOriginal(mousePosition);

		// Listen for clicks on the map, checking if the mouse is over a province mask.
		const std::optional<int> provinceID = CharacterCreationUiModel::getChooseRaceProvinceID(game, originalPoint);
		if (provinceID.has_value())
		{
			// Choose the selected province.
			this->selectProvinceButton.click(game, *provinceID);
		}
	}
}

void ChooseRacePanel::drawProvinceTooltip(int provinceID, Renderer &renderer)
{
	auto &game = this->getGame();
	const Texture tooltip = TextureUtils::createTooltip(
		CharacterCreationUiModel::getChooseRaceProvinceTooltipText(game, provinceID),
		this->getGame().getFontLibrary(),
		renderer);

	const auto &inputManager = game.getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);
	const int mouseX = originalPosition.x;
	const int mouseY = originalPosition.y;
	const int x = ((mouseX + 8 + tooltip.getWidth()) < ArenaRenderUtils::SCREEN_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < ArenaRenderUtils::SCREEN_HEIGHT) ?
		mouseY : (mouseY - tooltip.getHeight());

	renderer.drawOriginal(tooltip, x, y);
}

void ChooseRacePanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw background map.
	auto &textureManager = this->getGame().getTextureManager();
	const TextureAssetReference backgroundTextureAssetRef = CharacterCreationUiView::getChooseRaceBackgroundTextureAssetRef();
	const std::optional<PaletteID> backgroundPaletteID = textureManager.tryGetPaletteID(backgroundTextureAssetRef);
	if (!backgroundPaletteID.has_value())
	{
		DebugLogError("Couldn't get race select palette ID for \"" + backgroundTextureAssetRef.filename + "\".");
		return;
	}

	const std::optional<TextureBuilderID> backgroundTextureBuilderID = textureManager.tryGetTextureBuilderID(backgroundTextureAssetRef);
	if (!backgroundTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get race select texture builder ID for \"" + backgroundTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*backgroundTextureBuilderID, *backgroundPaletteID, textureManager);

	// Cover up the "exit" text at the bottom right.
	const TextureAssetReference noExitTextureAssetRef = CharacterCreationUiView::getChooseRaceNoExitTextureAssetRef();
	const std::optional<TextureBuilderID> noExitTextureBuilderID = textureManager.tryGetTextureBuilderID(noExitTextureAssetRef);
	if (!noExitTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get exit cover texture builder ID for \"" + noExitTextureAssetRef.filename + "\".");
		return;
	}

	const TextureBuilder &noExitTextureBuilder = textureManager.getTextureBuilderHandle(*noExitTextureBuilderID);
	const int exitCoverX = CharacterCreationUiView::getChooseRaceNoExitTextureX(noExitTextureBuilder.getWidth());
	const int exitCoverY = CharacterCreationUiView::getChooseRaceNoExitTextureY(noExitTextureBuilder.getHeight());
	renderer.drawOriginal(*noExitTextureBuilderID, *backgroundPaletteID, exitCoverX, exitCoverY, textureManager);
}

void ChooseRacePanel::renderSecondary(Renderer &renderer)
{
	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();

	// Draw hovered province tooltip.
	const Int2 originalPoint = game.getRenderer().nativeToOriginal(mousePosition);

	// Draw tooltip if the mouse is in a province.
	const std::optional<int> provinceID = CharacterCreationUiModel::getChooseRaceProvinceID(game, originalPoint);
	if (provinceID.has_value())
	{
		this->drawProvinceTooltip(*provinceID, renderer);
	}
}
