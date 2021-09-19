#include "ImagePanel.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../Media/TextureManager.h"
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

	auto &textureManager = game.getTextureManager();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette for image from \"" + paletteName + "\".");
		return false;
	}

	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureName.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugLogError("Couldn't get texture builder ID for image from \"" + textureName + "\".");
		return false;
	}

	auto &renderer = game.getRenderer();
	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(*textureBuilderID, *paletteID, &textureID))
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
