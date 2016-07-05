#include <cassert>

#include "SDL.h"

#include "OptionsPanel.h"

#include "Button.h"
#include "PauseMenuPanel.h"
#include "TextBox.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

OptionsPanel::OptionsPanel(GameState *gameState)
	: Panel(gameState)
{
	this->titleTextBox = [gameState]()
	{
		Int2 center(160, 30);
		auto color = Color::White;
		std::string text = "Options";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->backToPauseButton = []()
	{
		auto function = [](GameState *gameState)
		{
			std::unique_ptr<Panel> pausePanel(new PauseMenuPanel(gameState));
			gameState->setPanel(std::move(pausePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();
}

OptionsPanel::~OptionsPanel()
{

}

void OptionsPanel::handleEvents(bool &running)
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
			this->backToPauseButton->click(this->getGameState());
		}

		/*bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_LEFT);*/

		// Option clicks...
	}
}

void OptionsPanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void OptionsPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void OptionsPanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
}

void OptionsPanel::render(SDL_Renderer *renderer, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(renderer);

	// Set palette.
	auto &textureManager = this->getGameState()->getTextureManager();
	textureManager.setPalette(PaletteName::Default);

	// Draw temporary background.
	SDL_SetRenderDrawColor(renderer, 48, 48, 36, 255);
	SDL_RenderDrawRect(renderer, letterbox);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	// Draw buttons, eventually...


	// Draw text: title.
	this->drawScaledToNative(*this->titleTextBox.get(), renderer);

	// Draw cursor.
	const auto &cursor = textureManager.getSurface(
		TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, renderer);
}
