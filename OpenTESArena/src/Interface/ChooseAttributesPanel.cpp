#include <cassert>
#include <iostream>

#include "SDL2\SDL.h"

#include "ChooseAttributesPanel.h"

#include "Button.h"
#include "ChooseRacePanel.h"
#include "GameWorldPanel.h"
#include "TextBox.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/EntityManager.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/MusicName.h"
#include "../Media/FontName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/CLProgram.h"

ChooseAttributesPanel::ChooseAttributesPanel(GameState *gameState, 
	CharacterGenderName gender, const CharacterClass &charClass,
	const std::string &name, CharacterRaceName raceName)
	: Panel(gameState)
{
	this->titleTextBox = nullptr;
	this->backToRaceButton = nullptr;
	this->acceptButton = nullptr;
	this->gender = nullptr;
	this->charClass = nullptr;
	this->raceName = nullptr;

	this->titleTextBox = [gameState]()
	{
		auto center = Int2(160, 100);
		auto color = Color::White;
		std::string text = "Thy attributes will soon be here.\n\nLeft click.";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->backToRaceButton = [gameState, gender, charClass, name]()
	{
		auto function = [gameState, gender, charClass, name]()
		{
			auto racePanel = std::unique_ptr<Panel>(new ChooseRacePanel(
				gameState, gender, charClass, name));
			gameState->setPanel(std::move(racePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	// Set the game world's music to be some default for now.
	// None of the gender, class name, etc. things are being sent to the game world
	// panel because I'm planning on putting them in the GameState instead.
	this->acceptButton = [gameState, name]()
	{
		auto function = [gameState, name]()
		{
			// Make placeholders here for the game data. They'll be more informed
			// in the future once the player has a place in the world and the options
			// menu has settings for the CLProgram.
			auto entityManager = std::unique_ptr<EntityManager>(new EntityManager());
			auto player = std::unique_ptr<Player>(new Player(name, Float3d(),
				Float3d(), Float3d(), *entityManager.get()));
			auto clProgram = std::unique_ptr<CLProgram>(new CLProgram(
				gameState->getScreenDimensions().getX(),
				gameState->getScreenDimensions().getY()));
			auto gameData = std::unique_ptr<GameData>(new GameData(
				std::move(player), std::move(entityManager), std::move(clProgram)));

			// Set the game data before constructing the game world panel.
			gameState->setGameData(std::move(gameData));

			auto gameWorldPanel = std::unique_ptr<Panel>(new GameWorldPanel(gameState));
			gameState->setMusic(MusicName::Magic);
			gameState->setPanel(std::move(gameWorldPanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->gender = std::unique_ptr<CharacterGenderName>(
		new CharacterGenderName(gender));
	this->charClass = std::unique_ptr<CharacterClass>(
		new CharacterClass(charClass));
	this->raceName = std::unique_ptr<CharacterRaceName>(
		new CharacterRaceName(raceName));
	this->name = name;
	
	assert(this->titleTextBox.get() != nullptr);
	assert(this->backToRaceButton.get() != nullptr);
	assert(this->acceptButton.get() != nullptr);
	assert(this->gender.get() != nullptr);
	assert(this->charClass.get() != nullptr);
	assert(this->raceName.get() != nullptr);
	assert(this->name == name);
}

ChooseAttributesPanel::~ChooseAttributesPanel()
{

}

void ChooseAttributesPanel::handleEvents(bool &running)
{
	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		bool applicationExit = (e.type == SDL_QUIT);
		bool resized = (e.type == SDL_WINDOWEVENT) &&
			(e.window.event == SDL_WINDOWEVENT_RESIZED);
		bool escapePressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_ESCAPE);

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
		if (escapePressed)
		{
			this->backToRaceButton->click();
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);

		if (leftClick)
		{
			this->acceptButton->click();
		}
	}
}

void ChooseAttributesPanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void ChooseAttributesPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void ChooseAttributesPanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
}

void ChooseAttributesPanel::render(SDL_Surface *dst, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(dst);
		
	// Draw temporary background. I don't have the marble background or character
	// portraits programmed in yet, but they will be, eventually.
	SDL_FillRect(dst, letterbox, SDL_MapRGB(dst->format, 24, 36, 24));

	// Draw text: title.
	this->drawScaledToNative(*this->titleTextBox.get(), dst);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, dst);
}
