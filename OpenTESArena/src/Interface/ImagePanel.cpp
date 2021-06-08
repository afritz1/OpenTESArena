#include "SDL.h"

#include "ImagePanel.h"
#include "../Game/Game.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../UI/Texture.h"

ImagePanel::ImagePanel(Game &game)
	: Panel(game) { }

bool ImagePanel::init(const std::string &paletteName, const std::string &textureName,
	double secondsToDisplay, const std::function<void(Game&)> &endingAction)
{
	this->skipButton = Button<Game&>(endingAction);
	this->paletteName = paletteName;
	this->textureName = textureName;
	this->secondsToDisplay = secondsToDisplay;
	this->currentSeconds = 0.0;
	return true;
}

void ImagePanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	
	const bool spacePressed = inputManager.keyPressed(e, SDLK_SPACE);
	const bool enterPressed = inputManager.keyPressed(e, SDLK_RETURN) || inputManager.keyPressed(e, SDLK_KP_ENTER);
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool skipHotkeyPressed = spacePressed || enterPressed || escapePressed;

	if (leftClick || skipHotkeyPressed)
	{
		this->skipButton.click(this->getGame());
	}	
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
