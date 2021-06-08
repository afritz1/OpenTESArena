#include "LoadSaveUiController.h"
#include "MainMenuPanel.h"
#include "PauseMenuPanel.h"
#include "TextSubPanel.h"
#include "../Game/Game.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/FontName.h"
#include "../UI/RichTextString.h"
#include "../UI/TextAlignment.h"

void LoadSaveUiController::onEntryButtonSelected(Game &game, int index)
{
	// @temp: draw not implemented pop-up.
	const Int2 center(
		ArenaRenderUtils::SCREEN_WIDTH / 2,
		ArenaRenderUtils::SCREEN_HEIGHT / 2);

	const int lineSpacing = 1;
	const RichTextString richText(
		"Not implemented\n(save slot " + std::to_string(index) + ")",
		FontName::Arena,
		Color(150, 97, 0),
		TextAlignment::Center,
		lineSpacing,
		game.getFontLibrary());

	auto popUpFunction = [](Game &game)
	{
		game.popSubPanel();
	};

	Texture texture = TextureUtils::generate(TextureUtils::PatternType::Dark,
		richText.getDimensions().x + 10, richText.getDimensions().y + 10,
		game.getTextureManager(), game.getRenderer());

	game.pushSubPanel<TextSubPanel>(center, richText, popUpFunction, std::move(texture), center);
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
