#include "CinematicPanel.h"
#include "../Assets/TextureManager.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/Texture.h"

CinematicPanel::CinematicPanel(Game &game)
	: Panel(game) { }

bool CinematicPanel::init(const std::string &paletteName, const std::string &sequenceName,
	double secondsPerImage, const OnFinishedFunction &onFinished)
{
	auto &game = this->getGame();

	this->skipButton = Button<Game&>(
		0,
		0,
		ArenaRenderUtils::SCREEN_WIDTH,
		ArenaRenderUtils::SCREEN_HEIGHT,
		onFinished);

	this->addButtonProxy(MouseButtonType::Left, this->skipButton.getRect(),
		[this, &game]() { this->skipButton.click(game); });

	this->addInputActionListener(InputActionName::Skip,
		[this](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			auto &game = values.game;
			this->skipButton.click(game);
		}
	});

	auto &textureManager = game.textureManager;
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(sequenceName.c_str());
	if (!metadataID.has_value())
	{
		DebugLogError("Couldn't get texture file metadata for \"" + sequenceName + "\".");
		return false;
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	const TextureAsset paletteTextureAsset = TextureAsset(std::string(paletteName));

	auto &renderer = game.renderer;
	this->textureRefs.init(textureFileMetadata.getTextureCount());
	for (int i = 0; i < textureFileMetadata.getTextureCount(); i++)
	{
		const TextureAsset textureAsset = TextureAsset(std::string(sequenceName), i);

		UiTextureID textureID;
		if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
		{
			DebugLogError("Couldn't create UI texture for sequence \"" + sequenceName + "\" frame " + std::to_string(i) + ".");
			return false;
		}

		this->textureRefs.set(i, ScopedUiTextureRef(textureID, renderer));
	}

	UiDrawCallInitInfo drawCallInitInfo;
	drawCallInitInfo.textureFunc = [this]()
	{
		const ScopedUiTextureRef &textureRef = this->textureRefs.get(this->imageIndex);
		return textureRef.get();
	};

	drawCallInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	this->addDrawCall(drawCallInitInfo);
	
	this->secondsPerImage = secondsPerImage;
	this->currentSeconds = 0.0;
	this->imageIndex = 0;
	return true;
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

	// If at the end, then prepare for the next panel.
	const int textureCount = this->textureRefs.getCount();
	if (this->imageIndex >= textureCount)
	{
		this->imageIndex = textureCount - 1;
		this->skipButton.click(this->getGame());
	}
}
