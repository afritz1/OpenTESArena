#include <cassert>

#include "SDL.h"

#include "CharacterEquipmentPanel.h"

#include "CharacterPanel.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Assets/CIFFile.h"
#include "../Assets/ExeStrings.h"
#include "../Assets/TextAssets.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/Game.h"
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

CharacterEquipmentPanel::CharacterEquipmentPanel(Game *game)
	: Panel(game), headOffsets()
{
	this->playerNameTextBox = [game]()
	{
		int x = 10;
		int y = 8;
		Color color(199, 199, 199);
		std::string text = game->getGameData().getPlayer().getDisplayName();
		auto &font = game->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Left;
		return std::unique_ptr<TextBox>(new TextBox(
			x,
			y,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->playerRaceTextBox = [game]()
	{
		int x = 10;
		int y = 17;
		Color color(199, 199, 199);
		auto &player = game->getGameData().getPlayer();
		std::string text = game->getTextAssets().getAExeSegment(
			ExeStrings::RaceNamesSingular.at(player.getRaceID()));
		auto &font = game->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Left;
		return std::unique_ptr<TextBox>(new TextBox(
			x,
			y,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->playerClassTextBox = [game]()
	{
		int x = 10;
		int y = 26;
		Color color(199, 199, 199);
		std::string text = game->getGameData().getPlayer().getCharacterClass()
			.getDisplayName();
		auto &font = game->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Left;
		return std::unique_ptr<TextBox>(new TextBox(
			x,
			y,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->backToStatsButton = []()
	{
		int x = 0;
		int y = 188;
		int width = 47;
		int height = 12;
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> characterPanel(new CharacterPanel(game));
			game->setPanel(std::move(characterPanel));
		};
		return std::unique_ptr<Button<Game*>>(
			new Button<Game*>(x, y, width, height, function));
	}();

	this->spellbookButton = []()
	{
		int x = 47;
		int y = 188;
		int width = 76;
		int height = 12;
		auto function = []()
		{
			// Nothing yet.
			// Might eventually take an argument for a panel?
		};
		return std::unique_ptr<Button<>>(new Button<>(x, y, width, height, function));
	}();

	this->dropButton = []()
	{
		int x = 123;
		int y = 188;
		int width = 48;
		int height = 12;
		auto function = [](Game *game, int index)
		{
			// Nothing yet.
			// The index parameter will point to which item in the list to drop.
		};
		return std::unique_ptr<Button<Game*, int>>(
			new Button<Game*, int>(x, y, width, height, function));
	}();

	this->scrollDownButton = []()
	{
		Int2 center(16, 131);
		int width = 9;
		int height = 9;
		auto function = [](CharacterEquipmentPanel* panel)
		{
			// Nothing yet.
		};
		return std::unique_ptr<Button<CharacterEquipmentPanel*>>(
			new Button<CharacterEquipmentPanel*>(center, width, height, function));
	}();

	this->scrollUpButton = []()
	{
		Int2 center(152, 131);
		int width = 9;
		int height = 9;
		auto function = [](CharacterEquipmentPanel* panel)
		{
			// Nothing yet.
		};
		return std::unique_ptr<Button<CharacterEquipmentPanel*>>(
			new Button<CharacterEquipmentPanel*>(center, width, height, function));
	}();

	// Get pixel offsets for each head.
	const auto &player = this->getGame()->getGameData().getPlayer();
	const std::string &headsFilename = PortraitFile::getHeads(
		player.getGenderName(), player.getRaceID(), false);
	CIFFile cifFile(headsFilename, Palette());

	for (int i = 0; i < cifFile.getImageCount(); ++i)
	{
		this->headOffsets.push_back(Int2(cifFile.getXOffset(i), cifFile.getYOffset(i)));
	}
}

CharacterEquipmentPanel::~CharacterEquipmentPanel()
{

}

void CharacterEquipmentPanel::handleEvent(const SDL_Event &e)
{
	bool escapePressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_ESCAPE);
	bool tabPressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_TAB);

	if (escapePressed || tabPressed)
	{
		this->backToStatsButton->click(this->getGame());
	}

	bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = this->getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
			.nativePointToOriginal(mousePosition);

		if (this->backToStatsButton->contains(mouseOriginalPoint))
		{
			this->backToStatsButton->click(this->getGame());
		}
		else if (this->spellbookButton->contains(mouseOriginalPoint))
		{
			this->spellbookButton->click();
		}
		else if (this->dropButton->contains(mouseOriginalPoint))
		{
			// Eventually give the index of the clicked item instead.
			this->dropButton->click(this->getGame(), 0);
		}
		else if (this->scrollUpButton->contains(mouseOriginalPoint))
		{
			this->scrollUpButton->click(this);
		}
		else if (this->scrollDownButton->contains(mouseOriginalPoint))
		{
			this->scrollDownButton->click(this);
		}
	}
}

void CharacterEquipmentPanel::render(Renderer &renderer)
{
	assert(this->getGame()->gameDataIsActive());

	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGame()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::CharSheet));

	// Get a reference to the active player data.
	const auto &player = this->getGame()->getGameData().getPlayer();

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
	renderer.drawToOriginal(body.get(), Renderer::ORIGINAL_WIDTH - body.getWidth(), 0);
	renderer.drawToOriginal(pants.get(), pantsOffset.x, pantsOffset.y);
	renderer.drawToOriginal(head.get(), headOffset.x, headOffset.y);
	renderer.drawToOriginal(shirt.get(), shirtOffset.x, shirtOffset.y);

	// Draw character equipment background.
	const auto &equipmentBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::CharacterEquipment));
	renderer.drawToOriginal(equipmentBackground.get());

	// Draw text boxes: player name, race, class.
	renderer.drawToOriginal(this->playerNameTextBox->getTexture(),
		this->playerNameTextBox->getX(), this->playerNameTextBox->getY());
	renderer.drawToOriginal(this->playerRaceTextBox->getTexture(),
		this->playerRaceTextBox->getX(), this->playerRaceTextBox->getY());
	renderer.drawToOriginal(this->playerClassTextBox->getTexture(),
		this->playerClassTextBox->getX(), this->playerClassTextBox->getY());

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
