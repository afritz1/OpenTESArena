#include "ImagePanel.h"
#include "../Assets/TextureManager.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"

ImagePanel::ImagePanel(Game &game)
	: Panel(game) { }

bool ImagePanel::init(const std::string &paletteName, const std::string &textureName,
	double secondsToDisplay, const OnFinishedFunction &onFinished)
{
	auto &game = this->getGame();

	// Fullscreen button.
	this->skipButton = Button<Game&>(
		0,
		0,
		ArenaRenderUtils::SCREEN_WIDTH,
		ArenaRenderUtils::SCREEN_HEIGHT,
		onFinished);

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

	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;
	const TextureAsset textureAsset = TextureAsset(std::string(textureName));
	const TextureAsset paletteTextureAsset = TextureAsset(std::string(paletteName));

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugLogError("Couldn't create UI texture for image \"" + textureName + "\" with palette \"" + paletteName + "\".");
		return false;
	}

	this->textureRef.init(textureID, renderer);
	this->addDrawCall(
		this->textureRef.get(),
		Int2::Zero,
		Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT),
		PivotType::TopLeft);

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
