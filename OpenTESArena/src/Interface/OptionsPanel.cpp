#include <cassert>
#include <iostream>

#include "SDL.h"

#include "OptionsPanel.h"

#include "Button.h"
#include "PauseMenuPanel.h"
#include "TextBox.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

OptionsPanel::OptionsPanel(GameState *gameState)
	: Panel(gameState)
{
	this->titleTextBox = [gameState]()
	{
		auto center = Int2(160, 30);
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

	this->backToPauseButton = [gameState]()
	{
		auto function = [gameState]()
		{
			auto pausePanel = std::unique_ptr<Panel>(new PauseMenuPanel(gameState));
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
			this->backToPauseButton->click();
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

void OptionsPanel::render(SDL_Surface *dst, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(dst);

	// Draw temporary background.
	SDL_FillRect(dst, letterbox, SDL_MapRGB(dst->format, 48, 48, 36));

	// Draw buttons, eventually...


	// Draw text: title.
	this->drawScaledToNative(*this->titleTextBox.get(), dst);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, dst);
}
