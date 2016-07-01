#include <cassert>

#include "SDL.h"

#include "CharacterEquipmentPanel.h"

#include "Button.h"
#include "CharacterPanel.h"
#include "TextBox.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/CharacterRace.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/GameState.h"
#include "../Math/Constants.h"
#include "../Math/Int2.h"
#include "../Media/FontName.h"
#include "../Media/PortraitFile.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

CharacterEquipmentPanel::CharacterEquipmentPanel(GameState *gameState)
	: Panel(gameState)
{
	this->playerNameTextBox = [gameState]()
	{
		int x = 10;
		int y = 8;
		Color color(199, 199, 199);
		std::string text = gameState->getGameData()->getPlayer().getDisplayName();
		auto fontName = FontName::Arena;
		return std::unique_ptr<TextBox>(new TextBox(
			x,
			y,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->playerRaceTextBox = [gameState]()
	{
		int x = 10;
		int y = 17;
		Color color(199, 199, 199);
		std::string text = CharacterRace(gameState->getGameData()->getPlayer()
			.getRaceName()).toString();
		auto fontName = FontName::Arena;
		return std::unique_ptr<TextBox>(new TextBox(
			x,
			y,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->playerClassTextBox = [gameState]()
	{
		int x = 10;
		int y = 26;
		Color color(199, 199, 199);
		std::string text = gameState->getGameData()->getPlayer().getCharacterClass()
			.getDisplayName();
		auto fontName = FontName::Arena;
		return std::unique_ptr<TextBox>(new TextBox(
			x,
			y,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->backToStatsButton = []()
	{
		int x = 0;
		int y = 188;
		int width = 47;
		int height = 12;
		auto function = [](GameState *gameState)
		{
			std::unique_ptr<Panel> characterPanel(new CharacterPanel(gameState));
			gameState->setPanel(std::move(characterPanel));
		};
		return std::unique_ptr<Button>(new Button(x, y, width, height, function));
	}();

	this->spellbookButton = []()
	{
		int x = 47;
		int y = 188;
		int width = 76;
		int height = 12;
		auto function = [](GameState *gameState)
		{
			// Nothing yet.
		};
		return std::unique_ptr<Button>(new Button(x, y, width, height, function));
	}();

	this->dropButton = []()
	{
		int x = 123;
		int y = 188;
		int width = 48;
		int height = 12;
		auto function = [](GameState *gameState)
		{
			// Nothing yet.
		};
		return std::unique_ptr<Button>(new Button(x, y, width, height, function));
	}();

	this->scrollDownButton = []()
	{
		Int2 center(16, 131);
		int width = 9;
		int height = 9;
		auto function = [](GameState *gameState)
		{
			// Nothing yet.
		};
		return std::unique_ptr<Button>(new Button(center, width, height, function));
	}();

	this->scrollUpButton = []()
	{
		Int2 center(152, 131);
		int width = 9;
		int height = 9;
		auto function = [](GameState *gameState)
		{
			// Nothing yet.
		};
		return std::unique_ptr<Button>(new Button(center, width, height, function));
	}();
}

CharacterEquipmentPanel::~CharacterEquipmentPanel()
{

}

void CharacterEquipmentPanel::handleEvents(bool &running)
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
			this->backToStatsButton->click(this->getGameState());
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);

		if (leftClick)
		{
			if (this->backToStatsButton->containsPoint(mouseOriginalPoint))
			{
				this->backToStatsButton->click(this->getGameState());
			}
			else if (this->spellbookButton->containsPoint(mouseOriginalPoint))
			{
				this->spellbookButton->click(this->getGameState());
			}
			else if (this->dropButton->containsPoint(mouseOriginalPoint))
			{
				this->dropButton->click(this->getGameState());
			}
			else if (this->scrollUpButton->containsPoint(mouseOriginalPoint))
			{
				this->scrollUpButton->click(this->getGameState());
			}
			else if (this->scrollDownButton->containsPoint(mouseOriginalPoint))
			{
				this->scrollDownButton->click(this->getGameState());
			}
		}
	}
}

void CharacterEquipmentPanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void CharacterEquipmentPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void CharacterEquipmentPanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
}

void CharacterEquipmentPanel::render(SDL_Renderer *renderer, const SDL_Rect *letterbox)
{
	assert(this->getGameState()->gameDataIsActive());

	// Clear full screen.
	this->clearScreen(renderer);

	// Draw character equipment background.
	this->getGameState()->getTextureManager().setPalette("CHARSHT.COL");
	const auto *equipmentBackground = this->getGameState()->getTextureManager()
		.getTexture(TextureFile::fromName(TextureName::CharacterEquipment));
	this->drawScaledToNative(equipmentBackground, renderer);
	this->getGameState()->getTextureManager().setPalette("PAL.COL");

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
