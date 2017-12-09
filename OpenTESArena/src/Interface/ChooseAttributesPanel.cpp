#include <cassert>

#include "SDL.h"

#include "ChooseAttributesPanel.h"
#include "ChooseRacePanel.h"
#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "TextCinematicPanel.h"
#include "TextSubPanel.h"
#include "../Assets/CIFFile.h"
#include "../Assets/ExeStrings.h"
#include "../Assets/MiscAssets.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Game/PlayerInterface.h"
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

ChooseAttributesPanel::ChooseAttributesPanel(Game &game,
	const CharacterClass &charClass, const std::string &name, 
	GenderName gender, int raceID)
	: Panel(game), charClass(charClass), gender(gender), name(name)
{
	this->nameTextBox = [&game, &name]()
	{
		const int x = 10;
		const int y = 8;

		const RichTextString richText(
			name,
			FontName::Arena,
			Color(199, 199, 199),
			TextAlignment::Left,
			game.getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			x, y, richText, game.getRenderer()));
	}();

	this->raceTextBox = [&game, raceID]()
	{
		const int x = 10;
		const int y = 17;

		const std::string &text = game.getMiscAssets().getAExeStrings().getList(
			ExeStringKey::RaceNamesSingular).at(raceID);

		const RichTextString richText(
			text,
			FontName::Arena,
			Color(199, 199, 199),
			TextAlignment::Left,
			game.getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			x, y, richText, game.getRenderer()));
	}();

	this->classTextBox = [&game, &charClass]()
	{
		const int x = 10;
		const int y = 26;

		const RichTextString richText(
			charClass.getName(),
			FontName::Arena,
			Color(199, 199, 199),
			TextAlignment::Left,
			game.getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			x, y, richText, game.getRenderer()));
	}();

	this->backToRaceButton = [&charClass, &name, gender]()
	{
		auto function = [charClass, name, gender](Game &game)
		{
			game.setPanel<ChooseRacePanel>(game, charClass, name, gender);
		};
		return Button<Game&>(function);
	}();

	this->doneButton = [this, &charClass, &name, gender, raceID]()
	{
		const Int2 center(25, Renderer::ORIGINAL_HEIGHT - 15);
		const int width = 21;
		const int height = 12;

		auto gameDataFunction = [this, charClass, name, gender, raceID](Game &game)
		{
			// Initialize 3D renderer.
			auto &renderer = game.getRenderer();
			const bool fullGameWindow = game.getOptions().getModernInterface();
			renderer.initializeWorldRendering(
				game.getOptions().getResolutionScale(), fullGameWindow);

			// Generate the test world data.
			std::unique_ptr<GameData> gameData = GameData::createDefault(
				name, gender, raceID, charClass, this->portraitID,
				game.getTextureManager(), renderer);

			// Set the game data before constructing the game world panel.
			game.setGameData(std::move(gameData));
		};

		auto gameFunction = [](Game &game)
		{
			game.setPanel<GameWorldPanel>(game);
			game.setMusic(MusicName::SunnyDay);
		};

		auto cinematicFunction = [gameDataFunction, gameFunction](Game &game)
		{
			gameDataFunction(game);

			// The original game wraps text onto the next screen if the player's name is
			// too long. For example, it causes "listen to me" to go down one line and 
			// "Imperial Battle" to go onto the next screen, which then pushes the text
			// for every subsequent screen forward by a little bit.

			// Read Ria Silmane's text from TEMPLATE.DAT.
			std::string silmaneText = game.getMiscAssets().getTemplateDatText("#1400");
			silmaneText.append("\n");

			// Replace all instances of %pcf with the player's first name.
			const std::string playerName =
				game.getGameData().getPlayer().getFirstName();
			silmaneText = String::replace(silmaneText, "%pcf", playerName);

			// Some more formatting should be done in the future so the text wraps nicer.
			// That is, replace all new lines with spaces and redistribute new lines given
			// some max line length value.

			game.setPanel<TextCinematicPanel>(
				game,
				TextureFile::fromName(TextureSequenceName::Silmane),
				silmaneText,
				0.171,
				gameFunction);
			game.setMusic(MusicName::Vision);
		};

		return Button<Game&>(center, width, height, cinematicFunction);
	}();

	this->portraitButton = []()
	{
		const Int2 center(Renderer::ORIGINAL_WIDTH - 72, 25);
		const int width = 60;
		const int height = 42;
		auto function = [](ChooseAttributesPanel &panel, bool increment)
		{
			const int minID = 0;
			const int maxID = 9;

			if (increment)
			{
				panel.portraitID = (panel.portraitID == maxID) ?
					minID : (panel.portraitID + 1);
			}
			else
			{
				panel.portraitID = (panel.portraitID == minID) ?
					maxID : (panel.portraitID - 1);
			}
		};
		return Button<ChooseAttributesPanel&, bool>(center, width, height, function);
	}();

	// Get pixel offsets for each head.
	const std::string &headsFilename = PortraitFile::getHeads(gender, raceID, false);
	CIFFile cifFile(headsFilename, Palette());

	for (int i = 0; i < cifFile.getImageCount(); i++)
	{
		this->headOffsets.push_back(Int2(cifFile.getXOffset(i), cifFile.getYOffset(i)));
	}

	this->raceID = raceID;
	this->portraitID = 0;

	// Push the initial text pop-up onto the sub-panel stack.
	std::unique_ptr<Panel> textSubPanel = [&game]()
	{
		const Int2 center(
			(Renderer::ORIGINAL_WIDTH / 2) - 1,
			(Renderer::ORIGINAL_HEIGHT / 2) - 2);
		const Color color(199, 199, 199);

		const std::string text = [&game]()
		{
			std::string segment = game.getMiscAssets().getAExeStrings().get(
				ExeStringKey::DistributeClassPoints);
			segment = String::replace(segment, '\r', '\n');

			return segment;
		}();

		const int lineSpacing = 1;
		
		const RichTextString richText(
			text,
			FontName::Arena,
			color,
			TextAlignment::Center,
			lineSpacing,
			game.getFontManager());

		Texture texture(Texture::generate(
			Texture::PatternType::Dark, 183, 42, game.getTextureManager(),
			game.getRenderer()));

		const Int2 textureCenter(
			(Renderer::ORIGINAL_WIDTH / 2) - 1,
			(Renderer::ORIGINAL_HEIGHT / 2) - 1);

		// The sub-panel does nothing after it's removed.
		auto function = [](Game &game) {};

		return std::unique_ptr<Panel>(new TextSubPanel(
			game, center, richText, function, std::move(texture), textureCenter));
	}();

	game.pushSubPanel(std::move(textSubPanel));
}

