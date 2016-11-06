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
#include "../Math/Random.h"
#include "../Math/Rect3D.h"
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
#include "../World/VoxelBuilder.h"

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

			// Initialize 3D rendering program.
			auto &renderer = gameState->getRenderer();
			renderer.initializeWorldRendering(
				worldWidth, worldHeight, worldDepth, 
				gameState->getOptions().getResolutionScale(), false);

			// Send some textures and test geometry to CL device memory. Eventually
			// this will be moved out to another data class, maybe stored in the game
			// data object.
			auto &textureManager = gameState->getTextureManager();
			textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

			std::vector<const SDL_Surface*> surfaces =
			{
				// Texture indices:
				// 0: city wall
				textureManager.getSurface("CITYWALL.IMG").getSurface(),

				// 1-3: grounds
				textureManager.getSurfaces("NORM1.SET").at(0),
				textureManager.getSurfaces("NORM1.SET").at(1),
				textureManager.getSurfaces("NORM1.SET").at(2),

				// 4-5: gates
				textureManager.getSurface("DLGT.IMG").getSurface(),
				textureManager.getSurface("DRGT.IMG").getSurface(),

				// 6-9: tavern + door
				textureManager.getSurfaces("MTAVERN.SET").at(0),
				textureManager.getSurfaces("MTAVERN.SET").at(1),
				textureManager.getSurfaces("MTAVERN.SET").at(2),
				textureManager.getSurface("DTAV.IMG").getSurface(),

				// 10-15: temple + door
				textureManager.getSurfaces("MTEMPLE.SET").at(0),
				textureManager.getSurfaces("MTEMPLE.SET").at(1),
				textureManager.getSurfaces("MTEMPLE.SET").at(2),
				textureManager.getSurfaces("MTEMPLE.SET").at(3),
				textureManager.getSurfaces("MTEMPLE.SET").at(4),
				textureManager.getSurface("DTEP.IMG").getSurface(),

				// 16-21: Mages' Guild + door
				textureManager.getSurfaces("MMUGUILD.SET").at(0),
				textureManager.getSurfaces("MMUGUILD.SET").at(1),
				textureManager.getSurfaces("MMUGUILD.SET").at(2),
				textureManager.getSurfaces("MMUGUILD.SET").at(3),
				textureManager.getSurfaces("MMUGUILD.SET").at(4),
				textureManager.getSurface("DMU.IMG").getSurface(),

				// 22-25: Equipment store + door
				textureManager.getSurfaces("MEQUIP.SET").at(0),
				textureManager.getSurfaces("MEQUIP.SET").at(1),
				textureManager.getSurfaces("MEQUIP.SET").at(2),
				textureManager.getSurface("DEQ.IMG").getSurface(),

				// 26-29: Noble house + door
				textureManager.getSurfaces("MNOBLE.SET").at(0),
				textureManager.getSurfaces("MNOBLE.SET").at(1),
				textureManager.getSurfaces("MNOBLE.SET").at(2),
				textureManager.getSurface("DNB1.IMG").getSurface()
			};

			std::vector<int> textureIndices;

			for (int i = 0; i < static_cast<int>(surfaces.size()); ++i)
			{
				const SDL_Surface *surface = surfaces.at(i);
				int textureIndex = renderer.addTexture(
					static_cast<uint32_t*>(surface->pixels), surface->w, surface->h);

				textureIndices.push_back(textureIndex);
			}

			// Arbitrary random seed for texture indices.
			Random random(2);
			
			// Ground.
			for (int k = 0; k < worldDepth; ++k)
			{
				for (int i = 0; i < worldWidth; ++i)
				{
					Rect3D rect = VoxelBuilder::makeCeiling(i, 0, k);
					int textureIndex = textureIndices.at(1 + random.next(3));
					renderer.updateVoxel(i, 0, k,
						std::vector<Rect3D>{ rect }, textureIndex);
				}
			}

			// Near X and far X walls.
			for (int j = 1; j < worldHeight; ++j)
			{
				int textureIndex = textureIndices.at(0);
				for (int k = 0; k < worldDepth; ++k)
				{
					std::vector<Rect3D> block = VoxelBuilder::makeBlock(0, j, k);
					renderer.updateVoxel(0, j, k, block, textureIndex);

					block = VoxelBuilder::makeBlock(worldWidth - 1, j, k);
					renderer.updateVoxel(worldWidth - 1, j, k, block, textureIndex);
				}
			}

			// Near Z and far Z walls (ignoring existing corners).
			for (int j = 1; j < worldHeight; ++j)
			{
				int textureIndex = textureIndices.at(0);
				for (int i = 1; i < (worldWidth - 1); ++i)
				{
					std::vector<Rect3D> block = VoxelBuilder::makeBlock(i, j, 0);
					renderer.updateVoxel(i, j, 0, block, textureIndex);

					block = VoxelBuilder::makeBlock(i, j, worldDepth - 1);
					renderer.updateVoxel(i, j, worldDepth - 1, block, textureIndex);
				}
			}

			// Lambda for adding simple voxel buildings.
			auto makeBuilding = [&renderer, &random, &textureIndices](int cellX,
				int cellZ, int width, int height, int depth, const std::vector<int> &indices)
			{
				const int cellY = 1;

				for (int k = 0; k < depth; ++k)
				{
					for (int j = 0; j < height; ++j)
					{
						for (int i = 0; i < width; ++i)
						{
							const int x = cellX + i;
							const int y = cellY + j;
							const int z = cellZ + k;

							const std::vector<Rect3D> block = VoxelBuilder::makeBlock(x, y, z);
							const int textureIndex = textureIndices.at(indices.at(random.next(
								static_cast<int>(indices.size()))));

							renderer.updateVoxel(x, y, z, block, textureIndex);
						}
					}
				}
			};

			// Add some simple buildings around. This data should come from a "World" or 
			// "CityData" class sometime.
			// Tavern #1
			makeBuilding(3, 5, 5, 2, 6, { 6, 7, 8 });
			makeBuilding(3, 6, 1, 1, 1, { 9 });

			// Tavern #2
			makeBuilding(3, 13, 7, 1, 5, { 6, 7, 8 });
			makeBuilding(6, 13, 1, 1, 1, { 9 });

			// Temple #1
			makeBuilding(11, 4, 6, 2, 5, { 10, 11, 12, 13, 14 });
			makeBuilding(11, 6, 1, 1, 1, { 15 });

			// Mage's Guild #1
			makeBuilding(12, 12, 5, 2, 4, { 16, 17, 18, 19, 20 });
			makeBuilding(15, 12, 1, 1, 1, { 21 });

			// Equipment store #1
			makeBuilding(20, 4, 5, 1, 7, { 22, 23, 24 });
			makeBuilding(20, 8, 1, 1, 1, { 25 });

			// Equipment store #2
			makeBuilding(11, 19, 6, 2, 6, { 22, 23, 24 });
			makeBuilding(13, 19, 1, 1, 1, { 25 });

			// Noble house #1
			makeBuilding(21, 15, 6, 2, 8, { 26, 27, 28 });
			makeBuilding(21, 17, 1, 1, 1, { 29 });

			// Add a city gate with some walls.
			makeBuilding(8, 0, 1, 1, 1, { 4 });
			makeBuilding(9, 0, 1, 1, 1, { 5 });
			makeBuilding(1, 1, 7, worldHeight - 1, 1, { 0 });
			makeBuilding(10, 1, 3, worldHeight - 1, 1, { 0 });

			double gameTime = 0.0; // In seconds. Also affects sun position.
			std::unique_ptr<GameData> gameData(new GameData(
				std::move(player), std::move(entityManager), gameTime, 
				worldWidth, worldHeight, worldDepth));

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
