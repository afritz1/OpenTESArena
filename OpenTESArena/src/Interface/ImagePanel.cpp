#include "SDL.h"

#include "ImagePanel.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/Texture.h"

ImagePanel::ImagePanel(Game &game)
	: Panel(game) { }

ImagePanel::~ImagePanel()
{
	auto &inputManager = this->getGame().getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::Cinematic, false);
}

bool ImagePanel::init(const std::string &paletteName, const std::string &textureName,
	double secondsToDisplay, const std::function<void(Game&)> &endingAction)
{
	auto &game = this->getGame();
	auto &inputManager = game.getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::Cinematic, true);

	// Fullscreen button.
	this->skipButton = Button<Game&>(
		0,
		0,
		ArenaRenderUtils::SCREEN_WIDTH,
		ArenaRenderUtils::SCREEN_HEIGHT,
		endingAction);

	this->addButtonProxy(MouseButtonType::Left, this->skipButton.getRect(),
		[this, &game]() { this->skipButton.click(game); });

	this->addInputActionListener(InputActionName::Skip,
		[this, &game](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->skipButton.click(game);
		}
	});

	this->paletteName = paletteName;
	this->textureName = textureName;
	this->secondsToDisplay = secondsToDisplay;
	this->currentSeconds = 0.0;
	return true;
}

void ImagePanel::tick(double dt)
{
	this->currentSeconds += dt;
	if (this->currentSeconds > this->secondsToDisplay)
	{
		this->skipButton.click(this->getGame());
	}
}

void ImagePanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw image.
	auto &textureManager = this->getGame().getTextureManager();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(this->paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for \"" + this->paletteName + "\".");
		return;
	}

	const std::optional<TextureBuilderID> textureBuilderID =
		textureManager.tryGetTextureBuilderID(this->textureName.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugLogError("Couldn't get texture builder ID for \"" + this->textureName + "\".");
		return;
	}
	
	renderer.drawOriginal(*textureBuilderID, *paletteID, textureManager);
}
