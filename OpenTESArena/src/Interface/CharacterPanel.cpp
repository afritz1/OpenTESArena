#include <cassert>

#include "SDL.h"

#include "CharacterPanel.h"

#include "Button.h"
#include "CharacterEquipmentPanel.h"
#include "GameWorldPanel.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/CharacterRace.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/PortraitFile.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"

CharacterPanel::CharacterPanel(GameState *gameState)
	: Panel(gameState)
{
	this->playerNameTextBox = [gameState]()
	{
		Int2 origin(10, 8);
		Color color(199, 199, 199);
		std::string text = gameState->getGameData()->getPlayer().getDisplayName();
		auto &font = gameState->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Left;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.getX(),
			origin.getY(),
			color,
			text,
			font,
			alignment,
			gameState->getRenderer()));
	}();

	this->playerRaceTextBox = [gameState]()
	{
		Int2 origin(10, 17);
		Color color(199, 199, 199);
		std::string text = CharacterRace(gameState->getGameData()->getPlayer()
			.getRaceName()).toString();
		auto &font = gameState->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Left;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.getX(),
			origin.getY(),
			color,
			text,
			font,
			alignment,
			gameState->getRenderer()));
	}();

	this->playerClassTextBox = [gameState]()
	{
		Int2 origin(10, 26);
		Color color(199, 199, 199);
		std::string text = gameState->getGameData()->getPlayer().getCharacterClass()
			.getDisplayName();
		auto &font = gameState->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Left;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.getX(),
			origin.getY(),
			color,
			text,
			font,
			alignment,
			gameState->getRenderer()));
	}();

	this->doneButton = []()
	{
		Int2 center(25, Renderer::ORIGINAL_HEIGHT - 15);
		int width = 21;
		int height = 13;
		auto function = [](GameState *gameState)
		{
			std::unique_ptr<Panel> gamePanel(new GameWorldPanel(gameState));
			gameState->setPanel(std::move(gamePanel));
		};
		return std::unique_ptr<Button>(new Button(center, width, height, function));
	}();

	this->nextPageButton = []()
	{
		int x = 108;
		int y = 179;
		int width = 49;
		int height = 13;
		auto function = [](GameState *gameState)
		{
			std::unique_ptr<Panel> equipmentPanel(new CharacterEquipmentPanel(gameState));
			gameState->setPanel(std::move(equipmentPanel));
		};
		return std::unique_ptr<Button>(new Button(x, y, width, height, function));
	}();
}

CharacterPanel::~CharacterPanel()
{

}

void CharacterPanel::handleEvents(bool &running)
{
	auto mousePosition = this->getMousePosition();
	auto mouseOriginalPoint = this->getGameState()->getRenderer()
		.nativePointToOriginal(mousePosition);

	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		bool applicationExit = (e.type == SDL_QUIT);
		bool resized = (e.type == SDL_WINDOWEVENT) &&
			(e.window.event == SDL_WINDOWEVENT_RESIZED);
		bool escapePressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_ESCAPE);
		bool tabPressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_TAB);

		if (applicationExit)
		{
			running = false;
		}
		if (resized)
		{
			int width = e.window.data1;
			int height = e.window.data2;
			this->getGameState()->resizeWindow(width, height);
		}
		if (escapePressed || tabPressed)
		{
			this->doneButton->click(this->getGameState());
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);

		if (leftClick)
		{
			if (this->doneButton->containsPoint(mouseOriginalPoint))
			{
				this->doneButton->click(this->getGameState());
			}

			else if (this->nextPageButton->containsPoint(mouseOriginalPoint))
			{
				this->nextPageButton->click(this->getGameState());
			}
		}
	}
}

void CharacterPanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void CharacterPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void CharacterPanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
}

void CharacterPanel::render(Renderer &renderer)
{
	assert(this->getGameState()->gameDataIsActive());

	// Clear full screen.
	renderer.clearNative();

	// Set palette.
	auto &textureManager = this->getGameState()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::CharSheet));

	// Draw character stats background.
	auto *statsBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::CharacterStats));
	renderer.drawToOriginal(statsBackground);

	// Draw "Next Page" texture.
	auto *nextPageTexture = textureManager.getTexture(
		TextureFile::fromName(TextureName::NextPage));
	renderer.drawToOriginal(nextPageTexture, 108, 179);

	// Get a reference to the active player data.
	const auto &player = this->getGameState()->getGameData()->getPlayer();

	// Get the filenames for the portraits.
	auto portraitStrings = PortraitFile::getGroup(player.getGenderName(),
		player.getRaceName(), player.getCharacterClass().canCastMagic());

	// Draw the player's portrait.
	auto *portrait = textureManager.getTexture(
		portraitStrings.at(player.getPortraitID()));
	int portraitWidth, portraitHeight;
	SDL_QueryTexture(portrait, nullptr, nullptr, &portraitWidth, &portraitHeight);
	renderer.drawToOriginal(portrait, Renderer::ORIGINAL_WIDTH - portraitWidth, 0);

	// Draw text boxes: player name, race, class.
	renderer.drawToOriginal(this->playerNameTextBox->getTexture(),
		this->playerNameTextBox->getX(), this->playerNameTextBox->getY());
	renderer.drawToOriginal(this->playerRaceTextBox->getTexture(),
		this->playerRaceTextBox->getX(), this->playerRaceTextBox->getY());
	renderer.drawToOriginal(this->playerClassTextBox->getTexture(),
		this->playerClassTextBox->getX(), this->playerClassTextBox->getY());

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();

	// Draw cursor.
	const auto &cursor = textureManager.getSurface(
		TextureFile::fromName(TextureName::SwordCursor));
	SDL_SetColorKey(cursor.getSurface(), SDL_TRUE,
		renderer.getFormattedARGB(Color::Black));
	const auto mousePosition = this->getMousePosition();
	renderer.drawToNative(cursor.getSurface(),
		mousePosition.getX(), mousePosition.getY(),
		static_cast<int>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int>(cursor.getHeight() * this->getCursorScale()));
}
