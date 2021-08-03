#include "CinematicPanel.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../UI/Texture.h"

CinematicPanel::CinematicPanel(Game &game)
	: Panel(game) { }

CinematicPanel::~CinematicPanel()
{
	auto &inputManager = this->getGame().getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::Cinematic, false);
}

bool CinematicPanel::init(const std::string &paletteName, const std::string &sequenceName,
	double secondsPerImage, const OnFinishedFunction &onFinished)
{
	auto &game = this->getGame();
	auto &inputManager = game.getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::Cinematic, true);

	this->addInputActionListener(InputActionName::Skip,
		[this](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			auto &game = values.game;
			this->onFinished(game);
		}
	});

	this->onFinished = onFinished;
	this->paletteTextureAssetRef = TextureAssetReference(std::string(paletteName));
	this->sequenceFilename = sequenceName;
	this->secondsPerImage = secondsPerImage;
	this->currentSeconds = 0.0;
	this->imageIndex = 0;
	return true;
}

TextureAssetReference CinematicPanel::getCurrentSequenceTextureAssetRef()
{
	return TextureAssetReference(std::string(this->sequenceFilename), this->imageIndex);
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
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(this->sequenceFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugLogError("Couldn't get texture file metadata for \"" + this->sequenceFilename + "\".");
		return;
	}

	// If at the end, then prepare for the next panel.
	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	const int textureCount = textureFileMetadata.getTextureCount();
	if (this->imageIndex >= textureCount)
	{
		this->imageIndex = textureCount - 1;
		this->onFinished(game);
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
