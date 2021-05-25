#include "SDL.h"

#include "CinematicPanel.h"
#include "../Game/Game.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../UI/Texture.h"

CinematicPanel::CinematicPanel(Game &game, const std::string &paletteName, const std::string &sequenceName,
	double secondsPerImage, const std::function<void(Game&)> &endingAction)
	: Panel(game), skipButton(endingAction), paletteTextureAssetRef(std::string(paletteName)), sequenceFilename(sequenceName)
{
	this->secondsPerImage = secondsPerImage;
	this->currentSeconds = 0.0;
	this->imageIndex = 0;
}

TextureAssetReference CinematicPanel::getCurrentSequenceTextureAssetRef()
{
	return TextureAssetReference(std::string(this->sequenceFilename), this->imageIndex);
}

void CinematicPanel::handleEvent(const SDL_Event &e)
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
	auto &textureManager = game.getTextureManager();
	const std::optional<TextureFileMetadata> textureFileMetadata = textureManager.tryGetMetadata(this->sequenceFilename.c_str());
	if (!textureFileMetadata.has_value())
	{
		DebugLogError("Couldn't get texture file metadata for \"" + this->sequenceFilename + "\".");
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
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(this->paletteTextureAssetRef);
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for \"" + this->paletteTextureAssetRef.filename + "\".");
		return;
	}

	const TextureAssetReference currentTextureAssetRef = this->getCurrentSequenceTextureAssetRef();
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(currentTextureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugLogError("Couldn't get texture builder ID for \"" + currentTextureAssetRef.filename + "\" index " +
			std::to_string(*currentTextureAssetRef.index) + ".");
		return;
	}

	renderer.drawOriginal(*textureBuilderID, *paletteID, textureManager);
}
