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
	constexpr Rect buttonRect(162, 88, 145, 15);
	const Surface surface = TextureUtils::generate(UiTexturePatternType::Custom1, buttonRect.width, buttonRect.height, textureManager, renderer);

	Span2D<const uint32_t> pixels = surface.getPixels();
	const UiTextureID textureID = renderer.createUiTexture(pixels.getWidth(), pixels.getHeight());
	if (textureID < 0)
	{
		DebugLogError("Couldn't create options button texture for pause menu.");
		return -1;
	}

	if (!renderer.populateUiTextureNoPalette(textureID, pixels))
	{
		DebugLogError("Couldn't populate options button texture for pause menu.");
	}

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
