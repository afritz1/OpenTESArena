#include <unordered_map>

#include "SDL.h"

#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseClassPanel.h"
#include "ChooseGenderPanel.h"
#include "ChooseNamePanel.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ExeData.h"
#include "../Entities/CharacterClassDefinition.h"
#include "../Entities/CharacterClassLibrary.h"
#include "../Game/CharacterCreationState.h"
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
#include "../UI/TextEntry.h"

#include "components/utilities/String.h"

ChooseNamePanel::ChooseNamePanel(Game &game)
	: Panel(game) { }

bool ChooseNamePanel::init()
{
	auto &game = this->getGame();

	this->parchment = TextureUtils::generate(
		CharacterCreationUiView::ChooseNameTexturePatternType,
		CharacterCreationUiView::ChooseNameTextureWidth,
		CharacterCreationUiView::ChooseNameTextureHeight,
		game.getTextureManager(),
		game.getRenderer());

	this->titleTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			CharacterCreationUiModel::getChooseNameTitleText(game),
			CharacterCreationUiView::ChooseNameTitleFontName,
			CharacterCreationUiView::ChooseNameTitleColor,
			CharacterCreationUiView::ChooseNameTitleAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			CharacterCreationUiView::ChooseNameTitleTextBoxX,
			CharacterCreationUiView::ChooseNameTitleTextBoxY,
			richText,
			fontLibrary,
			game.getRenderer());
	}();

	this->nameTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			std::string(),
			CharacterCreationUiView::ChooseNameEntryFontName,
			CharacterCreationUiView::ChooseNameEntryColor,
			CharacterCreationUiView::ChooseNameEntryAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			CharacterCreationUiView::ChooseNameEntryTextBoxX,
			CharacterCreationUiView::ChooseNameEntryTextBoxY,
			richText,
			fontLibrary,
			game.getRenderer());
	}();

	this->backToClassButton = Button<Game&>(CharacterCreationUiController::onBackToChooseClassButtonSelected);
	this->acceptButton = Button<Game&, const std::string&>(CharacterCreationUiController::onChooseNameAcceptButtonSelected);

	// Activate SDL text input (handled in handleEvent()).
	SDL_StartTextInput();

	return true;
}

std::optional<Panel::CursorData> ChooseNamePanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void ChooseNamePanel::handleEvent(const SDL_Event &e)
{
	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool enterPressed = inputManager.keyPressed(e, SDLK_RETURN) || inputManager.keyPressed(e, SDLK_KP_ENTER);
	const bool backspacePressed = inputManager.keyPressed(e, SDLK_BACKSPACE) ||
		inputManager.keyPressed(e, SDLK_KP_BACKSPACE);

	if (escapePressed)
	{
		this->backToClassButton.click(this->getGame());
	}
	else if (enterPressed && (this->name.size() > 0))
	{
		// Accept the given name.
		this->acceptButton.click(this->getGame(), this->name);
	}
	else
	{
		// Listen for SDL text input and changes in text.
		const bool textChanged = TextEntry::updateText(this->name, e, backspacePressed,
			CharacterCreationUiModel::isPlayerNameCharacterAccepted, CharacterCreationState::MAX_NAME_LENGTH);

		if (textChanged)
		{
			// Update the displayed name.
			this->nameTextBox = [this]()
			{
				auto &game = this->getGame();
				const RichTextString &oldRichText = this->nameTextBox->getRichText();

				const auto &fontLibrary = game.getFontLibrary();
				const RichTextString richText(
					this->name,
					oldRichText.getFontName(),
					oldRichText.getColor(),
					oldRichText.getAlignment(),
					fontLibrary);

				return std::make_unique<TextBox>(
					CharacterCreationUiView::ChooseNameEntryTextBoxX,
					CharacterCreationUiView::ChooseNameEntryTextBoxY,
					richText,
					fontLibrary,
					game.getRenderer());
			}();
		}
	}
}

void ChooseNamePanel::render(Renderer &renderer)
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

	// Draw parchment: title.
	const int titleParchmentX = CharacterCreationUiView::getChooseNameTitleTextureX(this->parchment.getWidth());
	const int titleParchmentY = CharacterCreationUiView::getChooseNameTitleTextureY(this->parchment.getHeight());
	renderer.drawOriginal(this->parchment, titleParchmentX, titleParchmentY);

	// Draw text: title, name.
	renderer.drawOriginal(this->titleTextBox->getTexture(), this->titleTextBox->getX(), this->titleTextBox->getY());
	renderer.drawOriginal(this->nameTextBox->getTexture(), this->nameTextBox->getX(), this->nameTextBox->getY());
}
