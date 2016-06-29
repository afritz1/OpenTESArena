#include <cassert>

#include "SDL.h"

#include "LogbookPanel.h"

#include "Button.h"
#include "GameWorldPanel.h"
#include "TextBox.h"
#include "../Game/GameState.h"
#include "../Math/Constants.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

LogbookPanel::LogbookPanel(GameState *gameState)
	: Panel(gameState)
{
	this->backButton = [gameState]()
	{
		Int2 center(ORIGINAL_WIDTH - 40, ORIGINAL_HEIGHT - 13);
		auto function = [gameState]()
		{
			std::unique_ptr<Panel> backPanel(new GameWorldPanel(gameState));
			gameState->setPanel(std::move(backPanel));
		};
		return std::unique_ptr<Button>(new Button(center, 34, 14, function));
	}();

	this->titleTextBox = [gameState]()
	{
		Int2 center(ORIGINAL_WIDTH / 2, ORIGINAL_HEIGHT / 2);
		Color color(255, 207, 12);
		std::string text = "Your logbook is empty.";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();
}

LogbookPanel::~LogbookPanel()
{

}

void LogbookPanel::handleEvents(bool &running)
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
		bool lPressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_l);

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
		if (escapePressed || lPressed)
		{
			this->backButton->click();
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);		

		if (leftClick && this->backButton->containsPoint(mouseOriginalPoint))
		{
			this->backButton->click();
		}
	}
}

void LogbookPanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void LogbookPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void LogbookPanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
}

void LogbookPanel::render(SDL_Renderer *renderer, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(renderer);

	// Draw logbook background.
	const auto *logbookBackground = this->getGameState()->getTextureManager()
		.getTexture(TextureFile::fromName(TextureName::Logbook));
	this->drawLetterbox(logbookBackground, renderer, letterbox);

	// Draw text: title.
	this->drawScaledToNative(*this->titleTextBox.get(), renderer);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, renderer);
}
