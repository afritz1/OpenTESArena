#include <cassert>

#include "SDL.h"

#include "CharacterEquipmentPanel.h"
#include "CharacterPanel.h"
#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Assets/CIFFile.h"
#include "../Assets/ExeStrings.h"
#include "../Assets/MiscAssets.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/PortraitFile.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"

CharacterPanel::CharacterPanel(Game &game)
	: Panel(game)
{
	this->playerNameTextBox = [&game]()
	{
		const int x = 10;
		const int y = 8;

		const RichTextString richText(
			game.getGameData().getPlayer().getDisplayName(),
			FontName::Arena,
			Color(199, 199, 199),
			TextAlignment::Left,
			game.getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			x, y, richText, game.getRenderer()));
	}();

	this->playerRaceTextBox = [&game]()
	{
		const int x = 10;
		const int y = 17;

		const auto &player = game.getGameData().getPlayer();
		const std::string &text = game.getMiscAssets().getAExeStrings().getList(
			ExeStringKey::RaceNamesSingular).at(player.getRaceID());

		const RichTextString richText(
			text,
			FontName::Arena,
			Color(199, 199, 199),
			TextAlignment::Left,
			game.getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			x, y, richText, game.getRenderer()));
	}();

	this->playerClassTextBox = [&game]()
	{
		const int x = 10;
		const int y = 26;

		const RichTextString richText(
			game.getGameData().getPlayer().getCharacterClass().getName(),
			FontName::Arena,
			Color(199, 199, 199),
			TextAlignment::Left,
			game.getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			x, y, richText, game.getRenderer()));
	}();

	this->doneButton = []()
	{
		Int2 center(25, Renderer::ORIGINAL_HEIGHT - 15);
		int width = 21;
		int height = 13;
		auto function = [](Game &game)
		{
			std::unique_ptr<Panel> gamePanel(new GameWorldPanel(game));
			game.setPanel(std::move(gamePanel));
		};
		return Button<Game&>(center, width, height, function);
	}();

	this->nextPageButton = []()
	{
		int x = 108;
		int y = 179;
		int width = 49;
		int height = 13;
		auto function = [](Game &game)
		{
			std::unique_ptr<Panel> equipmentPanel(new CharacterEquipmentPanel(game));
			game.setPanel(std::move(equipmentPanel));
		};
		return Button<Game&>(x, y, width, height, function);
	}();

	// Get pixel offsets for each head.
	const auto &player = this->getGame().getGameData().getPlayer();
	const std::string &headsFilename = PortraitFile::getHeads(
		player.getGenderName(), player.getRaceID(), false);
	CIFFile cifFile(headsFilename, Palette());

	for (int i = 0; i < cifFile.getImageCount(); i++)
	{
		this->headOffsets.push_back(Int2(cifFile.getXOffset(i), cifFile.getYOffset(i)));
	}
}

CharacterPanel::~CharacterPanel()
{

}

std::pair<SDL_Texture*, CursorAlignment> CharacterPanel::getCurrentCursor() const
{
	auto &textureManager = this->getGame().getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default));
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
}

void CharacterPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool tabPressed = inputManager.keyPressed(e, SDLK_TAB);

	if (escapePressed || tabPressed)
	{
		this->doneButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame().getRenderer()
			.nativePointToOriginal(mousePosition);

		if (this->doneButton.contains(mouseOriginalPoint))
		{
			this->doneButton.click(this->getGame());
		}
		else if (this->nextPageButton.contains(mouseOriginalPoint))
		{
			this->nextPageButton.click(this->getGame());
		}
	}
}

void CharacterPanel::render(Renderer &renderer)
{
	assert(this->getGame().gameDataIsActive());

	// Clear full screen.
	renderer.clear();

	// Set palette.
	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::CharSheet));

	// Get a reference to the active player data.
	const auto &player = this->getGame().getGameData().getPlayer();

	// Get the filenames for the portrait and clothes.
	const std::string &headsFilename = PortraitFile::getHeads(
		player.getGenderName(), player.getRaceID(), false);
	const std::string &bodyFilename = PortraitFile::getBody(
		player.getGenderName(), player.getRaceID());
	const std::string &shirtFilename = PortraitFile::getShirt(
		player.getGenderName(), player.getCharacterClass().canCastMagic());
	const std::string &pantsFilename = PortraitFile::getPants(player.getGenderName());

	// Get pixel offsets for each clothes texture.
	const Int2 &shirtOffset = PortraitFile::getShirtOffset(
		player.getGenderName(), player.getCharacterClass().canCastMagic());
	const Int2 &pantsOffset = PortraitFile::getPantsOffset(player.getGenderName());

	// Draw the current portrait and clothes.
	const Int2 &headOffset = this->headOffsets.at(player.getPortraitID());
	const auto &head = textureManager.getTextures(headsFilename,
		PaletteFile::fromName(PaletteName::CharSheet)).at(player.getPortraitID());
	const auto &body = textureManager.getTexture(bodyFilename);
	const auto &shirt = textureManager.getTexture(shirtFilename);
	const auto &pants = textureManager.getTexture(pantsFilename);
	renderer.drawOriginal(body.get(), Renderer::ORIGINAL_WIDTH - body.getWidth(), 0);
	renderer.drawOriginal(pants.get(), pantsOffset.x, pantsOffset.y);
	renderer.drawOriginal(head.get(), headOffset.x, headOffset.y);
	renderer.drawOriginal(shirt.get(), shirtOffset.x, shirtOffset.y);

	// Draw character stats background.
	const auto &statsBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::CharacterStats));
	renderer.drawOriginal(statsBackground.get());

	// Draw "Next Page" texture.
	const auto &nextPageTexture = textureManager.getTexture(
		TextureFile::fromName(TextureName::NextPage));
	renderer.drawOriginal(nextPageTexture.get(), 108, 179);

	// Draw text boxes: player name, race, class.
	renderer.drawOriginal(this->playerNameTextBox->getTexture(),
		this->playerNameTextBox->getX(), this->playerNameTextBox->getY());
	renderer.drawOriginal(this->playerRaceTextBox->getTexture(),
		this->playerRaceTextBox->getX(), this->playerRaceTextBox->getY());
	renderer.drawOriginal(this->playerClassTextBox->getTexture(),
		this->playerClassTextBox->getX(), this->playerClassTextBox->getY());
}
