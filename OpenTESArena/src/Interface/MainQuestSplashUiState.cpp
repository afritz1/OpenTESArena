#include "GameWorldUiState.h"
#include "MainQuestSplashUiModel.h"
#include "MainQuestSplashUiState.h"
#include "MainQuestSplashUiView.h"
#include "../Audio/MusicUtils.h"
#include "../Game/Game.h"

MainQuestSplashUiState::MainQuestSplashUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->provinceID = -1;
}

void MainQuestSplashUiState::init(Game &game)
{
	DebugAssert(this->provinceID >= 0); // Must already be set by fast travel logic.
	this->game = &game;
}

void MainQuestSplashUI::create(Game &game)
{
	MainQuestSplashUiState &state = MainQuestSplashUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(MainQuestSplashUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	UiElementInitInfo backgroundImageElementInitInfo;
	backgroundImageElementInitInfo.name = "MainQuestSplashBackgroundImage";

	const TextureAsset backgroundImageTextureAsset = MainQuestSplashUiView::getSplashTextureAsset(state.provinceID);
	const UiTextureID backgroundImageTextureID = uiManager.getOrAddTexture(backgroundImageTextureAsset, backgroundImageTextureAsset, textureManager, renderer);
	uiManager.createImage(backgroundImageElementInitInfo, backgroundImageTextureID, state.contextInstID, renderer);

	const std::string descriptionText = MainQuestSplashUiModel::getDungeonText(game, state.provinceID);
	const UiElementInstanceID descriptionTextBoxElementInstID = uiManager.getElementByName("MainQuestSplashTextBox");
	uiManager.setTextBoxText(descriptionTextBoxElementInstID, descriptionText.c_str());

	game.setCursorOverride(std::nullopt);
}

void MainQuestSplashUI::destroy()
{
	MainQuestSplashUiState &state = MainQuestSplashUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	Renderer &renderer = game.renderer;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, renderer);
		state.contextInstID = -1;
	}

	state.provinceID = -1;
}

void MainQuestSplashUI::update(double dt)
{
	// Do nothing.
}

void MainQuestSplashUI::onExitButtonSelected(MouseButtonType mouseButtonType)
{
	MainQuestSplashUiState &state = MainQuestSplashUI::state;
	Game &game = *state.game;

	const MusicDefinition *musicDef = MusicUtils::getRandomDungeonMusicDefinition(game.random);
	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing main quest dungeon music.");
	}

	AudioManager &audioManager = game.audioManager;
	audioManager.setMusic(musicDef);

	game.setNextContext(GameWorldUI::ContextName);
}

void MainQuestSplashUI::onBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		MainQuestSplashUI::onExitButtonSelected(MouseButtonType::Left);
	}
}
