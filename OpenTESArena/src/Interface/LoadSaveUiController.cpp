#include "LoadSaveUiController.h"
#include "MainMenuPanel.h"
#include "PauseMenuPanel.h"
#include "TextSubPanel.h"
#include "../Game/Game.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"

void LoadSaveUiController::onEntryButtonSelected(Game &game, int index)
{
	// @temp: draw not implemented pop-up.
	const std::string text = "Not implemented\n(save slot " + std::to_string(index) + ")";
	const Int2 center(
		ArenaRenderUtils::SCREEN_WIDTH / 2,
		ArenaRenderUtils::SCREEN_HEIGHT / 2);

	const TextBox::InitInfo textBoxInitInfo = TextBox::InitInfo::makeWithCenter(
		text,
		center,
		ArenaFontName::Arena,
		Color(150, 97, 0),
		TextAlignment::MiddleCenter,
		std::nullopt,
		1,
		game.getFontLibrary());

	auto popUpFunction = [](Game &game)
	{
		game.popSubPanel();
	};

	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();
	const Surface surface = TextureUtils::generate(TextureUtils::PatternType::Dark,
		textBoxInitInfo.rect.getWidth() + 10, textBoxInitInfo.rect.getHeight() + 10,
		game.getTextureManager(), renderer);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTextureFromSurface(surface, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create non-implemented pop-up texture.");
	}

	ScopedUiTextureRef textureRef(textureID, renderer);
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, popUpFunction, std::move(textureRef), center);
}

void LoadSaveUiController::onBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		const GameState &gameState = game.getGameState();
		if (gameState.hasActiveMapInst()) // @todo: change this to something like "has a game session been started?". Voxels/entities/sky aren't animating but a game save is "active". What low-level system can we rely on? A non-empty scene?
		{
			game.setPanel<PauseMenuPanel>();
		}
		else
		{
			game.setPanel<MainMenuPanel>();
		}
	}
}
