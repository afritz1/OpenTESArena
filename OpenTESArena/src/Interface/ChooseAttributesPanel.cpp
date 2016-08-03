#include <cassert>

#include "SDL.h"

#include "ChooseAttributesPanel.h"

#include "Button.h"
#include "ChooseRacePanel.h"
#include "GameWorldPanel.h"
#include "TextBox.h"
#include "TextCinematicPanel.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/CharacterRace.h"
#include "../Entities/EntityManager.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/GameState.h"
#include "../Game/Options.h"
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
#include "../Media/TextureSequenceName.h"
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
			gameState->getTextureManager(),
			gameState->getRenderer()));
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
			gameState->getTextureManager(),
			gameState->getRenderer()));
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
			gameState->getTextureManager(),
			gameState->getRenderer()));
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

		auto gameDataFunction = [this, charClass, name, gender, raceName](GameState *gameState)
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

			std::unique_ptr<CLProgram> clProgram(new CLProgram(
				worldWidth, worldHeight, worldDepth,
				gameState->getTextureManager(),
				gameState->getRenderer(),
				gameState->getOptions().getRenderQuality()));

			double gameTime = 0.0; // In seconds. Also affects sun position.
			std::unique_ptr<GameData> gameData(new GameData(
				std::move(player), std::move(entityManager), std::move(clProgram),
				gameTime, worldWidth, worldHeight, worldDepth));

			// Set the game data before constructing the game world panel.
			gameState->setGameData(std::move(gameData));
		};

		auto gameFunction = [](GameState *gameState)
		{
			std::unique_ptr<Panel> gameWorldPanel(new GameWorldPanel(gameState));
			gameState->setPanel(std::move(gameWorldPanel));
			gameState->setMusic(MusicName::Overcast);
		};

		auto cinematicFunction = [gameFunction, gameDataFunction](GameState *gameState)
		{
			gameDataFunction(gameState);

			// The original game wraps text onto the next screen if the player's name is
			// too long. For example, it causes "listen to me" to go down one line and 
			// "Imperial Battle" to go onto the next screen, which then pushes the text
			// for every subsequent screen forward by a little bit.
			std::string silmaneText;

			// Use the player's first name.
			std::string playerName = gameState->getGameData()->getPlayer().getFirstName();

			// Load this text from the original data eventually. It's at #1400 in TEMPLATE.DAT.
			// Do a string replace of the %pcf identifier with the player's name.
			silmaneText.append("Do not fear for it is I, Ria Silmane. " + playerName + ", listen to me,\n");
			silmaneText.append("there are no others left to carry on this fight. You have\n");
			silmaneText.append("been left in this cell to die. Jagar Tharn, Imperial Battle\n");
			silmaneText.append("Mage of Tamriel has taken on the guise of the true Emperor.\n");
			silmaneText.append("He does not see you as a threat, being only a minor part of\n");
			silmaneText.append("the Imperial Court. In that act of arrogance, he has made\n");
			silmaneText.append("his first mistake. Look to the north wall of this cell. You\n");
			silmaneText.append("will find a ruby key which will unlock the door. Take it and\n");
			silmaneText.append("make your escape. The passages here were once used by Tharn\n");
			silmaneText.append("to hide treasures he had stolen from the Emperor's coffers.\n");
			silmaneText.append("If you wish, you can gather enough to support yourself away\n");
			silmaneText.append("from the Imperial Seat. Be careful, there are many creatures\n");
			silmaneText.append("which inhabit the sewers now, vile rats and goblins. It is\n");
			silmaneText.append("too late for me, for I am already dead. Only my powers as a\n");
			silmaneText.append("Sorceress keep me between this life and the next. That power\n");
			silmaneText.append("however is waning. Do not succumb to greed or you may find\n");
			silmaneText.append("these tunnels to be your final resting place as well. I can\n");
			silmaneText.append("still work my magic to a certain extent. If you travel west\n");
			silmaneText.append("from this cell, then south, you will find a Shift Gate. It\n");
			silmaneText.append("will transport you far enough from the center of the Empire\n");
			silmaneText.append("that you should be safe. If you survive these sewers you\n");
			silmaneText.append("will see me again. Remember, " + playerName + ", Tharn has taken on the\n");
			silmaneText.append("guise of the Emperor. No one will gainsay his word for\n");
			silmaneText.append("yours. I will come to you again in your dreams, so it is\n");
			silmaneText.append("imperative that you rest from time to time. In that way I\n");
			silmaneText.append("will be able to communicate with you and lend my aid. You\n");
			silmaneText.append("are entering a dangerous arena, my friend, one in which the\n");
			silmaneText.append("players are beings beyond your mortal comprehension. I do\n");
			silmaneText.append("not envy your role. There is however a power within you as\n");
			silmaneText.append("yet untapped. Look for me when you have gained experience in\n");
			silmaneText.append("the world. You are my last and best hope...\n");

			std::unique_ptr<Panel> cinematicPanel(new TextCinematicPanel(
				gameState,
				TextureSequenceName::Silmane,
				silmaneText,
				TextCinematicPanel::DEFAULT_MOVIE_SECONDS_PER_IMAGE,
				gameFunction));

			gameState->setPanel(std::move(cinematicPanel));
			gameState->setMusic(MusicName::Vision);
		};

		return std::unique_ptr<Button>(new Button(center, width, height, cinematicFunction));
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

void ChooseAttributesPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();

	// Set palette.
	auto &textureManager = this->getGameState()->getTextureManager();
	textureManager.setPalette(PaletteName::CharSheet);

	// Draw attributes texture.
	auto *attributesBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::CharacterStats));
	renderer.drawToOriginal(attributesBackground);

	// Get the filenames for the portraits.
	auto portraitStrings = PortraitFile::getGroup(*this->gender.get(),
		*this->raceName.get(), this->charClass->canCastMagic());

	// Draw the current portrait.
	auto *portrait = textureManager.getTexture(
		portraitStrings.at(this->portraitIndex));
	int portraitWidth, portraitHeight;
	SDL_QueryTexture(portrait, nullptr, nullptr, &portraitWidth, &portraitHeight);
	renderer.drawToOriginal(portrait, ORIGINAL_WIDTH - portraitWidth, 0);

	// Draw text boxes: player name, race, class.
	renderer.drawToOriginal(this->nameTextBox->getSurface(),
		this->nameTextBox->getX(), this->nameTextBox->getY());
	renderer.drawToOriginal(this->raceTextBox->getSurface(),
		this->raceTextBox->getX(), this->raceTextBox->getY());
	renderer.drawToOriginal(this->classTextBox->getSurface(),
		this->classTextBox->getX(), this->classTextBox->getY());

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
