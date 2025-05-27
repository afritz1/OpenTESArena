#include "LoadSaveUiController.h"
#include "MainMenuPanel.h"
#include "PauseMenuPanel.h"
#include "TextSubPanel.h"
#include "../Game/Game.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/FontLibrary.h"
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

	const TextBoxInitInfo textBoxInitInfo = TextBoxInitInfo::makeWithCenter(
		text,
		center,
		ArenaFontName::Arena,
		Color(150, 97, 0),
		TextAlignment::MiddleCenter,
		std::nullopt,
		1,
		FontLibrary::getInstance());

	auto popUpFunction = [](Game &game)
	{
		game.popSubPanel();
	};

	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;
	const Surface surface = TextureUtils::generate(UiTexturePatternType::Dark,
		textBoxInitInfo.rect.width + 10, textBoxInitInfo.rect.height + 10,
		game.textureManager, renderer);

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
		const GameState &gameState = game.gameState;
		if (gameState.isActiveMapValid())
		{
			game.setPanel<PauseMenuPanel>();
		}
		else
		{
			game.setPanel<MainMenuPanel>();
		}
	}
}
