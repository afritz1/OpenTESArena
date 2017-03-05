#include <algorithm>
#include <cassert>

#include "SDL.h"

#include "TextCinematicPanel.h"

#include "Button.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Game/Game.h"
#include "../Math/Vector2.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureSequenceName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

TextCinematicPanel::TextCinematicPanel(Game *game, 
	const std::string &sequenceName, const std::string &text, 
	double secondsPerImage, const std::function<void(Game*)> &endingAction)
	: Panel(game), sequenceName(sequenceName)
{
	// Text cannot be empty.
	assert(text.size() > 0);

	this->textBoxes = [game, &text]()
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
				game->getFontManager().getFont(FontName::Arena),
				TextAlignment::Center,
				game->getRenderer()));
			textBoxes.push_back(std::move(textBox));
		}

		return textBoxes;
	}();

	this->skipButton = [&endingAction]()
	{
		return std::unique_ptr<Button>(new Button(endingAction));
	}();

	this->secondsPerImage = secondsPerImage;
	this->currentImageSeconds = 0.0;
	this->imageIndex = 0;
	this->textIndex = 0;
}

TextCinematicPanel::~TextCinematicPanel()
{

}

void TextCinematicPanel::handleEvent(const SDL_Event &e)
{
	bool escapePressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_ESCAPE);

	if (escapePressed)
	{
		// Force the cinematic to end.
		this->skipButton->click(this->getGame());
	}

	bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_LEFT);
	bool spacePressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_SPACE);
	bool returnPressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_RETURN);
	bool numpadEnterPressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_KP_ENTER);

	bool skipHotkeyPressed = spacePressed || returnPressed || numpadEnterPressed;

	if (leftClick || skipHotkeyPressed)
	{
		this->textIndex++;

		// If done with the last text box, then prepare for the next panel.
		int textBoxCount = static_cast<int>(this->textBoxes.size());
		if (this->textIndex >= textBoxCount)
		{
			this->textIndex = textBoxCount - 1;
			this->skipButton->click(this->getGame());
		}
	}	
}

void TextCinematicPanel::tick(double dt)
{
	this->currentImageSeconds += dt;

	while (this->currentImageSeconds > this->secondsPerImage)
	{
		this->currentImageSeconds -= this->secondsPerImage;
		this->imageIndex++;

		auto &textureManager = this->getGame()->getTextureManager();

		// If at the end of the sequence, go back to the first image. The cinematic 
		// ends at the end of the last text box.
		const auto &textures = textureManager.getTextures(this->sequenceName);
		const int textureCount = static_cast<int>(textures.size());
		if (this->imageIndex == textureCount)
		{
			this->imageIndex = 0;
		}
	}
}

void TextCinematicPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGame()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Get a reference to all relevant textures.
	const auto &textures = textureManager.getTextures(this->sequenceName);

	// Draw animation.
	const auto &texture = textures.at(this->imageIndex);
	renderer.drawToOriginal(texture.get());

	// Get the relevant text box.
	const auto &textBox = this->textBoxes.at(this->textIndex);

	// Draw text.
	renderer.drawToOriginal(textBox->getTexture(), textBox->getX(), textBox->getY());

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();
}
