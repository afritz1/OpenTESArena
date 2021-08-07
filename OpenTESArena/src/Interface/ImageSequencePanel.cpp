#include <algorithm>

#include "SDL.h"

#include "ImageSequencePanel.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Media/TextureManager.h"
#include "../UI/Texture.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"

ImageSequencePanel::ImageSequencePanel(Game &game)
	: Panel(game) { }

ImageSequencePanel::~ImageSequencePanel()
{
	auto &inputManager = this->getGame().getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::Cinematic, false);
}

bool ImageSequencePanel::init(const std::vector<std::string> &paletteNames,
	const std::vector<std::string> &textureNames, const std::vector<double> &imageDurations,
	const OnFinishedFunction &onFinished)
{
	if (paletteNames.size() != textureNames.size())
	{
		DebugLogError("Palette names size (" + std::to_string(paletteNames.size()) +
			") doesn't match texture names size (" + std::to_string(textureNames.size()) + ").");
		return false;
	}

	if (paletteNames.size() != imageDurations.size())
	{
		DebugLogError("Palette names size (" + std::to_string(paletteNames.size()) +
			") doesn't match image durations size (" + std::to_string(imageDurations.size()) + ").");
		return false;
	}

	auto &game = this->getGame();
	auto &inputManager = game.getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::Cinematic, true);

	this->skipButton = Button<Game&>(0, 0, ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT,
		[this](Game &game)
	{
		this->currentSeconds = 0.0;

		const int imageCount = static_cast<int>(this->textureNames.size());
		this->imageIndex = std::min(this->imageIndex + 1, imageCount);

		if (this->imageIndex == imageCount)
		{
			this->onFinished(game);
		}
	});

	this->addButtonProxy(MouseButtonType::Left, this->skipButton.getRect(),
		[this, &game]() { this->skipButton.click(game); });

	this->addInputActionListener(InputActionName::Skip,
		[this, &game](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->onFinished(game);
		}
	});

	this->onFinished = onFinished;
	this->paletteNames = paletteNames;
	this->textureNames = textureNames;
	this->imageDurations = imageDurations;
	this->currentSeconds = 0.0;
	this->imageIndex = 0;
	return true;
}

void ImageSequencePanel::tick(double dt)
{
	const int imageCount = static_cast<int>(this->textureNames.size());

	// Check if done iterating through images.
	if (this->imageIndex < imageCount)
	{
		this->currentSeconds += dt;

		// Step to the next image if its duration has passed.
		if (this->currentSeconds >= this->imageDurations.at(this->imageIndex))
		{
			this->currentSeconds = 0.0;
			this->imageIndex++;

			// Check if the last image is now over.
			if (this->imageIndex == imageCount)
			{
				this->onFinished(this->getGame());
			}
		}
	}

	// Clamp against the max so the index doesn't go outside the image vector.
	this->imageIndex = std::min(this->imageIndex, imageCount - 1);
}

void ImageSequencePanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw image.
	auto &textureManager = this->getGame().getTextureManager();
	DebugAssertIndex(this->paletteNames, this->imageIndex);
	const std::string &paletteName = this->paletteNames[this->imageIndex];
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for \"" + paletteName + "\".");
		return;
	}

	DebugAssertIndex(this->textureNames, this->imageIndex);
	const std::string &textureName = this->textureNames[this->imageIndex];
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureName.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugLogError("Couldn't get texture builder ID for \"" + textureName + "\".");
		return;
	}

	renderer.drawOriginal(*textureBuilderID, *paletteID, textureManager);
}
