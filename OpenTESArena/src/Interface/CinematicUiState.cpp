#include "CinematicUiState.h"
#include "../Game/Game.h"

namespace
{
	constexpr char ElementName_VideoImage[] = "VideoImage";
}

CinematicUiInitInfo::CinematicUiInitInfo()
{
	this->secondsPerImage = 0.0;
	this->callback = []() { };
}

void CinematicUiInitInfo::init(const std::string &paletteName, const std::string &sequenceName, double secondsPerImage, const CinematicFinishedCallback &callback)
{
	this->paletteName = paletteName;
	this->sequenceName = sequenceName;
	this->secondsPerImage = secondsPerImage;
	this->callback = callback;
}

CinematicUiState::CinematicUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->secondsPerImage = 0.0;
	this->currentSeconds = 0.0;
	this->imageIndex = -1;
	this->callback = []() { };
}

void CinematicUiState::init(Game &game)
{
	DebugAssert(this->initInfo.secondsPerImage > 0.0);

	this->game = &game;

	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;	
	const std::string &sequenceName = this->initInfo.sequenceName;
	const std::string &paletteName = this->initInfo.paletteName;
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(sequenceName.c_str());
	if (!metadataID.has_value())
	{
		DebugLogErrorFormat("Couldn't get texture file metadata for \"%s\".", sequenceName.c_str());
		return;
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	const TextureAsset paletteTextureAsset(paletteName);

	const int videoFrameCount = textureFileMetadata.getTextureCount();
	this->videoTextureIDs.init(videoFrameCount);
	for (int i = 0; i < videoFrameCount; i++)
	{
		const TextureAsset textureAsset(sequenceName, i);

		UiTextureID textureID;
		if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
		{
			DebugLogErrorFormat("Couldn't create UI texture for sequence \"%s\" frame %d.", sequenceName.c_str(), i);
			continue;
		}

		this->videoTextureIDs.set(i, textureID);
	}

	this->secondsPerImage = this->initInfo.secondsPerImage;
	this->currentSeconds = 0.0;
	this->imageIndex = 0;
	this->callback = this->initInfo.callback;
}

void CinematicUiState::freeTextures(Renderer &renderer)
{
	for (UiTextureID textureID : this->videoTextureIDs)
	{
		renderer.freeUiTexture(textureID);
	}

	this->videoTextureIDs.clear();
}

void CinematicUI::create(Game &game)
{
	CinematicUiState &state = CinematicUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(CinematicUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	UiElementInitInfo videoImageElementInitInfo;
	videoImageElementInitInfo.name = ElementName_VideoImage;
	uiManager.createImage(videoImageElementInitInfo, state.videoTextureIDs[0], state.contextInstID, renderer);

	game.setCursorOverride(std::nullopt);
	uiManager.setElementActive(game.cursorImageElementInstID, false);
}

void CinematicUI::destroy()
{
	CinematicUiState &state = CinematicUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	Renderer &renderer = game.renderer;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, renderer);
		state.contextInstID = -1;
	}

	state.freeTextures(renderer);
	state.secondsPerImage = 0.0;
	state.currentSeconds = 0.0;
	state.imageIndex = -1;
	state.callback = []() { };

	uiManager.setElementActive(game.cursorImageElementInstID, true);
}

void CinematicUI::update(double dt)
{
	CinematicUiState &state = CinematicUI::state;
	Game &game = *state.game;

	const UiTextureID prevImageTextureID = state.videoTextureIDs[state.imageIndex];
	state.currentSeconds += dt;
	while (state.currentSeconds > state.secondsPerImage)
	{
		state.currentSeconds -= state.secondsPerImage;
		state.imageIndex++;
	}

	const int textureCount = state.videoTextureIDs.getCount();
	if (state.imageIndex >= textureCount)
	{
		state.imageIndex = textureCount - 1;
		CinematicUI::onSkipButtonSelected(MouseButtonType::Left);
	}

	const UiTextureID currentImageTextureID = state.videoTextureIDs[state.imageIndex];
	if (currentImageTextureID != prevImageTextureID)
	{
		UiManager &uiManager = game.uiManager;
		const UiElementInstanceID videoImageElementInstID = uiManager.getElementByName(ElementName_VideoImage);
		uiManager.setImageTexture(videoImageElementInstID, currentImageTextureID);
	}
}

void CinematicUI::onSkipButtonSelected(MouseButtonType mouseButtonType)
{
	CinematicUiState &state = CinematicUI::state;
	state.callback();
}

void CinematicUI::onSkipInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		CinematicUI::onSkipButtonSelected(MouseButtonType::Left);
	}
}
