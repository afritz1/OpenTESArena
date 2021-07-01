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
#include "../UI/CursorData.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextEntry.h"

#include "components/utilities/String.h"

ChooseNamePanel::ChooseNamePanel(Game &game)
	: Panel(game) { }

bool ChooseNamePanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();

	this->parchment = TextureUtils::generate(
		CharacterCreationUiView::ChooseNameTexturePatternType,
		CharacterCreationUiView::ChooseNameTextureWidth,
		CharacterCreationUiView::ChooseNameTextureHeight,
		game.getTextureManager(),
		renderer);

	const auto &fontLibrary = game.getFontLibrary();
	const std::string titleText = CharacterCreationUiModel::getChooseNameTitleText(game);
	const TextBox::InitInfo titleTextBoxInitInfo =
		CharacterCreationUiView::getChooseNameTitleTextBoxInitInfo(titleText, fontLibrary);
	if (!this->titleTextBox.init(titleTextBoxInitInfo, titleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	const TextBox::InitInfo entryTextBoxInitInfo =
		CharacterCreationUiView::getChooseNameEntryTextBoxInitInfo(fontLibrary);
	if (!this->entryTextBox.init(entryTextBoxInitInfo, renderer))
	{
		DebugLogError("Couldn't init entry text box.");
		return false;
	}

	this->backToClassButton = Button<Game&>(CharacterCreationUiController::onBackToChooseClassButtonSelected);
	this->acceptButton = Button<Game&, const std::string&>(CharacterCreationUiController::onChooseNameAcceptButtonSelected);

	// Activate SDL text input (handled in handleEvent()).
	SDL_StartTextInput();

	return true;
}

std::optional<CursorData> ChooseNamePanel::getCurrentCursor() const
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
			this->entryTextBox.setText(this->name);
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
	const Rect &titleTextBoxRect = this->titleTextBox.getRect();
	const Rect &entryTextBoxRect = this->entryTextBox.getRect();
	renderer.drawOriginal(this->titleTextBox.getTexture(), titleTextBoxRect.getLeft(), titleTextBoxRect.getTop());
	renderer.drawOriginal(this->entryTextBox.getTexture(), entryTextBoxRect.getLeft(), entryTextBoxRect.getTop());
}
