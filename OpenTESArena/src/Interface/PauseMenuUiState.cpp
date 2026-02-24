#include <algorithm>

#include "SDL.h"

#include "GameWorldUiView.h"
#include "GameWorldPanel.h"
#include "LoadSavePanel.h"
#include "MainMenuPanel.h"
#include "OptionsPanel.h"
#include "PauseMenuUiController.h"
#include "PauseMenuUiModel.h"
#include "PauseMenuUiState.h"
#include "PauseMenuUiView.h"
#include "../Audio/MusicLibrary.h"
#include "../Game/Game.h"
#include "../Player/Player.h"

namespace
{
	constexpr char InterfaceImageElementName[] = "PauseMenuInterfaceImage";
	constexpr char NoMagicImageElementName[] = "PauseMenuNoMagicImage";
	constexpr char StatusGradientImageElementName[] = "PauseMenuStatusGradientImage";
	constexpr char PortraitImageElementName[] = "PauseMenuPlayerPortraitImage";
	constexpr char StatusBarsImageElementName[] = "PauseMenuPlayerStatusBarsImage";
	constexpr char NameTextBoxElementName[] = "PauseMenuPlayerNameTextBox";

	constexpr char SoundTextBoxElementName[] = "PauseMenuSoundTextBox";
	constexpr char MusicTextBoxElementName[] = "PauseMenuMusicTextBox";

	constexpr char OptionsButtonImageElementName[] = "PauseMenuOptionsButtonImage";

	constexpr double VOLUME_MIN = 0.0;
	constexpr double VOLUME_MAX = 1.0;
	constexpr double VOLUME_DELTA = 0.10;
}

PauseMenuUiState::PauseMenuUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->statusBarsTextureID = -1;
	this->statusGradientTextureID = -1;
	this->playerPortraitTextureID = -1;
	this->optionsButtonTextureID = -1;
}

void PauseMenuUiState::init(Game &game)
{
	const Player &player = game.player;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	this->game = &game;

	this->statusBarsTextureID = GameWorldUiView::allocStatusBarsTexture(textureManager, renderer);
	this->statusGradientTextureID = GameWorldUiView::allocStatusGradientTexture(GameWorldUiView::StatusGradientType::Default, textureManager, renderer);
	this->playerPortraitTextureID = GameWorldUiView::allocPlayerPortraitTexture(player.male, player.raceID, player.portraitID, textureManager, renderer);
	this->optionsButtonTextureID = PauseMenuUiView::allocOptionsButtonTexture(textureManager, renderer);
}

void PauseMenuUiState::freeTextures(Renderer &renderer)
{
	if (this->statusBarsTextureID >= 0)
	{
		renderer.freeUiTexture(this->statusBarsTextureID);
		this->statusBarsTextureID = -1;
	}

	if (this->statusGradientTextureID >= 0)
	{
		renderer.freeUiTexture(this->statusGradientTextureID);
		this->statusGradientTextureID = -1;
	}

	if (this->playerPortraitTextureID >= 0)
	{
		renderer.freeUiTexture(this->playerPortraitTextureID);
		this->playerPortraitTextureID = -1;
	}

	if (this->optionsButtonTextureID >= 0)
	{
		renderer.freeUiTexture(this->optionsButtonTextureID);
		this->optionsButtonTextureID = -1;
	}
}

void PauseMenuUI::create(Game &game)
{
	PauseMenuUiState &state = PauseMenuUI::state;
	state.init(game);

	const Options &options = game.options;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(PauseMenuUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	UiElementInitInfo statusBarsImageElementInitInfo;
	statusBarsImageElementInitInfo.name = StatusBarsImageElementName;
	statusBarsImageElementInitInfo.position = GameWorldUiView::HealthBarRect.getBottomLeft();
	statusBarsImageElementInitInfo.pivotType = GameWorldUiView::StatusBarPivotType;
	statusBarsImageElementInitInfo.drawOrder = 2;
	uiManager.createImage(statusBarsImageElementInitInfo, state.statusBarsTextureID, state.contextInstID, renderer);

	const UiElementInstanceID statusGradientImageElementInstID = uiManager.getElementByName(StatusGradientImageElementName);
	uiManager.setImageTexture(statusGradientImageElementInstID, state.statusGradientTextureID);

	const UiElementInstanceID playerPortraitImageElementInstID = uiManager.getElementByName(PortraitImageElementName);
	uiManager.setImageTexture(playerPortraitImageElementInstID, state.playerPortraitTextureID);

	const std::string playerNameText = GameWorldUiModel::getPlayerNameText(game);
	const UiElementInstanceID playerNameTextBoxElementInstID = uiManager.getElementByName(NameTextBoxElementName);
	uiManager.setTextBoxText(playerNameTextBoxElementInstID, playerNameText.c_str());

	GameWorldUiView::updateStatusBarsTexture(state.statusBarsTextureID, game.player, renderer);

	PauseMenuUI::setSoundText(options.getAudio_SoundVolume());
	PauseMenuUI::setMusicText(options.getAudio_MusicVolume());

	const std::optional<Int2> cursorDims = renderer.tryGetUiTextureDims(game.defaultCursorTextureID);
	DebugAssert(cursorDims.has_value());

	const double cursorScale = options.getGraphics_CursorScale();
	const Int2 cursorSize(
		static_cast<int>(static_cast<double>(cursorDims->x) * cursorScale),
		static_cast<int>(static_cast<double>(cursorDims->y) * cursorScale));

	uiManager.setTransformSize(game.cursorImageElementInstID, cursorSize);
	uiManager.setTransformPivot(game.cursorImageElementInstID, UiPivotType::TopLeft);
	uiManager.setImageTexture(game.cursorImageElementInstID, game.defaultCursorTextureID);
}

void PauseMenuUI::destroy()
{
	PauseMenuUiState &state = PauseMenuUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	Renderer &renderer = game.renderer;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, renderer);
		state.contextInstID = -1;
	}

	state.freeTextures(renderer);
}

