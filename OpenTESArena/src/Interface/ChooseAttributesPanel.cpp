#include <cassert>

#include "SDL.h"

#include "ChooseAttributesPanel.h"

#include "Button.h"
#include "ChooseRacePanel.h"
#include "GameWorldPanel.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "TextCinematicPanel.h"
#include "../Assets/CIFFile.h"
#include "../Assets/TextAssets.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/CharacterRace.h"
#include "../Entities/EntityManager.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/GameState.h"
#include "../Game/Options.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/MusicName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/PortraitFile.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Media/TextureSequenceName.h"
#include "../Rendering/CLProgram.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

ChooseAttributesPanel::ChooseAttributesPanel(GameState *gameState,
	const CharacterClass &charClass, const std::string &name, CharacterGenderName gender,
	CharacterRaceName raceName)
	: Panel(gameState), headOffsets()
{
	this->nameTextBox = [gameState, name]()
	{
		Int2 origin(10, 8);
		Color color(199, 199, 199);
		std::string text = name;
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

	this->raceTextBox = [gameState, raceName]()
	{
		Int2 origin(10, 17);
		Color color(199, 199, 199);
		std::string text = CharacterRace(raceName).toString();
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

	this->classTextBox = [gameState, charClass]()
	{
		Int2 origin(10, 26);
		Color color(199, 199, 199);
		std::string text = charClass.getDisplayName();
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
		Int2 center(25, Renderer::ORIGINAL_HEIGHT - 15);
		int width = 21;
		int height = 12;

		auto gameDataFunction = [this, charClass, name, gender, raceName](GameState *gameState)
		{
			// Make placeholders here for the game data. They'll be more informed
			// in the future once the player has a place in the world and the options
			// menu has settings for the CLProgram.
			std::unique_ptr<EntityManager> entityManager(new EntityManager());

			Float3d position = Float3d(1.50, 1.70, 2.50); // Arbitrary player height.
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
				gameState->getOptions().getResolutionScale()));

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

		auto cinematicFunction = [gameDataFunction, gameFunction](GameState *gameState)
		{
			gameDataFunction(gameState);

			// The original game wraps text onto the next screen if the player's name is
			// too long. For example, it causes "listen to me" to go down one line and 
			// "Imperial Battle" to go onto the next screen, which then pushes the text
			// for every subsequent screen forward by a little bit.

			// Read Ria Silmane's text from TEMPLATE.DAT.
			std::string silmaneText = gameState->getTextAssets().getTemplateDatText("#1400");
			silmaneText.append("\n");

			// Replace all instances of %pcf with the player's first name.
			const std::string playerName =
				gameState->getGameData()->getPlayer().getFirstName();
			silmaneText = String::replace(silmaneText, "%pcf", playerName);

			// Some more formatting should be done in the future so the text wraps nicer.
			// That is, replace all new lines with spaces and redistribute new lines given
			// some max line length value.

			std::unique_ptr<Panel> cinematicPanel(new TextCinematicPanel(
				gameState,
				TextureFile::fromName(TextureSequenceName::Silmane),
				silmaneText,
				0.171,
				gameFunction));

			gameState->setPanel(std::move(cinematicPanel));
			gameState->setMusic(MusicName::Vision);
		};

		return std::unique_ptr<Button>(new Button(center, width, height, cinematicFunction));
	}();

	this->incrementPortraitButton = [this]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH - 72, 25);
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
		Int2 center(Renderer::ORIGINAL_WIDTH - 72, 25);
		int width = 60;
		int height = 42;
		auto function = [this](GameState *gameState)
		{
			this->portraitIndex = (this->portraitIndex == 0) ? 9 : (this->portraitIndex - 1);
		};
		return std::unique_ptr<Button>(new Button(center, width, height, function));
	}();

	// Get pixel offsets for each head.
	const std::string &headsFilename = PortraitFile::getHeads(gender, raceName, false);
	CIFFile cifFile(headsFilename, Palette());

	for (int i = 0; i < cifFile.getImageCount(); ++i)
	{
		this->headOffsets.push_back(Int2(cifFile.getXOffset(i), cifFile.getYOffset(i)));
	}

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
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGameState()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::CharSheet));

	// Get the filenames for the portrait and clothes.
	const std::string &headsFilename = PortraitFile::getHeads(
		*this->gender.get(), *this->raceName.get(), false);
	const std::string &bodyFilename = PortraitFile::getBody(
		*this->gender.get(), *this->raceName.get());
	const std::string &shirtFilename = PortraitFile::getShirt(
		*this->gender.get(), this->charClass->canCastMagic());
	const std::string &pantsFilename = PortraitFile::getPants(*this->gender.get());

	// Get pixel offsets for each clothes texture.
	const Int2 &shirtOffset = PortraitFile::getShirtOffset(
		*this->gender.get(), this->charClass->canCastMagic());
	const Int2 &pantsOffset = PortraitFile::getPantsOffset(*this->gender.get());

	// Draw the current portrait and clothes.
	const Int2 &headOffset = this->headOffsets.at(portraitIndex);
	auto *head = textureManager.getSurfaces(headsFilename,
		PaletteFile::fromName(PaletteName::CharSheet)).at(this->portraitIndex);
	auto *body = textureManager.getTexture(bodyFilename);
	auto &shirt = textureManager.getSurface(shirtFilename);
	auto &pants = textureManager.getSurface(pantsFilename);
	int portraitWidth, portraitHeight;
	SDL_QueryTexture(body, nullptr, nullptr, &portraitWidth, &portraitHeight);
	renderer.drawToOriginal(body, Renderer::ORIGINAL_WIDTH - portraitWidth, 0);
	renderer.drawToOriginal(pants.getSurface(), pantsOffset.getX(), pantsOffset.getY());
	renderer.drawToOriginal(head, headOffset.getX(), headOffset.getY());
	renderer.drawToOriginal(shirt.getSurface(), shirtOffset.getX(), shirtOffset.getY());

	// Draw attributes texture.
	auto *attributesBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::CharacterStats));
	renderer.drawToOriginal(attributesBackground);

	// Draw text boxes: player name, race, class.
	renderer.drawToOriginal(this->nameTextBox->getTexture(),
		this->nameTextBox->getX(), this->nameTextBox->getY());
	renderer.drawToOriginal(this->raceTextBox->getTexture(),
		this->raceTextBox->getX(), this->raceTextBox->getY());
	renderer.drawToOriginal(this->classTextBox->getTexture(),
		this->classTextBox->getX(), this->classTextBox->getY());

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();

	// Draw cursor.
	const auto &cursor = textureManager.getSurface(
		TextureFile::fromName(TextureName::SwordCursor));
	const auto mousePosition = this->getMousePosition();
	renderer.drawToNative(cursor.getSurface(),
		mousePosition.getX(), mousePosition.getY(),
		static_cast<int>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int>(cursor.getHeight() * this->getCursorScale()));
}
