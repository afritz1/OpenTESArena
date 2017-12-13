#include <cassert>

#include "SDL.h"

#include "CursorAlignment.h"
#include "LoadGamePanel.h"
#include "MainMenuPanel.h"
#include "PauseMenuPanel.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/MusicName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"

LoadGamePanel::LoadGamePanel(Game &game)
	: Panel(game)
{
	this->backButton = []()
	{
		auto function = [](Game &game)
		{
			// Back button behavior depends on whether game data is active.
			if (game.gameDataIsActive())
			{
				game.setPanel<PauseMenuPanel>(game);
			}
			else
			{
				game.setPanel<MainMenuPanel>(game);
			}
		};
		return Button<Game&>(function);
	}();
}

LoadGamePanel::~LoadGamePanel()
{

}

std::pair<SDL_Texture*, CursorAlignment> LoadGamePanel::getCurrentCursor() const
{
	auto &textureManager = this->getGame().getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default));
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
}

void LoadGamePanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);

	if (escapePressed)
	{
		this->backButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		// Listen for up/down arrow click, saved game click...
	}
}

void LoadGamePanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Set palette.
	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw slots background.
	const auto &slotsBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::LoadSave));
	renderer.drawOriginal(slotsBackground.get());
}
