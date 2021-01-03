#include "SDL.h"

#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "LogbookPanel.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "Texture.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ExeData.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/FontLibrary.h"
#include "../Media/FontName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"

LogbookPanel::LogbookPanel(Game &game)
	: Panel(game)
{
	this->titleTextBox = [&game]()
	{
		const Int2 center(
			ArenaRenderUtils::SCREEN_WIDTH / 2,
			ArenaRenderUtils::SCREEN_HEIGHT / 2);

		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
		const std::string &text = exeData.logbook.isEmpty;

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			FontName::A,
			Color(255, 207, 12),
			TextAlignment::Center,
			fontLibrary);

		return std::make_unique<TextBox>(center, richText, fontLibrary, game.getRenderer());
	}();

	this->backButton = []()
	{
		const Int2 center(
			ArenaRenderUtils::SCREEN_WIDTH - 40,
			ArenaRenderUtils::SCREEN_HEIGHT - 13);

		auto function = [](Game &game)
		{
			game.setPanel<GameWorldPanel>(game);
		};
		return Button<Game&>(center, 34, 14, function);
	}();
}

std::optional<Panel::CursorData> LogbookPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void LogbookPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool lPressed = inputManager.keyPressed(e, SDLK_l);

	if (escapePressed || lPressed)
	{
		this->backButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

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
	const std::string &backgroundTextureName = ArenaTextureName::Logbook;
	const std::string &backgroundPaletteName = backgroundTextureName;
	const std::optional<PaletteID> backgroundPaletteID = textureManager.tryGetPaletteID(backgroundPaletteName.c_str());
	if (!backgroundPaletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for \"" + backgroundPaletteName + "\".");
		return;
	}

	const std::optional<TextureBuilderID> backgroundTextureBuilderID =
		textureManager.tryGetTextureBuilderID(backgroundTextureName.c_str());
	if (!backgroundTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get texture builder ID for \"" + backgroundTextureName + "\".");
		return;
	}

	renderer.drawOriginal(*backgroundTextureBuilderID, *backgroundPaletteID, textureManager);

	// Draw text: title.
	renderer.drawOriginal(this->titleTextBox->getTexture(), this->titleTextBox->getX(), this->titleTextBox->getY());
}
