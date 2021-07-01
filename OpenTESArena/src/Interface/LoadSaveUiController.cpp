#include "LoadSaveUiController.h"
#include "MainMenuPanel.h"
#include "PauseMenuPanel.h"
#include "TextSubPanel.h"
#include "../Game/Game.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/ArenaFontName.h"
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

	Texture texture = TextureUtils::generate(TextureUtils::PatternType::Dark,
		textBoxInitInfo.rect.getWidth() + 10, textBoxInitInfo.rect.getHeight() + 10,
		game.getTextureManager(), game.getRenderer());

	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, popUpFunction, std::move(texture), center);
}

void LoadSaveUiController::onBackButtonSelected(Game &game)
{
	if (game.gameStateIsActive())
	{
		game.setPanel<PauseMenuPanel>();
	}
	else
	{
		game.setPanel<MainMenuPanel>();
	}
}
