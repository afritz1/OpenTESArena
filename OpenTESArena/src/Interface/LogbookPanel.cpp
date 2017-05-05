#include <cassert>

#include "SDL.h"

#include "LogbookPanel.h"

#include "GameWorldPanel.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Assets/ExeStrings.h"
#include "../Assets/TextAssets.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"

LogbookPanel::LogbookPanel(Game *game)
	: Panel(game)
{
	this->titleTextBox = [game]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, Renderer::ORIGINAL_HEIGHT / 2);
		Color color(255, 207, 12);
		std::string text = game->getTextAssets().getAExeSegment(
			ExeStrings::LogbookIsEmpty);
		auto &font = game->getFontManager().getFont(FontName::A);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->backButton = []()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH - 40, Renderer::ORIGINAL_HEIGHT - 13);
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> backPanel(new GameWorldPanel(game));
			game->setPanel(std::move(backPanel));
		};
		return std::unique_ptr<Button<Game*>>(
			new Button<Game*>(center, 34, 14, function));
	}();
}

LogbookPanel::~LogbookPanel()
{

}

void LogbookPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame()->getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool lPressed = inputManager.keyPressed(e, SDLK_l);

	if (escapePressed || lPressed)
	{
		this->backButton->click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
			.nativePointToOriginal(mousePosition);

		if (this->backButton->contains(mouseOriginalPoint))
		{
			this->backButton->click(this->getGame());
		}
	}
}

void LogbookPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGame()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw logbook background.
	const auto &logbookBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::Logbook),
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawToOriginal(logbookBackground.get());

	// Draw text: title.
	renderer.drawToOriginal(this->titleTextBox->getTexture(),
		this->titleTextBox->getX(), this->titleTextBox->getY());

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();

	// Draw cursor.
	const auto &cursor = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor));
	const auto &inputManager = this->getGame()->getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const auto &options = this->getGame()->getOptions();
	renderer.drawToNative(cursor.get(),
		mousePosition.x, mousePosition.y,
		static_cast<int>(cursor.getWidth() * options.getCursorScale()),
		static_cast<int>(cursor.getHeight() * options.getCursorScale()));
}
