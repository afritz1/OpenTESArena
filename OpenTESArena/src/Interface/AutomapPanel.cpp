#include <cassert>

#include "SDL.h"

#include "AutomapPanel.h"

#include "Button.h"
#include "GameWorldPanel.h"
#include "TextBox.h"
#include "../Game/GameState.h"
#include "../Math/Constants.h"
#include "../Math/Int2.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

AutomapPanel::AutomapPanel(GameState *gameState)
	: Panel(gameState)
{
	this->backToGameButton = []()
	{
		Int2 center(ORIGINAL_WIDTH - 57, ORIGINAL_HEIGHT - 29);
		int width = 38;
		int height = 13;
		auto function = [](GameState *gameState)
		{
			std::unique_ptr<Panel> gamePanel(new GameWorldPanel(gameState));
			gameState->setPanel(std::move(gamePanel));
		};
		return std::unique_ptr<Button>(new Button(center, width, height, function));
	}();
}

AutomapPanel::~AutomapPanel()
{

}

void AutomapPanel::handleEvents(bool &running)
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
		bool nPressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_n);

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
		if (escapePressed || nPressed)
		{
			this->backToGameButton->click(this->getGameState());
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);

		if (leftClick)
		{
			// Check if "Exit" was clicked.
			if (this->backToGameButton->containsPoint(mouseOriginalPoint))
			{
				this->backToGameButton->click(this->getGameState());
			}
		}
	}
}

void AutomapPanel::handleMouse(double dt)
{
	// Check if the left mouse button is held on one of the compass directions.
	static_cast<void>(dt);
}

void AutomapPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void AutomapPanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
}

void AutomapPanel::render(SDL_Renderer *renderer, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(renderer);

	// Draw automap background.
	auto &textureManager = this->getGameState()->getTextureManager();
	const auto *automapBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::Automap), PaletteName::CharSheet);
	this->drawLetterbox(automapBackground, renderer, letterbox);

	// Prepare the cursor color key.
	const auto *pixelFormat = textureManager.getFormat();
	unsigned int colorKey = this->getFormattedRGB(Color::Black, pixelFormat);

	// Draw quill cursor. This one uses a different point for blitting because 
	// the tip of the cursor is at the bottom left, not the top left.
	const auto &cursor = textureManager.getSurface(
		TextureFile::fromName(TextureName::QuillCursor));
	SDL_SetColorKey(cursor.getSurface(), SDL_TRUE, colorKey);
	const int cursorYOffset = static_cast<int>(
		static_cast<double>(cursor.getHeight()) * this->getCursorScale());
	const auto mousePosition = this->getMousePosition();
	this->drawCursor(cursor, mousePosition.getX(),
		mousePosition.getY() - cursorYOffset, renderer);
}
