#include <cassert>

#include <SDL2/SDL.h>

#include "LoadGamePanel.h"

#include "Button.h"
#include "MainMenuPanel.h"
#include "TextBox.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/MusicName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

LoadGamePanel::LoadGamePanel(GameState *gameState)
	: Panel(gameState)
{
	this->backToMainMenuButton = nullptr;
	this->underConstructionTextBox = nullptr;

	this->backToMainMenuButton = [gameState]()
	{
		auto function = [gameState]()
		{
			auto mainMenuPanel = std::unique_ptr<Panel>(
				new MainMenuPanel(gameState));
			gameState->setPanel(std::move(mainMenuPanel));
		};

		return std::unique_ptr<Button>(new Button(function));
	}();

	this->underConstructionTextBox = [gameState]()
	{
		auto center = Int2(160, 85);
		auto color = Color::White;
		std::string text = "Under construction!";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	assert(this->backToMainMenuButton.get() != nullptr);
	assert(this->underConstructionTextBox.get() != nullptr);
}

LoadGamePanel::~LoadGamePanel()
{

}

void LoadGamePanel::handleEvents(bool &running)
{
	auto mousePosition = this->getMousePosition();
	auto mouseOriginalPoint = this->nativePointToOriginal(mousePosition);

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
			this->backToMainMenuButton->click();
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

void LoadGamePanel::render(SDL_Surface *dst, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(dst);

	// Draw temp text. The load game design is unclear at this point, but it should
	// have up/down arrows and buttons.
	this->drawScaledToNative(*this->underConstructionTextBox.get(), dst);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, dst);
}
