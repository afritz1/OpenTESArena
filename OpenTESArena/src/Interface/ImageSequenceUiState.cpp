#include <algorithm>

#include "ImageSequenceUiState.h"
#include "../Game/Game.h"

namespace
{
	constexpr char ElementName_ImageSequence[] = "ImageSequence";
}

ImageSequenceUiInitInfo::ImageSequenceUiInitInfo()
{
	this->callback = []() { };
}

void ImageSequenceUiInitInfo::init(Span<const std::string> paletteNames, Span<const std::string> textureNames, Span<const double> imageDurations, const ImageSequenceFinishedCallback &callback)
{
	DebugAssert(paletteNames.getCount() > 0);

	this->paletteNames.init(paletteNames.getCount());
	std::copy(paletteNames.begin(), paletteNames.end(), this->paletteNames.begin());

	this->textureNames.init(textureNames.getCount());
	std::copy(textureNames.begin(), textureNames.end(), this->textureNames.begin());

	this->imageDurations.init(imageDurations.getCount());
	std::copy(imageDurations.begin(), imageDurations.end(), this->imageDurations.begin());

	this->callback = callback;
}

ImageSequenceUiState::ImageSequenceUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->currentSeconds = 0.0;
	this->imageIndex = -1;
	this->callback = []() { };
}

void ImageSequenceUiState::init(Game &game)
{
	DebugAssert(this->initInfo.paletteNames.getCount() > 0);
	this->game = &game;

	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	this->textureIDs.init(this->initInfo.paletteNames.getCount());
	for (int i = 0; i < this->initInfo.paletteNames.getCount(); i++)
	{
		const std::string &textureName = this->initInfo.textureNames[i];
		const std::string &paletteName = this->initInfo.paletteNames[i];
		const TextureAsset textureAsset(textureName);
		const TextureAsset paletteTextureAsset(paletteName);

		UiTextureID textureID;
		if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
		{
			DebugLogErrorFormat("Couldn't create texture for image %d from \"%s\" with palette \"%s\".", i, textureName.c_str(), paletteName.c_str());
			continue;
		}

		this->textureIDs[i] = textureID;
	}

	this->imageDurations.init(this->initInfo.imageDurations.getCount());
	std::copy(this->initInfo.imageDurations.begin(), this->initInfo.imageDurations.end(), this->imageDurations.begin());

	this->currentSeconds = 0.0;
	this->imageIndex = 0;
	this->callback = this->initInfo.callback;
}

void ImageSequenceUiState::freeTextures(Renderer &renderer)
{
	if (this->textureIDs.isValid())
	{
		for (UiTextureID textureID : this->textureIDs)
		{
			renderer.freeUiTexture(textureID);
		}

		this->textureIDs.clear();
	}
}

void ImageSequenceUI::create(Game &game)
{
	ImageSequenceUiState &state = ImageSequenceUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(ImageSequenceUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	UiElementInitInfo imageElementInitInfo;
	imageElementInitInfo.name = ElementName_ImageSequence;
	uiManager.createImage(imageElementInitInfo, state.textureIDs[0], state.contextInstID, renderer);

	game.setCursorOverride(std::nullopt);
	uiManager.setElementActive(game.cursorImageElementInstID, false);
}

void ImageSequenceUI::destroy()
{
	ImageSequenceUiState &state = ImageSequenceUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	Renderer &renderer = game.renderer;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, renderer);
		state.contextInstID = -1;
	}

	state.imageDurations.clear();
	state.currentSeconds = 0.0;
	state.imageIndex = -1;
	state.callback = []() { };

	uiManager.setElementActive(game.cursorImageElementInstID, true);
}

void ImageSequenceUI::update(double dt)
{
	ImageSequenceUiState &state = ImageSequenceUI::state;
	Game &game = *state.game;

	const int imageCount = state.textureIDs.getCount();
	const UiTextureID prevTextureID = state.textureIDs[state.imageIndex];

	// Check if done iterating through images.
	if (state.imageIndex < imageCount)
	{
		state.currentSeconds += dt;

		if (state.currentSeconds >= state.imageDurations[state.imageIndex])
		{
			state.currentSeconds = 0.0;
			state.imageIndex++;

			if (state.imageIndex == imageCount)
			{
				state.callback();
			}
		}
	}

	state.imageIndex = std::min(state.imageIndex, imageCount - 1);
	const UiTextureID currentTextureID = state.textureIDs[state.imageIndex];

	if (currentTextureID != prevTextureID)
	{
		UiManager &uiManager = game.uiManager;
		const UiElementInstanceID imageElementInstID = uiManager.getElementByName(ElementName_ImageSequence);
		uiManager.setImageTexture(imageElementInstID, currentTextureID);
	}
}

void ImageSequenceUI::onSkipButtonSelected(MouseButtonType mouseButtonType)
{
	ImageSequenceUiState &state = ImageSequenceUI::state;
	Game &game = *state.game;

	const int imageCount = state.textureIDs.getCount();

	state.currentSeconds = 0.0;
	state.imageIndex++;
	if (state.imageIndex == imageCount)
	{
		state.callback();
		return;
	}

	const UiTextureID currentTextureID = state.textureIDs[state.imageIndex];

	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID imageElementInstID = uiManager.getElementByName(ElementName_ImageSequence);
	uiManager.setImageTexture(imageElementInstID, currentTextureID);
}

void ImageSequenceUI::onSkipInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ImageSequenceUiState &state = ImageSequenceUI::state;
		state.callback();
	}
}
