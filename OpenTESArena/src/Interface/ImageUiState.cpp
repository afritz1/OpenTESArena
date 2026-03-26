#include "ImageUiState.h"
#include "../Game/Game.h"

ImageUiInitInfo::ImageUiInitInfo()
{
	this->secondsToDisplay = 0.0;
	this->callback = []() { };
}

void ImageUiInitInfo::init(const std::string &paletteName, const std::string &textureName, double secondsToDisplay, const ImageFinishedCallback &callback)
{
	DebugAssert(secondsToDisplay > 0.0);
	this->paletteName = paletteName;
	this->textureName = textureName;
	this->secondsToDisplay = secondsToDisplay;
	this->callback = callback;
}

ImageUiState::ImageUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->secondsToDisplay = 0.0;
	this->currentSeconds = 0.0;
	this->callback = []() { };
}

void ImageUiState::init(Game &game)
{
	DebugAssert(this->initInfo.secondsToDisplay > 0.0);
	this->game = &game;
	this->secondsToDisplay = this->initInfo.secondsToDisplay;
	this->currentSeconds = 0.0;
	this->callback = this->initInfo.callback;
}

void ImageUI::create(Game &game)
{
	ImageUiState &state = ImageUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(ImageUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	const TextureAsset textureTextureAsset = TextureAsset(std::string(state.initInfo.textureName));
	const TextureAsset paletteTextureAsset = TextureAsset(std::string(state.initInfo.paletteName));
	const UiTextureID imageTextureID = uiManager.getOrAddTexture(textureTextureAsset, paletteTextureAsset, textureManager, renderer);

	UiElementInitInfo imageElementInitInfo;
	imageElementInitInfo.name = "Image";
	uiManager.createImage(imageElementInitInfo, imageTextureID, state.contextInstID, renderer);

	game.setCursorOverride(std::nullopt);
	uiManager.setElementActive(game.cursorImageElementInstID, false);
}

void ImageUI::destroy()
{
	ImageUiState &state = ImageUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	Renderer &renderer = game.renderer;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, renderer);
		state.contextInstID = -1;
	}

	state.secondsToDisplay = 0.0;
	state.currentSeconds = 0.0;
	state.callback = []() { };

	uiManager.setElementActive(game.cursorImageElementInstID, true);
}

void ImageUI::update(double dt)
{
	ImageUiState &state = ImageUI::state;
	state.currentSeconds += dt;
	if (state.currentSeconds > state.secondsToDisplay)
	{
		ImageUI::onSkipButtonSelected(MouseButtonType::Left);
	}
}

void ImageUI::onSkipButtonSelected(MouseButtonType mouseButtonType)
{
	ImageUiState &state = ImageUI::state;
	state.callback();
}

void ImageUI::onSkipInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ImageUI::onSkipButtonSelected(MouseButtonType::Left);
	}
}
