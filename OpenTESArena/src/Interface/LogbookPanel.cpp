#include "SDL.h"

#include "GameWorldPanel.h"
#include "LogbookPanel.h"
#include "LogbookUiController.h"
#include "LogbookUiModel.h"
#include "LogbookUiView.h"
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
#include "../UI/FontName.h"
#include "../UI/RichTextString.h"
#include "../UI/TextAlignment.h"
#include "../UI/Texture.h"

LogbookPanel::LogbookPanel(Game &game)
	: Panel(game) { }

bool LogbookPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	const auto &fontLibrary = game.getFontLibrary();

	const std::string titleText = LogbookUiModel::getTitleText(game);
	const TextBox::InitInfo titleTextBoxInitInfo =
		LogbookUiView::getTitleTextBoxInitInfo(titleText, fontLibrary);
	if (!this->titleTextBox.init(titleTextBoxInitInfo, titleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	this->backButton = Button<Game&>(
		LogbookUiView::BackButtonCenterPoint,
		LogbookUiView::BackButtonWidth,
		LogbookUiView::BackButtonHeight,
		LogbookUiController::onBackButtonSelected);

	return true;
}

std::optional<CursorData> LogbookPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void LogbookPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool lPressed = inputManager.keyPressed(e, SDLK_l);

	if (escapePressed || lPressed)
	{
		this->backButton.click(this->getGame());
	}

	const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame().getRenderer().nativeToOriginal(mousePosition);

		if (this->backButton.contains(mouseOriginalPoint))
		{
			this->backButton.click(this->getGame());
		}
	}
}

void LogbookPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Logbook background.
	auto &textureManager = this->getGame().getTextureManager();
	const TextureAssetReference paletteTextureAssetRef = LogbookUiView::getBackgroundPaletteTextureAssetRef();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteTextureAssetRef);
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for \"" + paletteTextureAssetRef.filename + "\".");
		return;
	}

	const TextureAssetReference backgroundTextureAssetRef = LogbookUiView::getBackgroundTextureAssetRef();
	const std::optional<TextureBuilderID> backgroundTextureBuilderID =
		textureManager.tryGetTextureBuilderID(backgroundTextureAssetRef);
	if (!backgroundTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get texture builder ID for \"" + backgroundTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*backgroundTextureBuilderID, *paletteID, textureManager);

	// Draw text: title.
	const Rect &titleTextBoxRect = this->titleTextBox.getRect();
	renderer.drawOriginal(this->titleTextBox.getTexture(), titleTextBoxRect.getLeft(), titleTextBoxRect.getTop());
}
