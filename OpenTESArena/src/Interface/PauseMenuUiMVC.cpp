#include <cmath>

#include "MainMenuUiState.h"
#include "PauseMenuUiMVC.h"
#include "../Assets/TextureUtils.h"
#include "../Audio/MusicLibrary.h"
#include "../Game/Game.h"
#include "../Rendering/Renderer.h"
#include "../UI/Surface.h"

std::string PauseMenuUiModel::getVolumeString(double percent)
{
	const int volumeInteger = static_cast<int>(std::round(percent * 100.0));
	return std::to_string(volumeInteger);
}

UiTextureID PauseMenuUiView::allocOptionsButtonTexture(TextureManager &textureManager, Renderer &renderer)
{
	constexpr int buttonWidth = 145;
	constexpr int buttonHeight = 15;
	const UiTextureID textureID = renderer.createUiTexture(buttonWidth, buttonHeight);
	if (textureID < 0)
	{
		DebugLogError("Couldn't create options button texture for pause menu.");
		return -1;
	}

	LockedTexture lockedTexture = renderer.lockUiTexture(textureID);
	if (!lockedTexture.isValid())
	{
		DebugLogError("Couldn't lock options button texture for writing.");
		return -1;
	}

	Span2D<uint32_t> dstTexels = lockedTexture.getTexels32();
	TextureUtils::populateGeneratedTexture(UiTexturePatternType::Custom1, dstTexels, textureManager, renderer);
	renderer.unlockUiTexture(textureID);

	return textureID;
}

void PauseMenuUiController::onNewGameButtonSelected(Game &game)
{
	GameState &gameState = game.gameState;
	gameState.clearSession();

	game.setNextContext(MainMenuUI::ContextName);

	const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
	const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(MusicType::MainMenu, game.random);
	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing main menu music.");
	}

	AudioManager &audioManager = game.audioManager;
	audioManager.setMusic(musicDef);
}
