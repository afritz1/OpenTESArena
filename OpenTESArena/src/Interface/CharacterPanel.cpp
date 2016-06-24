#include <cassert>

#include "SDL.h"

#include "CharacterPanel.h"

#include "Button.h"
#include "GameWorldPanel.h"
#include "TextBox.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/CharacterRace.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/GameState.h"
#include "../Math/Constants.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/PortraitFile.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

CharacterPanel::CharacterPanel(GameState *gameState)
	: Panel(gameState)
{
	this->playerNameTextBox = [gameState]()
	{
		auto origin = Int2(10, 8);
		auto color = Color(199, 199, 199);
		std::string text = gameState->getGameData()->getPlayer().getDisplayName();
		auto fontName = FontName::Arena;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.getX(),
			origin.getY(),
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->playerRaceTextBox = [gameState]()
	{
		auto origin = Int2(10, 17);
		auto color = Color(199, 199, 199);
		std::string text = CharacterRace(gameState->getGameData()->getPlayer()
			.getRaceName()).toString();
		auto fontName = FontName::Arena;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.getX(),
			origin.getY(),
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->playerClassTextBox = [gameState]()
	{
		auto origin = Int2(10, 26);
		auto color = Color(199, 199, 199);
		std::string text = gameState->getGameData()->getPlayer().getCharacterClass()
			.getDisplayName();
		auto fontName = FontName::Arena;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.getX(),
			origin.getY(),
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->doneButton = [gameState]()
	{
		auto center = Int2(25, ORIGINAL_HEIGHT - 15);
		int width = 21;
		int height = 12;
		auto function = [gameState]()
		{
			auto gamePanel = std::unique_ptr<Panel>(new GameWorldPanel(gameState));
			gameState->setPanel(std::move(gamePanel));
		};
		return std::unique_ptr<Button>(new Button(center, width, height, function));
	}();
}

CharacterPanel::~CharacterPanel()
{

}

void CharacterPanel::handleEvents(bool &running)
{
	auto mousePosition = this->getMousePosition();
	auto mouseOriginalPoint = this->nativePointToOriginal(mousePosition);

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
			this->doneButton->click();
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);

		if (leftClick)
		{
			// Check if "Done" was clicked.
			if (this->doneButton->containsPoint(mouseOriginalPoint))
			{
				this->doneButton->click();
			}

			// What other buttons should the character panel have?
			// - Inventory?
			// - Spell book?
			// - Active effects?
			// - Journal (or "log" like Daggerfall)?
			// - Time and date? Maybe that should remain as an in-game hotkey.
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

void CharacterPanel::render(SDL_Renderer *renderer, const SDL_Rect *letterbox)
{
	assert(this->getGameState()->gameDataIsActive());

	// Clear full screen.
	this->clearScreen(renderer);

	// Draw character stats background.
	const auto *statsBackground = this->getGameState()->getTextureManager()
		.getTexture(TextureFile::fromName(TextureName::CharacterStats));
	this->drawScaledToNative(statsBackground, renderer);

	// Get a reference to the active player data.
	const auto &player = this->getGameState()->getGameData()->getPlayer();

	// Get the filenames for the portraits.
	auto portraitStrings = PortraitFile::getGroup(player.getGenderName(),
		player.getRaceName(), player.getCharacterClass().canCastMagic());

	// Draw the player's portrait.
	const auto *portrait = this->getGameState()->getTextureManager()
		.getTexture(portraitStrings.at(player.getPortraitID()));
	int portraitWidth, portraitHeight;
	SDL_QueryTexture(const_cast<SDL_Texture*>(portrait), nullptr, nullptr, 
		&portraitWidth, &portraitHeight);

	this->drawScaledToNative(portrait,
		ORIGINAL_WIDTH - portraitWidth,
		0,
		portraitWidth,
		portraitHeight,
		renderer);

	// Draw text boxes: player name, race, class.
	this->drawScaledToNative(*this->playerClassTextBox.get(), renderer);
	this->drawScaledToNative(*this->playerNameTextBox.get(), renderer);
	this->drawScaledToNative(*this->playerRaceTextBox.get(), renderer);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, renderer);
}
