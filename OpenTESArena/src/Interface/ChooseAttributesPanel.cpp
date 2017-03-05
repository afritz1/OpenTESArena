#include <cassert>

#include "SDL.h"

#include "ChooseAttributesPanel.h"

#include "ChooseRacePanel.h"
#include "GameWorldPanel.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "TextCinematicPanel.h"
#include "../Assets/CIFFile.h"
#include "../Assets/TextAssets.h"
#include "../Entities/CharacterRace.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
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
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

ChooseAttributesPanel::ChooseAttributesPanel(Game *game,
	const CharacterClass &charClass, const std::string &name, GenderName gender,
	CharacterRaceName raceName)
	: Panel(game), headOffsets(), gender(gender), charClass(charClass), 
	raceName(raceName), name(name)
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
		return std::unique_ptr<Button<>>(new Button<>(function));
	}();

	this->doneButton = [this, charClass, name, gender, raceName]()
	{
		Int2 center(25, Renderer::ORIGINAL_HEIGHT - 15);
		int width = 21;
		int height = 12;

		auto gameDataFunction = [this, charClass, name, gender, raceName](Game *game)
		{
			// Initialize 3D renderer.
			auto &renderer = game->getRenderer();
			renderer.initializeWorldRendering(
				game->getOptions().getResolutionScale(), false);

			// Add some distinctive wall textures for testing.
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

			// Generate the test world data.
			std::unique_ptr<GameData> gameData = GameData::createDefault(
				name, gender, raceName, charClass, this->portraitID);

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

		return std::unique_ptr<Button<>>(new Button<>(center, width, height, cinematicFunction));
	}();

	this->incrementPortraitButton = [this]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH - 72, 25);
		int width = 60;
		int height = 42;
		auto function = [this](Game *game)
		{
			this->portraitID = (this->portraitID == 9) ? 0 : (this->portraitID + 1);
		};
		return std::unique_ptr<Button<>>(new Button<>(center, width, height, function));
	}();

	this->decrementPortraitButton = [this]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH - 72, 25);
		int width = 60;
		int height = 42;
		auto function = [this](Game *game)
		{
			this->portraitID = (this->portraitID == 0) ? 9 : (this->portraitID - 1);
		};
		return std::unique_ptr<Button<>>(new Button<>(center, width, height, function));
	}();

	// Get pixel offsets for each head.
	const std::string &headsFilename = PortraitFile::getHeads(gender, raceName, false);
	CIFFile cifFile(headsFilename, Palette());

	for (int i = 0; i < cifFile.getImageCount(); ++i)
	{
		this->headOffsets.push_back(Int2(cifFile.getXOffset(i), cifFile.getYOffset(i)));
	}

	this->portraitID = 0;
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
		this->gender, this->raceName, false);
	const std::string &bodyFilename = PortraitFile::getBody(
		this->gender, this->raceName);
	const std::string &shirtFilename = PortraitFile::getShirt(
		this->gender, this->charClass.canCastMagic());
	const std::string &pantsFilename = PortraitFile::getPants(this->gender);

	// Get pixel offsets for each clothes texture.
	const Int2 &shirtOffset = PortraitFile::getShirtOffset(
		this->gender, this->charClass.canCastMagic());
	const Int2 &pantsOffset = PortraitFile::getPantsOffset(this->gender);

	// Draw the current portrait and clothes.
	const Int2 &headOffset = this->headOffsets.at(this->portraitID);
	const auto &head = textureManager.getTextures(headsFilename,
		PaletteFile::fromName(PaletteName::CharSheet)).at(this->portraitID);
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
