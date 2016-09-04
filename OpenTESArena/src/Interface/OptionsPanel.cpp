#include <cassert>

#include "SDL.h"

#include "OptionsPanel.h"

#include "Button.h"
#include "PauseMenuPanel.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"

OptionsPanel::OptionsPanel(GameState *gameState)
	: Panel(gameState)
{
	this->titleTextBox = [gameState]()
	{
		Int2 center(160, 30);
		auto color = Color::White;
		std::string text = "Options";
		auto &font = gameState->getFontManager().getFont(FontName::A);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			font,
			alignment,
			gameState->getRenderer()));
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

void OptionsPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();

	// Set palette.
	auto &textureManager = this->getGameState()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw temporary background.
	renderer.clearOriginal(Color(48, 48, 36));

	// Draw buttons, eventually...


	// Draw text: title.
	renderer.drawToOriginal(this->titleTextBox->getTexture(),
		this->titleTextBox->getX(), this->titleTextBox->getY());

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
