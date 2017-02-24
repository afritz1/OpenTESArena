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
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Random.h"
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
#include "../Rendering/Rect3D.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"
#include "../World/VoxelBuilder.h"
#include "../World/VoxelData.h"
#include "../World/VoxelGrid.h"

ChooseAttributesPanel::ChooseAttributesPanel(Game *game,
	const CharacterClass &charClass, const std::string &name, GenderName gender,
	CharacterRaceName raceName)
	: Panel(game), headOffsets()
{
	this->nameTextBox = [game, name]()
	{
		Int2 origin(10, 8);
		Color color(199, 199, 199);
		std::string text = name;
		auto &font = game->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Left;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.x,
			origin.y,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->raceTextBox = [game, raceName]()
	{
		Int2 origin(10, 17);
		Color color(199, 199, 199);
		std::string text = CharacterRace(raceName).toString();
		auto &font = game->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Left;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.x,
			origin.y,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->classTextBox = [game, charClass]()
	{
		Int2 origin(10, 26);
		Color color(199, 199, 199);
		std::string text = charClass.getDisplayName();
		auto &font = game->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Left;
		return std::unique_ptr<TextBox>(new TextBox(
			origin.x,
			origin.y,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->backToRaceButton = [charClass, name, gender]()
	{
		auto function = [charClass, name, gender](Game *game)
		{
			std::unique_ptr<Panel> racePanel(new ChooseRacePanel(
				game, charClass, name, gender));
			game->setPanel(std::move(racePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->doneButton = [this, charClass, name, gender, raceName]()
	{
		Int2 center(25, Renderer::ORIGINAL_HEIGHT - 15);
		int width = 21;
		int height = 12;

		auto gameDataFunction = [this, charClass, name, gender, raceName](Game *game)
		{
			// Make placeholders here for the game data. They'll be more informed
			// in the future once the player has a place in the world and the options
			// menu has settings for the renderer.
			std::unique_ptr<EntityManager> entityManager(new EntityManager());

			Double3 position = Double3(1.50, 1.70, 2.50); // Arbitrary player height.
			Double3 direction = Double3(1.0, 0.0, 1.0).normalized();
			Double3 velocity = Double3(0.0, 0.0, 0.0);

			// Some arbitrary max speeds.
			double maxWalkSpeed = 2.0;
			double maxRunSpeed = 8.0;

			std::unique_ptr<Player> player(new Player(name, gender, raceName,
				charClass, this->portraitIndex, position, direction, velocity,
				maxWalkSpeed, maxRunSpeed, *entityManager.get()));

			// Initialize 3D renderer.
			auto &renderer = game->getRenderer();
			renderer.initializeWorldRendering(
				game->getOptions().getResolutionScale(), false);

			// Send some textures and test geometry to renderer memory. Eventually
			// this will be moved out to another data class, maybe stored in the game
			// data object.
			/*auto &textureManager = game->getTextureManager();
			textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

			std::vector<const SDL_Surface*> surfaces =
			{
				// Texture indices:
				// 0: city wall
				textureManager.getSurface("CITYWALL.IMG"),

				// 1-3: grounds
				textureManager.getSurfaces("NORM1.SET").at(0),
				textureManager.getSurfaces("NORM1.SET").at(1),
				textureManager.getSurfaces("NORM1.SET").at(2),

				// 4-5: gates
				textureManager.getSurface("DLGT.IMG"),
				textureManager.getSurface("DRGT.IMG"),

				// 6-9: tavern + door
				textureManager.getSurfaces("MTAVERN.SET").at(0),
				textureManager.getSurfaces("MTAVERN.SET").at(1),
				textureManager.getSurfaces("MTAVERN.SET").at(2),
				textureManager.getSurface("DTAV.IMG"),

				// 10-15: temple + door
				textureManager.getSurfaces("MTEMPLE.SET").at(0),
				textureManager.getSurfaces("MTEMPLE.SET").at(1),
				textureManager.getSurfaces("MTEMPLE.SET").at(2),
				textureManager.getSurfaces("MTEMPLE.SET").at(3),
				textureManager.getSurfaces("MTEMPLE.SET").at(4),
				textureManager.getSurface("DTEP.IMG"),

				// 16-21: Mages' Guild + door
				textureManager.getSurfaces("MMUGUILD.SET").at(0),
				textureManager.getSurfaces("MMUGUILD.SET").at(1),
				textureManager.getSurfaces("MMUGUILD.SET").at(2),
				textureManager.getSurfaces("MMUGUILD.SET").at(3),
				textureManager.getSurfaces("MMUGUILD.SET").at(4),
				textureManager.getSurface("DMU.IMG"),

				// 22-25: Equipment store + door
				textureManager.getSurfaces("MEQUIP.SET").at(0),
				textureManager.getSurfaces("MEQUIP.SET").at(1),
				textureManager.getSurfaces("MEQUIP.SET").at(2),
				textureManager.getSurface("DEQ.IMG"),

				// 26-29: Noble house + door
				textureManager.getSurfaces("MNOBLE.SET").at(0),
				textureManager.getSurfaces("MNOBLE.SET").at(1),
				textureManager.getSurfaces("MNOBLE.SET").at(2),
				textureManager.getSurface("DNB1.IMG")
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

			// -- test -- Add sprite textures, used later in GameWorldPanel::tick().
			const SDL_Surface *surface = textureManager.getSurfaces("BARTEND.DFA").at(0);
			renderer.addTexture(static_cast<uint32_t*>(surface->pixels), 
				surface->w, surface->h);
			surface = textureManager.getSurfaces("BARTEND.DFA").at(1);
			renderer.addTexture(static_cast<uint32_t*>(surface->pixels), 
				surface->w, surface->h);

			// Add lights. They are expensive to update every frame because they cover
			// so many voxels, so the tick method should only update moving ones in practice.
			// The intensity (reach) of a light also determines its memory usage.
			for (int k = 1; k < worldDepth - 1; k += 8)
			{
				for (int i = 1; i < worldWidth - 1; i += 8)
				{
					renderer.updateLight(
						(i - 1) + ((k - 1) * (worldWidth - 1)),
						Double3(i + 0.90, 1.50, k + 0.20),
						Double3(1.0, 0.80, 0.40),
						6.0);
				}
			}*/

			// Add some distinctive test textures.
			auto &textureManager = this->getGame()->getTextureManager();
			textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));
			std::vector<const SDL_Surface*> surfaces = {
				textureManager.getSurfaces("CASA.SET").at(3),
				textureManager.getSurfaces("CASF.SET").at(3),
				textureManager.getSurfaces("CASH.SET").at(3)
			};

			for (const auto *surface : surfaces)
			{
				renderer.addTexture(static_cast<uint32_t*>(surface->pixels), 
					surface->w, surface->h);
			}
			// -- end test --

			// Voxel grid with some arbitrary dimensions.
			const int gridWidth = 32;
			const int gridHeight = 5;
			const int gridDepth = 32;
			const double voxelHeight = 1.0;
			std::unique_ptr<VoxelGrid> voxelGrid(new VoxelGrid(
				gridWidth, gridHeight, gridDepth, voxelHeight));

			// Add some voxel data for the voxel IDs to refer to.
			voxelGrid->addVoxelData(VoxelData(0, 0));
			voxelGrid->addVoxelData(VoxelData(1, 1));
			voxelGrid->addVoxelData(VoxelData(2, 2));

			const double gameTime = 0.0; // In seconds. Also affects sun position.
			const double viewDistance = 15.0;
			std::unique_ptr<GameData> gameData(new GameData(
				std::move(player), std::move(entityManager), 
				std::move(voxelGrid), gameTime, viewDistance));

			// -- test --

			// Set random voxels. These voxel IDs will refer to voxel data.
			char *voxels = gameData->getVoxelGrid().getVoxels();
			Random random(0);

			for (int k = 0; k < gridDepth; ++k)
			{
				for (int i = 0; i < gridWidth; ++i)
				{
					// Ground.
					const int j = 0;
					const int index = i + (j * gridWidth) + (k * gridWidth * gridHeight);
					voxels[index] = 1 + random.next(3);
				}
			}

			for (int n = 0; n < 200; ++n)
			{
				const int x = random.next(gridWidth);
				const int y = 1 + random.next(gridHeight - 1);
				const int z = random.next(gridDepth);

				const int index = x + (y * gridWidth) + (z * gridWidth * gridHeight);
				voxels[index] = 1 + random.next(3);
			}
			// -- end test --

			// Set the game data before constructing the game world panel.
			game->setGameData(std::move(gameData));
		};

		auto gameFunction = [](Game *game)
		{
			std::unique_ptr<Panel> gameWorldPanel(new GameWorldPanel(game));
			game->setPanel(std::move(gameWorldPanel));
			game->setMusic(MusicName::Overcast);
		};

		auto cinematicFunction = [gameDataFunction, gameFunction](Game *game)
		{
			gameDataFunction(game);

			// The original game wraps text onto the next screen if the player's name is
			// too long. For example, it causes "listen to me" to go down one line and 
			// "Imperial Battle" to go onto the next screen, which then pushes the text
			// for every subsequent screen forward by a little bit.

			// Read Ria Silmane's text from TEMPLATE.DAT.
			std::string silmaneText = game->getTextAssets().getTemplateDatText("#1400");
			silmaneText.append("\n");

			// Replace all instances of %pcf with the player's first name.
			const std::string playerName =
				game->getGameData().getPlayer().getFirstName();
			silmaneText = String::replace(silmaneText, "%pcf", playerName);

			// Some more formatting should be done in the future so the text wraps nicer.
			// That is, replace all new lines with spaces and redistribute new lines given
			// some max line length value.

			std::unique_ptr<Panel> cinematicPanel(new TextCinematicPanel(
				game,
				TextureFile::fromName(TextureSequenceName::Silmane),
				silmaneText,
				0.171,
				gameFunction));

			game->setPanel(std::move(cinematicPanel));
			game->setMusic(MusicName::Vision);
		};

		return std::unique_ptr<Button>(new Button(center, width, height, cinematicFunction));
	}();

	this->incrementPortraitButton = [this]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH - 72, 25);
		int width = 60;
		int height = 42;
		auto function = [this](Game *game)
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
		auto function = [this](Game *game)
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

	this->gender = std::unique_ptr<GenderName>(new GenderName(gender));
	this->charClass = std::unique_ptr<CharacterClass>(new CharacterClass(charClass));
	this->raceName = std::unique_ptr<CharacterRaceName>(new CharacterRaceName(raceName));
	this->name = name;
	this->portraitIndex = 0;
}

ChooseAttributesPanel::~ChooseAttributesPanel()
{

}

void ChooseAttributesPanel::handleEvent(const SDL_Event &e)
{
	bool escapePressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_ESCAPE);

	if (escapePressed)
	{
		this->backToRaceButton->click(this->getGame());
	}

	bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_LEFT);
	bool rightClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_RIGHT);
		
	const Int2 mousePosition = this->getMousePosition();
	const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
		.nativePointToOriginal(mousePosition);

	if (leftClick)
	{
		if (this->doneButton->contains(mouseOriginalPoint))
		{
			this->doneButton->click(this->getGame());
		}
		else if (this->incrementPortraitButton->contains(mouseOriginalPoint))
		{
			this->incrementPortraitButton->click(this->getGame());
		}
	}

	if (rightClick)
	{
		if (this->decrementPortraitButton->contains(mouseOriginalPoint))
		{
			this->decrementPortraitButton->click(this->getGame());
		}
	}	
}

void ChooseAttributesPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGame()->getTextureManager();
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
	const auto &head = textureManager.getTextures(headsFilename,
		PaletteFile::fromName(PaletteName::CharSheet)).at(this->portraitIndex);
	const auto &body = textureManager.getTexture(bodyFilename);
	const auto &shirt = textureManager.getTexture(shirtFilename);
	const auto &pants = textureManager.getTexture(pantsFilename);
	renderer.drawToOriginal(body.get(), Renderer::ORIGINAL_WIDTH - body.getWidth(), 0);
	renderer.drawToOriginal(pants.get(), pantsOffset.x, pantsOffset.y);
	renderer.drawToOriginal(head.get(), headOffset.x, headOffset.y);
	renderer.drawToOriginal(shirt.get(), shirtOffset.x, shirtOffset.y);

	// Draw attributes texture.
	const auto &attributesBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::CharacterStats));
	renderer.drawToOriginal(attributesBackground.get());

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
	const auto &cursor = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor));
	const auto mousePosition = this->getMousePosition();
	renderer.drawToNative(cursor.get(),
		mousePosition.x, mousePosition.y,
		static_cast<int>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int>(cursor.getHeight() * this->getCursorScale()));
}
