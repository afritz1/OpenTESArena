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
	this->instructionsTextBox = [gameState]()
	{
		auto center = Int2((ORIGINAL_WIDTH / 4) + 5, 100);
		auto color = Color::White;
		std::string text = std::string("Use thine A and D\nkeys to change\n") + 
			"thy portrait.\n\nClick \"Done\" when\nthou art finished.";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->nameTextBox = [gameState, name]()
	{
		auto origin = Int2(10, 8);
		auto color = Color(199, 199, 199);
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
		auto origin = Int2(10, 17);
		auto color = Color(199, 199, 199);
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
		auto origin = Int2(10, 26);
		auto color = Color(199, 199, 199);
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

	this->backToRaceButton = [gameState, charClass, name, gender]()
	{
		auto function = [gameState, charClass, name, gender]()
		{
			auto racePanel = std::unique_ptr<Panel>(new ChooseRacePanel(
				gameState, charClass, name, gender));
			gameState->setPanel(std::move(racePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->doneButton = [this, gameState, charClass, name, gender, raceName]()
	{
		auto center = Int2(25, ORIGINAL_HEIGHT - 15);
		int width = 21;
		int height = 12;
		auto function = [this, gameState, charClass, name, gender, raceName]()
		{
			// Make placeholders here for the game data. They'll be more informed
			// in the future once the player has a place in the world and the options
			// menu has settings for the CLProgram.
			auto entityManager = std::unique_ptr<EntityManager>(new EntityManager());

			Float3d position = Float3d(0.0, 0.0, 0.0);
			Float3d direction = Float3d(0.0, 0.0, 1.0).normalized();
			Float3d velocity = Float3d(0.0, 0.0, 0.0);
			
			// Some arbitrary max speeds.
			double maxWalkSpeed = 2.0;
			double maxRunSpeed = 8.0;

			auto player = std::unique_ptr<Player>(new Player(name, gender, raceName,
				charClass, this->portraitIndex, position, direction, velocity,
				maxWalkSpeed, maxRunSpeed, *entityManager.get()));
			auto clProgram = std::unique_ptr<CLProgram>(new CLProgram(
				gameState->getScreenDimensions().getX(),
				gameState->getScreenDimensions().getY(),
				gameState->getRenderer().getRenderer()));
			double gameTime = 0.0; // In seconds. Also affects sun position.
			auto gameData = std::unique_ptr<GameData>(new GameData(
				std::move(player), std::move(entityManager), std::move(clProgram),
				gameTime));

			// Set the game data before constructing the game world panel.
			gameState->setGameData(std::move(gameData));

			auto gameWorldPanel = std::unique_ptr<Panel>(new GameWorldPanel(gameState));

			// Set the game world's music to be some placeholder for now.
			gameState->setMusic(MusicName::SunnyDay);
			gameState->setPanel(std::move(gameWorldPanel));
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
			this->backToRaceButton->click();
		}

		bool incrementIndex = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_d);
		bool decrementIndex = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_a);

		// Update the portrait index for which portrait to show. Only ten portraits
		// are allowed for now.
		if (incrementIndex)
		{
			this->portraitIndex = (this->portraitIndex == 9) ? 0 : (this->portraitIndex + 1);
		}
		if (decrementIndex)
		{
			this->portraitIndex = (this->portraitIndex == 0) ? 9 : (this->portraitIndex - 1);
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);

		if (leftClick && this->doneButton->containsPoint(mouseOriginalPoint))
		{
			this->doneButton->click();
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
	const auto *attributesBackground = this->getGameState()->getTextureManager()
		.getTexture(TextureFile::fromName(TextureName::CharacterStats));
	this->drawScaledToNative(attributesBackground, renderer);

	// Get the filenames for the portraits.
	auto portraitStrings = PortraitFile::getGroup(*this->gender.get(),
		*this->raceName.get(), this->charClass->canCastMagic());

	// Draw the current portrait.
	const auto *portrait = this->getGameState()->getTextureManager()
		.getTexture(portraitStrings.at(this->portraitIndex));
	int portraitWidth, portraitHeight;
	SDL_QueryTexture(const_cast<SDL_Texture*>(portrait), nullptr, nullptr, 
		&portraitWidth, &portraitHeight);

	this->drawScaledToNative(portrait,
		ORIGINAL_WIDTH - portraitWidth,
		0,
		portraitWidth,
		portraitHeight,
		renderer);

	// Draw text: instructions, name, race, class.
	this->drawScaledToNative(*this->instructionsTextBox.get(), renderer);
	this->drawScaledToNative(*this->nameTextBox.get(), renderer);
	this->drawScaledToNative(*this->raceTextBox.get(), renderer);
	this->drawScaledToNative(*this->classTextBox.get(), renderer);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, renderer);
}