ChooseAttributesPanel::~ChooseAttributesPanel()
{

}

std::pair<SDL_Texture*, CursorAlignment> ChooseAttributesPanel::getCurrentCursor() const
{
	auto &textureManager = this->getGame().getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default));
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
}

void ChooseAttributesPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);

	if (escapePressed)
	{
		this->backToRaceButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);
		
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 mouseOriginalPoint = this->getGame().getRenderer()
		.nativePointToOriginal(mousePosition);

	if (leftClick)
	{
		if (this->doneButton.contains(mouseOriginalPoint))
		{
			this->doneButton.click(this->getGame());
		}
		else if (this->portraitButton.contains(mouseOriginalPoint))
		{
			// Pass 'true' to increment the portrait ID.
			this->portraitButton.click(*this, true);
		}
	}

	if (rightClick)
	{
		if (this->portraitButton.contains(mouseOriginalPoint))
		{
			// Pass 'false' to decrement the portrait ID.
			this->portraitButton.click(*this, false);
		}
	}	
}

void ChooseAttributesPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Set palette.
	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::CharSheet));

	// Get the filenames for the portrait and clothes.
	const std::string &headsFilename = PortraitFile::getHeads(
		this->gender, this->raceID, false);
	const std::string &bodyFilename = PortraitFile::getBody(
		this->gender, this->raceID);
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
	renderer.drawOriginal(body.get(), Renderer::ORIGINAL_WIDTH - body.getWidth(), 0);
	renderer.drawOriginal(pants.get(), pantsOffset.x, pantsOffset.y);
	renderer.drawOriginal(head.get(), headOffset.x, headOffset.y);
	renderer.drawOriginal(shirt.get(), shirtOffset.x, shirtOffset.y);

	// Draw attributes texture.
	const auto &attributesBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::CharacterStats));
	renderer.drawOriginal(attributesBackground.get());

	// Draw text boxes: player name, race, class.
	renderer.drawOriginal(this->nameTextBox->getTexture(),
		this->nameTextBox->getX(), this->nameTextBox->getY());
	renderer.drawOriginal(this->raceTextBox->getTexture(),
		this->raceTextBox->getX(), this->raceTextBox->getY());
	renderer.drawOriginal(this->classTextBox->getTexture(),
		this->classTextBox->getX(), this->classTextBox->getY());
}
