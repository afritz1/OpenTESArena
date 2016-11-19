#include <cassert>

#include "SDL.h"

#include "LoadGamePanel.h"

#include "Button.h"
#include "MainMenuPanel.h"
#include "PauseMenuPanel.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Game/Game.h"
#include "../Math/Int2.h"
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
		return std::unique_ptr<Button>(new Button(function));
	}();
}

LoadGamePanel::~LoadGamePanel()
{

}

void LoadGamePanel::handleEvent(const SDL_Event &e)
{
	bool escapePressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_ESCAPE);

	if (escapePressed)
	{
		this->backButton->click(this->getGame());
	}

	bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = this->getMousePosition();
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
	auto mousePosition = this->getMousePosition();
	renderer.drawToNative(cursor.get(),
		mousePosition.getX(), mousePosition.getY(),
		static_cast<int>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int>(cursor.getHeight() * this->getCursorScale()));
}
