#include <cassert>

#include "SDL2\SDL.h"

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
	this->doneTextBox = nullptr;
	this->playerClassTextBox = nullptr;
	this->playerNameTextBox = nullptr;
	this->playerRaceTextBox = nullptr;
	this->doneButton = nullptr;

	this->doneTextBox = [gameState]()
	{
		auto center = Int2(26, ORIGINAL_HEIGHT - 14);
		auto color = Color(190, 113, 0);
		std::string text = "Done";
		auto fontName = FontName::Char;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

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

	assert(this->doneTextBox.get() != nullptr);
	assert(this->playerClassTextBox.get() != nullptr);
	assert(this->playerNameTextBox.get() != nullptr);
	assert(this->playerRaceTextBox.get() != nullptr);
	assert(this->doneButton.get() != nullptr);
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

void CharacterPanel::render(SDL_Surface *dst, const SDL_Rect *letterbox)
{
	assert(this->getGameState()->gameDataIsActive());

	// Clear full screen.
	this->clearScreen(dst);

	// Draw temporary background.
	SDL_FillRect(dst, letterbox, SDL_MapRGB(dst->format, 24, 36, 36));

	// Get a reference to the active player data.
	const auto &player = this->getGameState()->getGameData()->getPlayer();

	// Get the filenames for the portraits.
	auto portraitStrings = PortraitFile::getGroup(player.getGenderName(),
		player.getRaceName(), player.getCharacterClass().canCastMagic());

	// Draw the player's portrait.
	const auto &portrait = this->getGameState()->getTextureManager()
		.getSurface(portraitStrings.at(player.getPortraitID()));

	this->drawScaledToNative(portrait,
		ORIGINAL_WIDTH - portrait.getWidth(),
		0,
		portrait.getWidth(),
		portrait.getHeight(),
		dst);

	// Draw buttons: done.
	this->drawScaledToNative(*this->doneButton.get(), dst);

	// Draw text boxes: player name, race, class, done.
	this->drawScaledToNative(*this->playerClassTextBox.get(), dst);
	this->drawScaledToNative(*this->playerNameTextBox.get(), dst);
	this->drawScaledToNative(*this->playerRaceTextBox.get(), dst);
	this->drawScaledToNative(*this->doneTextBox.get(), dst);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, dst);
}
