#include "SDL.h"

#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseGenderPanel.h"
#include "ChooseNamePanel.h"
#include "ChooseRacePanel.h"
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
#include "../UI/FontLibrary.h"
#include "../UI/FontName.h"
#include "../UI/RichTextString.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"

ChooseGenderPanel::ChooseGenderPanel(Game &game)
	: Panel(game)
{
	this->parchment = TextureUtils::generate(
		CharacterCreationUiView::ChooseGenderTexturePatternType,
		CharacterCreationUiView::ChooseGenderTextureWidth,
		CharacterCreationUiView::ChooseGenderTextureHeight,
		game.getTextureManager(),
		game.getRenderer());

	this->genderTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			CharacterCreationUiModel::getChooseGenderTitleText(game),
			CharacterCreationUiView::ChooseGenderTitleFontName,
			CharacterCreationUiView::ChooseGenderTitleColor,
			CharacterCreationUiView::ChooseGenderTitleAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			CharacterCreationUiView::ChooseGenderTitleCenterPoint,
			richText,
			fontLibrary,
			game.getRenderer());
	}();

	this->maleTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			CharacterCreationUiModel::getChooseGenderMaleText(game),
			CharacterCreationUiView::ChooseGenderMaleFontName,
			CharacterCreationUiView::ChooseGenderMaleColor,
			CharacterCreationUiView::ChooseGenderMaleAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			CharacterCreationUiView::ChooseGenderMaleTextBoxCenter,
			richText,
			fontLibrary,
			game.getRenderer());
	}();

	this->femaleTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			CharacterCreationUiModel::getChooseGenderFemaleText(game),
			CharacterCreationUiView::ChooseGenderFemaleFontName,
			CharacterCreationUiView::ChooseGenderFemaleColor,
			CharacterCreationUiView::ChooseGenderFemaleAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			CharacterCreationUiView::ChooseGenderFemaleTextBoxCenter,
			richText,
			fontLibrary,
			game.getRenderer());
	}();

	this->backToNameButton = Button<Game&>(CharacterCreationUiController::onBackToChooseNameButtonSelected);
	this->maleButton = Button<Game&>(
		CharacterCreationUiView::ChooseGenderMaleButtonCenter,
		CharacterCreationUiView::ChooseGenderMaleButtonWidth,
		CharacterCreationUiView::ChooseGenderMaleButtonHeight,
		CharacterCreationUiController::onChooseGenderMaleButtonSelected);
	this->femaleButton = Button<Game&>(
		CharacterCreationUiView::ChooseGenderFemaleButtonCenter,
		CharacterCreationUiView::ChooseGenderFemaleButtonWidth,
		CharacterCreationUiView::ChooseGenderFemaleButtonHeight,
		CharacterCreationUiController::onChooseGenderFemaleButtonSelected);
}

std::optional<Panel::CursorData> ChooseGenderPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void ChooseGenderPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);

	if (escapePressed)
	{
		this->backToNameButton.click(this->getGame());
	}

	const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame().getRenderer().nativeToOriginal(mousePosition);

		if (this->maleButton.contains(mouseOriginalPoint))
		{
			this->maleButton.click(this->getGame());
		}
		else if (this->femaleButton.contains(mouseOriginalPoint))
		{
			this->femaleButton.click(this->getGame());
		}
	}
}

void ChooseGenderPanel::render(Renderer &renderer)
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

	// Draw parchments: title, male, and female.
	const int parchmentX = CharacterCreationUiView::getChooseGenderTitleTextureX(this->parchment.getWidth());
	const int parchmentY = CharacterCreationUiView::getChooseGenderTitleTextureY(this->parchment.getHeight());
	renderer.drawOriginal(this->parchment, parchmentX, parchmentY - 20);
	renderer.drawOriginal(this->parchment, parchmentX, parchmentY + 20);
	renderer.drawOriginal(this->parchment, parchmentX, parchmentY + 60);

	// Draw text: title, male, and female.
	renderer.drawOriginal(this->genderTextBox->getTexture(), this->genderTextBox->getX(), this->genderTextBox->getY());
	renderer.drawOriginal(this->maleTextBox->getTexture(), this->maleTextBox->getX(), this->maleTextBox->getY());
	renderer.drawOriginal(this->femaleTextBox->getTexture(), this->femaleTextBox->getX(), this->femaleTextBox->getY());
}
