#include <cassert>

#include "SDL.h"

#include "LoadGamePanel.h"

#include "MainMenuPanel.h"
#include "PauseMenuPanel.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/MusicName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"

LoadGamePanel::LoadGamePanel(Game *game)
	: Panel(game)
{
	this->underConstructionTextBox = [game]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, 170);
		auto color = Color::White;
		std::string text = "Under construction!";
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
		auto function = [](Game *game)
		{
			// Back button behavior depends on if game data is active.
			auto backPanel = game->gameDataIsActive() ?
				std::unique_ptr<Panel>(new PauseMenuPanel(game)) :
				std::unique_ptr<Panel>(new MainMenuPanel(game));
			game->setPanel(std::move(backPanel));
		};
		return std::unique_ptr<Button<Game*>>(new Button<Game*>(function));
	}();
}

LoadGamePanel::~LoadGamePanel()
{

}

void LoadGamePanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame()->getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);

	if (escapePressed)
	{
		this->backButton->click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
			.nativePointToOriginal(mousePosition);

		// Listen for up/down arrow click, saved game click...
	}
}

void LoadGamePanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGame()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw slots background.
	const auto &slotsBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::LoadSave));
	renderer.drawToOriginal(slotsBackground.get());

	// Draw temp text. The load game design is unclear at this point, but it should
	// have up/down arrows and buttons.
	renderer.drawToOriginal(this->underConstructionTextBox->getTexture(),
		this->underConstructionTextBox->getX(), this->underConstructionTextBox->getY());

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
