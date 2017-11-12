#include <cassert>

#include "SDL.h"

#include "CinematicPanel.h"
#include "../Game/Game.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"

CinematicPanel::CinematicPanel(Game &game,
	const std::string &paletteName, const std::string &sequenceName,
	double secondsPerImage, const std::function<void(Game&)> &endingAction)
	: Panel(game), paletteName(paletteName), sequenceName(sequenceName)
{
	this->skipButton = [&endingAction]()
	{
		return std::unique_ptr<Button<Game&>>(new Button<Game&>(endingAction));
	}();

	this->secondsPerImage = secondsPerImage;
	this->currentSeconds = 0.0;
	this->imageIndex = 0;
}

CinematicPanel::~CinematicPanel()
{

}

void CinematicPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	bool spacePressed = inputManager.keyPressed(e, SDLK_SPACE);
	bool enterPressed = inputManager.keyPressed(e, SDLK_RETURN) ||
		inputManager.keyPressed(e, SDLK_KP_ENTER);
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);

	bool skipHotkeyPressed = spacePressed || enterPressed || escapePressed;

	if (leftClick || skipHotkeyPressed)
	{
		this->skipButton->click(this->getGame());
	}
}

void CinematicPanel::tick(double dt)
{
	// See if it's time for the next image.
	this->currentSeconds += dt;
	while (this->currentSeconds > this->secondsPerImage)
	{
		this->currentSeconds -= this->secondsPerImage;
		this->imageIndex++;
	}

	// Get a reference to all images in the sequence.
	auto &textureManager = this->getGame().getTextureManager();
	const auto &textures = textureManager.getTextures(
		this->sequenceName, this->paletteName);

	// If at the end, then prepare for the next panel.
	if (this->imageIndex >= textures.size())
	{
		this->imageIndex = static_cast<int>(textures.size() - 1);
		this->skipButton->click(this->getGame());
	}
}

void CinematicPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Get a reference to all images in the sequence.
	auto &textureManager = this->getGame().getTextureManager();
	const auto &textures = textureManager.getTextures(
		this->sequenceName, this->paletteName);

	// Draw image.
	const auto &texture = textures.at(this->imageIndex);
	renderer.drawOriginal(texture.get());
}
