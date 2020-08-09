#include <algorithm>
#include <cmath>

#include "SDL.h"

#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "TextCinematicPanel.h"
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

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

TextCinematicPanel::TextCinematicPanel(Game &game, 
	const std::string &sequenceName, const std::string &text, 
	double secondsPerImage, const std::function<void(Game&)> &endingAction)
	: Panel(game), sequenceName(sequenceName)
{
	// Text cannot be empty.
	DebugAssert(text.size() > 0);

	this->textBoxes = [&game, &text]()
	{
		const Int2 center(
			Renderer::ORIGINAL_WIDTH / 2, 
			Renderer::ORIGINAL_HEIGHT - 11);

		// Re-distribute newlines.
		const std::string newText = String::distributeNewlines(text, 60);

		// Count new lines.
		const int newLineCount = static_cast<int>(
			std::count(newText.begin(), newText.end(), '\n'));

		// Split text into lines.
		const std::vector<std::string> textLines = String::split(newText, '\n');

		// Group up to three text lines per text box.
		std::vector<std::unique_ptr<TextBox>> textBoxes;
		int textBoxesToMake = static_cast<int>(std::ceil(newLineCount / 3)) + 1;
		for (int i = 0; i < textBoxesToMake; i++)
		{
			std::string textBoxText;
			int linesToUse = std::min(newLineCount - (i * 3), 3);
			for (int j = 0; j < linesToUse; j++)
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

			const int lineSpacing = 1;

			// Eventually use a different color for other cinematics (Tharn, Emperor, etc.).
			const RichTextString richText(
				textBoxText,
				FontName::Arena,
				Color(105, 174, 207),
				TextAlignment::Center,
				lineSpacing,
				game.getFontManager());

			auto textBox = std::make_unique<TextBox>(center, richText, game.getRenderer());
			textBoxes.push_back(std::move(textBox));
		}

		return textBoxes;
	}();

	this->skipButton = [&endingAction]()
	{
		return Button<Game&>(endingAction);
	}();

	this->secondsPerImage = secondsPerImage;
	this->currentImageSeconds = 0.0;
	this->imageIndex = 0;
	this->textIndex = 0;
}

void TextCinematicPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);

	if (escapePressed)
	{
		// Force the cinematic to end.
		this->skipButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	bool spacePressed = inputManager.keyPressed(e, SDLK_SPACE);
	bool enterPressed = inputManager.keyPressed(e, SDLK_RETURN) ||
		inputManager.keyPressed(e, SDLK_KP_ENTER);

	bool skipHotkeyPressed = spacePressed || enterPressed;

	if (leftClick || skipHotkeyPressed)
	{
		this->textIndex++;

		// If done with the last text box, then prepare for the next panel.
		int textBoxCount = static_cast<int>(this->textBoxes.size());
		if (this->textIndex >= textBoxCount)
		{
			this->textIndex = textBoxCount - 1;
			this->skipButton.click(this->getGame());
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

		// If at the end of the sequence, go back to the first image. The cinematic 
		// ends at the end of the last text box.
		const TextureManager::IdGroup<TextureID> textureIDs = this->getTextureIDs(
			this->sequenceName, PaletteFile::fromName(PaletteName::BuiltIn));

		if (this->imageIndex == textureIDs.getCount())
		{
			this->imageIndex = 0;
		}
	}
}

void TextCinematicPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Get texture IDs in advance of any texture references.
	const TextureManager::IdGroup<TextureID> textureIDs = this->getTextureIDs(
		this->sequenceName, PaletteFile::fromName(PaletteName::BuiltIn));
	const TextureID textureID = textureIDs.getID(this->imageIndex);

	// Draw current frame in animation.
	const auto &textureManager = this->getGame().getTextureManager();
	const TextureRef texture = textureManager.getTextureRef(textureID);
	renderer.drawOriginal(texture.get());

	// Draw the relevant text box.
	DebugAssertIndex(this->textBoxes, this->textIndex);
	const auto &textBox = this->textBoxes[this->textIndex];
	renderer.drawOriginal(textBox->getTexture(), textBox->getX(), textBox->getY());
}
