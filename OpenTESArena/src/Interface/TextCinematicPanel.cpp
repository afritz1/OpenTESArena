#include <cassert>
#include <iostream>

#include "SDL2\SDL.h"

#include "TextCinematicPanel.h"

#include "Button.h"
#include "../Game/GameState.h"
#include "../Media/TextureManager.h"

// incomplete! Need text box stuff and how to blit it.

TextCinematicPanel::TextCinematicPanel(GameState *gameState, TextureSequenceName name, 
	const std::string &text, const std::vector<double> &secondsPerText, 
	double secondsPerImage, const std::function<void()> &endingAction)
	: Panel(gameState)
{
	this->skipButton = nullptr;

	this->skipButton = [gameState, &endingAction]()
	{
		return std::unique_ptr<Button>(new Button(endingAction));
	}();

	this->sequenceName = name;
	this->text = text;
	this->secondsPerText = secondsPerText;
	this->secondsPerImage = secondsPerImage;
	this->currentImageSeconds = 0.0;
	this->currentTextSeconds = 0.0;
	this->imageIndex = 0;
	this->textIndex = 0;

	assert(this->skipButton.get() != nullptr);
	assert(this->secondsPerText.size() > 0);
	assert(this->text.size() > 0);
	assert(this->sequenceName == name);
	assert(this->secondsPerImage == secondsPerImage);
	assert(this->currentImageSeconds == 0.0);
	assert(this->currentTextSeconds == 0.0);
	assert(this->imageIndex == 0);
	assert(this->textIndex == 0);
}

TextCinematicPanel::~TextCinematicPanel()
{

}

void TextCinematicPanel::handleEvents(bool &running)
{
	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		bool applicationExit = (e.type == SDL_QUIT);
		bool resized = (e.type == SDL_WINDOWEVENT) &&
			(e.window.event == SDL_WINDOWEVENT_RESIZED);

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

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);
		bool skipHotkeyPressed = (e.type == SDL_KEYDOWN) &&
			((e.key.keysym.sym == SDLK_SPACE) || 
				(e.key.keysym.sym == SDLK_RETURN) ||
				(e.key.keysym.sym == SDLK_ESCAPE) ||
				(e.key.keysym.sym == SDLK_KP_ENTER));

		if (leftClick || skipHotkeyPressed)
		{
			this->skipButton->click();
		}
	}
}

void TextCinematicPanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void TextCinematicPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void TextCinematicPanel::tick(double dt, bool &running)
{
	this->handleEvents(running);

	this->currentImageSeconds += dt;
	this->currentTextSeconds += dt;

	while (this->currentImageSeconds > this->secondsPerImage)
	{
		this->currentImageSeconds -= this->secondsPerImage;
		this->imageIndex++;
	}

	while (this->currentTextSeconds > this->secondsPerText.at(this->textIndex))
	{
		this->currentTextSeconds -= this->secondsPerText.at(this->textIndex);
		this->textIndex++;

		if (this->textIndex >= this->secondsPerText.size())
		{
			this->textIndex = static_cast<int>(this->secondsPerText.size() - 1);
			this->skipButton->click();
			break;
		}
	}
}

void TextCinematicPanel::render(SDL_Surface *dst, const SDL_Rect *letterbox)
{
	const auto &images =
		this->getGameState()->getTextureManager().getSequence(this->sequenceName);

	if (this->imageIndex >= images.size())
	{
		this->imageIndex = static_cast<int>(images.size() - 1);
		this->skipButton->click();
	}

	// Draw animation.
	const auto &image = images.at(this->imageIndex);
	auto *imageSurface = image.getSurface();
	SDL_BlitScaled(imageSurface, nullptr, dst, const_cast<SDL_Rect*>(letterbox));

	// Draw text (make text box..., transform point...).
	// ...
}
