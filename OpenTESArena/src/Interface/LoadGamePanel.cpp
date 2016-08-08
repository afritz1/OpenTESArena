#include <cassert>

#include "SDL.h"

#include "LoadGamePanel.h"

#include "Button.h"
#include "MainMenuPanel.h"
#include "PauseMenuPanel.h"
#include "TextBox.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/MusicName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"

LoadGamePanel::LoadGamePanel(GameState *gameState)
	: Panel(gameState)
{
	this->underConstructionTextBox = [gameState]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, 170);
		auto color = Color::White;
		std::string text = "Under construction!";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager(),
			gameState->getRenderer()));
	}();

	this->backButton = []()
	{
		auto function = [](GameState *gameState)
		{
			// Back button behavior depends on if game data is active.
			auto backPanel = gameState->gameDataIsActive() ?
				std::unique_ptr<Panel>(new PauseMenuPanel(gameState)) :
				std::unique_ptr<Panel>(new MainMenuPanel(gameState));
			gameState->setPanel(std::move(backPanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();
}

LoadGamePanel::~LoadGamePanel()
{

}

void LoadGamePanel::handleEvents(bool &running)
{
	auto mousePosition = this->getMousePosition();
	auto mouseOriginalPoint = this->getGameState()->getRenderer()
		.nativePointToOriginal(mousePosition);

	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		bool applicationExit = (e.type == SDL_QUIT);
		bool resized = (e.type == SDL_WINDOWEVENT) &&
			(e.window.event == SDL_WINDOWEVENT_RESIZED);
		bool escapePressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_ESCAPE);

		if (applicationExit)
		{
			running = false;
		}
		if (resized)
		{
			int width = e.window.data1;
			int height = e.window.data2;
			this->getGameState()->resizeWindow(width, height);
		}
		if (escapePressed)
		{
			this->backButton->click(this->getGameState());
		}

		/*bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);*/

		// Listen for up/down arrow click, saved game click...
	}
}

void LoadGamePanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void LoadGamePanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void LoadGamePanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
}

void LoadGamePanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGameState()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw slots background.
	auto *slotsBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::LoadSave));
	renderer.drawToOriginal(slotsBackground);

	// Draw temp text. The load game design is unclear at this point, but it should
	// have up/down arrows and buttons.
	renderer.drawToOriginal(this->underConstructionTextBox->getSurface(),
		this->underConstructionTextBox->getX(), this->underConstructionTextBox->getY());

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();

	// Draw cursor.
	const auto &cursor = textureManager.getSurface(
		TextureFile::fromName(TextureName::SwordCursor));
	SDL_SetColorKey(cursor.getSurface(), SDL_TRUE,
		renderer.getFormattedARGB(Color::Black));
	auto mousePosition = this->getMousePosition();
	renderer.drawToNative(cursor.getSurface(),
		mousePosition.getX(), mousePosition.getY(),
		static_cast<int>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int>(cursor.getHeight() * this->getCursorScale()));
}
