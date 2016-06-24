#include <cassert>

#include "SDL.h"

#include "TextCinematicPanel.h"

#include "Button.h"
#include "../Game/GameState.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureSequenceName.h"

// incomplete! Need text box stuff and how to blit it.

TextCinematicPanel::TextCinematicPanel(GameState *gameState, TextureSequenceName name,
	const std::string &text, const std::vector<double> &secondsPerText,
	double secondsPerImage, const std::function<void()> &endingAction)
	: Panel(gameState)
{
	// There must be at least one text duration, and text to show must be non-empty.
	assert(secondsPerText.size() > 0);
	assert(text.size() > 0);

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

void TextCinematicPanel::render(SDL_Renderer *renderer, const SDL_Rect *letterbox)
{
	// Get all of the image filenames relevant to the sequence.
	auto imageFilenames = TextureFile::fromName(this->sequenceName);

	// If at the end of the sequence, go back to the first image. The cinematic 
	// ends when speech is done.
	if (this->imageIndex >= imageFilenames.size())
	{
		this->imageIndex = 0;
		this->skipButton->click();
	}

	// Draw animation.
	const auto *image = this->getGameState()->getTextureManager()
		.getTexture(imageFilenames.at(this->imageIndex));
	this->drawScaledToNative(image, renderer);

	// Draw text...
}
