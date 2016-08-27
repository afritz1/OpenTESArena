#include <algorithm>
#include <cassert>

#include "SDL.h"

#include "TextCinematicPanel.h"

#include "Button.h"
#include "TextBox.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureSequenceName.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

const double TextCinematicPanel::DEFAULT_MOVIE_SECONDS_PER_IMAGE = 1.0 / 7.0;

TextCinematicPanel::TextCinematicPanel(GameState *gameState, TextureSequenceName name,
	const std::string &text, double secondsPerImage,
	const std::function<void(GameState*)> &endingAction)
	: Panel(gameState)
{
	// Text cannot be empty.
	assert(text.size() > 0);

	this->textBoxes = [gameState, &text]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, Renderer::ORIGINAL_HEIGHT - 12);

		// Count new lines.
		int newLineCount = static_cast<int>(std::count(text.begin(), text.end(), '\n'));

		// Split text into lines.
		std::vector<std::string> textLines = String::split(text, '\n');

		// Group up to three text lines per text box.
		std::vector<std::unique_ptr<TextBox>> textBoxes;
		int textBoxesToMake = static_cast<int>(std::ceil(newLineCount / 3)) + 1;
		for (int i = 0; i < textBoxesToMake; ++i)
		{
			std::string textBoxText;
			int linesToUse = std::min(newLineCount - (i * 3), 3);
			for (int j = 0; j < linesToUse; ++j)
			{
				const std::string &textLine = textLines.at(j + (i * 3));
				textBoxText.append(textLine);
				textBoxText.append("\n");
			}
			
			// Avoid adding empty text boxes.
			if (textBoxText.size() == 0)
			{
				continue;
			}

			// Eventually use a different color for other cinematics (Tharn, Emperor, etc.).
			Color textColor(105, 174, 207);
			std::unique_ptr<TextBox> textBox(new TextBox(
				center,
				textColor,
				textBoxText,
				FontName::Arena,
				gameState->getTextureManager(),
				gameState->getRenderer()));
			textBoxes.push_back(std::move(textBox));
		}

		return textBoxes;
	}();

	this->skipButton = [&endingAction]()
	{
		return std::unique_ptr<Button>(new Button(endingAction));
	}();

	this->sequenceName = name;
	this->secondsPerImage = secondsPerImage;
	this->currentImageSeconds = 0.0;
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
			// Force the cinematic to end.
			this->skipButton->click(this->getGameState());
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);
		bool skipHotkeyPressed = (e.type == SDL_KEYDOWN) &&
			((e.key.keysym.sym == SDLK_SPACE) ||
			(e.key.keysym.sym == SDLK_RETURN) ||
				(e.key.keysym.sym == SDLK_KP_ENTER));

		if (leftClick || skipHotkeyPressed)
		{
			this->textIndex++;

			// If done with the last text box, then prepare for the next panel.
			int textBoxCount = static_cast<int>(this->textBoxes.size());
			if (this->textIndex >= textBoxCount)
			{
				this->textIndex = textBoxCount - 1;
				this->skipButton->click(this->getGameState());
			}
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

	while (this->currentImageSeconds > this->secondsPerImage)
	{
		this->currentImageSeconds -= this->secondsPerImage;
		this->imageIndex++;

		// If at the end of the sequence, go back to the first image. The cinematic 
		// ends at the end of the last text box.
		auto imageFilenames = TextureFile::fromName(this->sequenceName);
		int imageFilenameCount = static_cast<int>(imageFilenames.size());
		if (this->imageIndex == imageFilenameCount)
		{
			this->imageIndex = 0;
		}
	}
}

void TextCinematicPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();

	// Set palette.
	auto &textureManager = this->getGameState()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Get all of the image filenames relevant to the sequence.
	auto imageFilenames = TextureFile::fromName(this->sequenceName);

	// Draw animation.
	auto *image = textureManager.getTexture(
		imageFilenames.at(this->imageIndex));
	renderer.drawToOriginal(image);

	// Get the relevant text box.
	const auto &textBox = this->textBoxes.at(this->textIndex);

	// Draw text.
	renderer.drawToOriginal(textBox->getSurface(), textBox->getX(), textBox->getY());

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();
}