void PauseMenuUI::update(double dt)
{
	// Do nothing.
}

void PauseMenuUI::setSoundText(double volumePercent)
{
	PauseMenuUiState &state = PauseMenuUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID textBoxElementInstID = uiManager.getElementByName(SoundTextBoxElementName);
	const std::string volumeText = PauseMenuUiModel::getVolumeString(volumePercent);
	uiManager.setTextBoxText(textBoxElementInstID, volumeText.c_str());
}

void PauseMenuUI::setMusicText(double volumePercent)
{
	PauseMenuUiState &state = PauseMenuUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID musicTextBoxElementInstID = uiManager.getElementByName(MusicTextBoxElementName);
	const std::string volumeText = PauseMenuUiModel::getVolumeString(volumePercent);
	uiManager.setTextBoxText(musicTextBoxElementInstID, volumeText.c_str());
}

void PauseMenuUI::onNewGameButtonSelected(MouseButtonType mouseButtonType)
{
	PauseMenuUiState &state = PauseMenuUI::state;
	Game &game = *state.game;
	PauseMenuUiController::onNewGameButtonSelected(game);
}

void PauseMenuUI::onLoadGameButtonSelected(MouseButtonType mouseButtonType)
{
	PauseMenuUiState &state = PauseMenuUI::state;
	Game &game = *state.game;
	game.setPanel<LoadSavePanel>(LoadSavePanel::Type::Load);
}

void PauseMenuUI::onSaveGameButtonSelected(MouseButtonType mouseButtonType)
{
	// @todo
	// SaveGamePanel...
	//auto optionsPanel = std::make_unique<OptionsPanel>(game);
	//game.setPanel(std::move(optionsPanel));
}

void PauseMenuUI::onExitGameButtonSelected(MouseButtonType mouseButtonType)
{
	SDL_Event evt;
	evt.quit.type = SDL_QUIT;
	evt.quit.timestamp = 0;
	SDL_PushEvent(&evt);
}

void PauseMenuUI::onResumeGameButtonSelected(MouseButtonType mouseButtonType)
{
	PauseMenuUiState &state = PauseMenuUI::state;
	Game &game = *state.game;
	game.setPanel<GameWorldPanel>();
}

void PauseMenuUI::onOptionsButtonSelected(MouseButtonType mouseButtonType)
{
	PauseMenuUiState &state = PauseMenuUI::state;
	Game &game = *state.game;
	game.setPanel<OptionsPanel>();
}

void PauseMenuUI::onSoundUpButtonSelected(MouseButtonType mouseButtonType)
{
	PauseMenuUiState &state = PauseMenuUI::state;
	Game &game = *state.game;
	Options &options = game.options;
	options.setAudio_SoundVolume(std::min(options.getAudio_SoundVolume() + VOLUME_DELTA, VOLUME_MAX));

	AudioManager &audioManager = game.audioManager;
	audioManager.setSoundVolume(options.getAudio_SoundVolume());

	PauseMenuUI::setSoundText(options.getAudio_SoundVolume());
}

void PauseMenuUI::onSoundDownButtonSelected(MouseButtonType mouseButtonType)
{
	PauseMenuUiState &state = PauseMenuUI::state;
	Game &game = *state.game;
	Options &options = game.options;
	const double newVolume = [&options]()
	{
		const double volume = std::max(options.getAudio_SoundVolume() - VOLUME_DELTA, VOLUME_MIN);

		// Clamp very small values to zero to avoid precision issues with tiny numbers.
		return volume < Constants::Epsilon ? VOLUME_MIN : volume;
	}();

	options.setAudio_SoundVolume(newVolume);

	AudioManager &audioManager = game.audioManager;
	audioManager.setSoundVolume(options.getAudio_SoundVolume());

	PauseMenuUI::setSoundText(options.getAudio_SoundVolume());
}

void PauseMenuUI::onMusicUpButtonSelected(MouseButtonType mouseButtonType)
{
	PauseMenuUiState &state = PauseMenuUI::state;
	Game &game = *state.game;
	Options &options = game.options;
	options.setAudio_MusicVolume(std::min(options.getAudio_MusicVolume() + VOLUME_DELTA, VOLUME_MAX));

	AudioManager &audioManager = game.audioManager;
	audioManager.setMusicVolume(options.getAudio_MusicVolume());

	PauseMenuUI::setMusicText(options.getAudio_MusicVolume());
}

void PauseMenuUI::onMusicDownButtonSelected(MouseButtonType mouseButtonType)
{
	PauseMenuUiState &state = PauseMenuUI::state;
	Game &game = *state.game;
	Options &options = game.options;
	const double newVolume = [&options]()
	{
		const double volume = std::max(options.getAudio_MusicVolume() - VOLUME_DELTA, VOLUME_MIN);

		// Clamp very small values to zero to avoid precision issues with tiny numbers.
		return volume < Constants::Epsilon ? VOLUME_MIN : volume;
	}();

	options.setAudio_MusicVolume(newVolume);

	AudioManager &audioManager = game.audioManager;
	audioManager.setMusicVolume(options.getAudio_MusicVolume());

	PauseMenuUI::setMusicText(options.getAudio_MusicVolume());
}

void PauseMenuUI::onBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		PauseMenuUI::onResumeGameButtonSelected(MouseButtonType::Left);
	}
}
