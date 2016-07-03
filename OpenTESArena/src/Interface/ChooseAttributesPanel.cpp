#include <cassert>

#include "SDL.h"

#include "ChooseAttributesPanel.h"

#include "Button.h"
#include "ChooseRacePanel.h"
#include "GameWorldPanel.h"
#include "TextBox.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/CharacterRace.h"
#include "../Entities/EntityManager.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/GameState.h"
#include "../Math/Constants.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/MusicName.h"
#include "../Media/PaletteName.h"
#include "../Media/PortraitFile.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/CLProgram.h"
#include "../Rendering/Renderer.h"

ChooseAttributesPanel::ChooseAttributesPanel(GameState *gameState,
	const CharacterClass &charClass, const std::string &name, CharacterGenderName gender, 
	CharacterRaceName raceName)
	: Panel(gameState)
{
	this->nameTextBox = [gameState, name]()
	{
		Int2 origin(10, 8);
		Color color(199, 199, 199);
		std::string text = name;
		auto fontName = FontName::Arena;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.getX(),
			origin.getY(),
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->raceTextBox = [gameState, raceName]()
	{
		Int2 origin(10, 17);
		Color color(199, 199, 199);
		std::string text = CharacterRace(raceName).toString();
		auto fontName = FontName::Arena;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.getX(),
			origin.getY(),
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->classTextBox = [gameState, charClass]()
	{
		Int2 origin(10, 26);
		Color color(199, 199, 199);
		std::string text = charClass.getDisplayName();
		auto fontName = FontName::Arena;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.getX(),
			origin.getY(),
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->backToRaceButton = [charClass, name, gender]()
	{
		auto function = [charClass, name, gender](GameState *gameState)
		{
			std::unique_ptr<Panel> racePanel(new ChooseRacePanel(
				gameState, charClass, name, gender));
			gameState->setPanel(std::move(racePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->doneButton = [this, charClass, name, gender, raceName]()
	{
		Int2 center(25, ORIGINAL_HEIGHT - 15);
		int width = 21;
		int height = 12;
		auto function = [this, charClass, name, gender, raceName](GameState *gameState)
		{
			// Make placeholders here for the game data. They'll be more informed
			// in the future once the player has a place in the world and the options
			// menu has settings for the CLProgram.
			std::unique_ptr<EntityManager> entityManager(new EntityManager());

			Float3d position = Float3d(1.50, 1.70, 1.50); // Arbitrary player height.
			Float3d direction = Float3d(1.0, 0.0, 1.0).normalized();
			Float3d velocity = Float3d(0.0, 0.0, 0.0);
			
			// Some arbitrary max speeds.
			double maxWalkSpeed = 2.0;
			double maxRunSpeed = 8.0;

			std::unique_ptr<Player> player(new Player(name, gender, raceName,
				charClass, this->portraitIndex, position, direction, velocity,
				maxWalkSpeed, maxRunSpeed, *entityManager.get()));
			
			// Some arbitrary test dimensions.
			int worldWidth = 32;
			int worldHeight = 5;
			int worldDepth = 32;

			std::unique_ptr<CLProgram> clProgram (new CLProgram(
				gameState->getScreenDimensions().getX(),
				gameState->getScreenDimensions().getY(),
				gameState->getRenderer().getRenderer(),
				worldWidth, worldHeight, worldDepth));

			double gameTime = 0.0; // In seconds. Also affects sun position.
			std::unique_ptr<GameData> gameData(new GameData(
				std::move(player), std::move(entityManager), std::move(clProgram),
				gameTime, worldWidth, worldHeight, worldDepth));
			
			// Set the game data before constructing the game world panel.
			gameState->setGameData(std::move(gameData));

			std::unique_ptr<Panel> gameWorldPanel(new GameWorldPanel(gameState));
			gameState->setPanel(std::move(gameWorldPanel));
			gameState->setMusic(MusicName::Overcast);
		};
		return std::unique_ptr<Button>(new Button(center, width, height, function));
	}();

	this->incrementPortraitButton = [this]()
	{
		Int2 center(ORIGINAL_WIDTH - 72, 25);
		int width = 60;
		int height = 42;
		auto function = [this](GameState *gameState)
		{
			this->portraitIndex = (this->portraitIndex == 9) ? 0 : (this->portraitIndex + 1);
		};
		return std::unique_ptr<Button>(new Button(center, width, height, function));
	}();

	this->decrementPortraitButton = [this]()
	{
		Int2 center(ORIGINAL_WIDTH - 72, 25);
		int width = 60;
		int height = 42;
		auto function = [this](GameState *gameState)
		{
			this->portraitIndex = (this->portraitIndex == 0) ? 9 : (this->portraitIndex - 1);
		};
		return std::unique_ptr<Button>(new Button(center, width, height, function));
	}();

	this->gender = std::unique_ptr<CharacterGenderName>(new CharacterGenderName(gender));
	this->charClass = std::unique_ptr<CharacterClass>(new CharacterClass(charClass));
	this->raceName = std::unique_ptr<CharacterRaceName>(new CharacterRaceName(raceName));
	this->name = name;
	this->portraitIndex = 0;
}

ChooseAttributesPanel::~ChooseAttributesPanel()
{

}

void ChooseAttributesPanel::handleEvents(bool &running)
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
			this->backToRaceButton->click(this->getGameState());
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);
		bool rightClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_RIGHT);

		if (leftClick)
		{
			if (this->doneButton->containsPoint(mouseOriginalPoint))
			{
				this->doneButton->click(this->getGameState());
			}
			else if (this->incrementPortraitButton->containsPoint(mouseOriginalPoint))
			{
				this->incrementPortraitButton->click(this->getGameState());
			}
		}

		if (rightClick)
		{
			if (this->decrementPortraitButton->containsPoint(mouseOriginalPoint))
			{
				this->decrementPortraitButton->click(this->getGameState());
			}
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

void ChooseAttributesPanel::render(SDL_Renderer *renderer, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(renderer);

	// Draw attributes texture.
	auto &textureManager = this->getGameState()->getTextureManager();
	const auto *attributesBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::CharacterStats), PaletteName::CharSheet);
	this->drawScaledToNative(attributesBackground, renderer);

	// Get the filenames for the portraits.
	auto portraitStrings = PortraitFile::getGroup(*this->gender.get(),
		*this->raceName.get(), this->charClass->canCastMagic());

	// Draw the current portrait.
	const auto *portrait = textureManager.getTexture(
		portraitStrings.at(this->portraitIndex));
	int portraitWidth, portraitHeight;
	SDL_QueryTexture(const_cast<SDL_Texture*>(portrait), nullptr, nullptr, 
		&portraitWidth, &portraitHeight);

	this->drawScaledToNative(portrait,
		ORIGINAL_WIDTH - portraitWidth,
		0,
		portraitWidth,
		portraitHeight,
		renderer);

	// Draw text: name, race, class.
	this->drawScaledToNative(*this->nameTextBox.get(), renderer);
	this->drawScaledToNative(*this->raceTextBox.get(), renderer);
	this->drawScaledToNative(*this->classTextBox.get(), renderer);

	// Draw cursor.
	const auto &cursor = textureManager.getSurface(
		TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, renderer);
}
