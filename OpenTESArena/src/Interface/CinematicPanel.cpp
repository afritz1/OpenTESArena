#include "SDL.h"

#include "CinematicPanel.h"
#include "Texture.h"
#include "../Game/Game.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"

CinematicPanel::CinematicPanel(Game &game,
	const std::string &paletteName, const std::string &sequenceName,
	double secondsPerImage, const std::function<void(Game&)> &endingAction)
	: Panel(game), paletteName(paletteName), sequenceName(sequenceName)
{
	this->skipButton = [&endingAction]()
	{
		return Button<Game&>(endingAction);
	}();

	this->secondsPerImage = secondsPerImage;
	this->currentSeconds = 0.0;
	this->imageIndex = 0;
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
		this->skipButton.click(this->getGame());
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

	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const std::optional<TextureFileMetadata> textureFileMetadata =
		textureManager.tryGetMetadata(this->sequenceName.c_str());
	if (!textureFileMetadata.has_value())
	{
		DebugLogError("Couldn't get texture file metadata for \"" + this->sequenceName + "\".");
		return;
	}

	// If at the end, then prepare for the next panel.
	const int textureCount = textureFileMetadata->getTextureCount();
	if (this->imageIndex >= textureCount)
	{
		this->imageIndex = textureCount - 1;
		this->skipButton.click(game);
	}
}

void CinematicPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Get current frame of the cinematic and draw it.
	auto &textureManager = this->getGame().getTextureManager();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(this->paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for \"" + this->paletteName + "\".");
		return;
	}

	const std::optional<TextureBuilderIdGroup> textureBuilderIdGroup =
		textureManager.tryGetTextureBuilderIDs(this->sequenceName.c_str());
	if (!textureBuilderIdGroup.has_value())
	{
		DebugLogError("Couldn't get texture builder IDs for \"" + this->sequenceName + "\".");
		return;
	}

	const TextureBuilderID textureBuilderID = textureBuilderIdGroup->getID(this->imageIndex);
	renderer.drawOriginal(textureBuilderID, *paletteID, textureManager);
}
