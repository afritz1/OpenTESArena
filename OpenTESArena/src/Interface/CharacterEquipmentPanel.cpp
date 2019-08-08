#include "SDL.h"

#include "CharacterEquipmentPanel.h"
#include "CharacterPanel.h"
#include "CursorAlignment.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Assets/CIFFile.h"
#include "../Assets/ExeData.h"
#include "../Assets/MiscAssets.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
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

#include "components/debug/Debug.h"

CharacterEquipmentPanel::CharacterEquipmentPanel(Game &game)
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

		return std::make_unique<TextBox>(x, y, richText, game.getRenderer());
	}();

	this->playerRaceTextBox = [&game]()
	{
		const int x = 10;
		const int y = 17;

		const auto &player = game.getGameData().getPlayer();
		const auto &exeData = game.getMiscAssets().getExeData();
		const std::string &text = exeData.races.singularNames.at(player.getRaceID());

		const RichTextString richText(
			text,
			FontName::Arena,
			Color(199, 199, 199),
			TextAlignment::Left,
			game.getFontManager());

		return std::make_unique<TextBox>(x, y, richText, game.getRenderer());
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

		return std::make_unique<TextBox>(x, y, richText, game.getRenderer());
	}();

	this->inventoryListBox = [&game]()
	{
		const int x = 16;
		const int y = 51;

		// @todo: make these visible to other panels that need them.
		const Color equipmentColor(211, 142, 0);
		const Color equipmentEquippedColor(235, 199, 52);
		const Color magicItemColor(69, 186, 190);
		const Color magicItemEquippedColor(138, 255, 255);
		const Color unequipableColor(199, 32, 0);

		const std::vector<std::pair<std::string, Color>> elements =
		{
			{ "Test slot 1", equipmentColor },
			{ "Test slot 2", equipmentEquippedColor },
			{ "Test slot 3", magicItemColor },
			{ "Test slot 4", magicItemEquippedColor },
			{ "Test slot 5", unequipableColor },
			{ "Test slot 6", unequipableColor },
			{ "Test slot 7", equipmentColor },
			{ "Test slot 8", equipmentColor },
			{ "Test slot 9", magicItemColor },
			{ "Test slot 10", magicItemEquippedColor }
		};

		const int maxDisplayed = 7;
		const int rowSpacing = 3;
		return std::make_unique<ListBox>(x, y, elements, FontName::Teeny, maxDisplayed,
			rowSpacing, game.getFontManager(), game.getRenderer());
	}();

	this->backToStatsButton = []()
	{
		int x = 0;
		int y = 188;
		int width = 47;
		int height = 12;
		auto function = [](Game &game)
		{
			game.setPanel<CharacterPanel>(game);
		};
		return Button<Game&>(x, y, width, height, function);
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
		return Button<>(x, y, width, height, function);
	}();

	this->dropButton = []()
	{
		int x = 123;
		int y = 188;
		int width = 48;
		int height = 12;
		auto function = [](Game &game, int index)
		{
			// Nothing yet.
			// The index parameter will point to which item in the list to drop.
		};
		return Button<Game&, int>(x, y, width, height, function);
	}();

	this->scrollDownButton = []()
	{
		Int2 center(16, 131);
		int width = 9;
		int height = 9;
		auto function = [](ListBox &invListBox)
		{
			if ((invListBox.getScrollIndex() + invListBox.getMaxDisplayedCount()) <
				invListBox.getElementCount())
			{
				invListBox.scrollDown();
			}
		};
		return Button<ListBox&>(center, width, height, function);
	}();

	this->scrollUpButton = []()
	{
		Int2 center(152, 131);
		int width = 9;
		int height = 9;
		auto function = [](ListBox &invListBox)
		{
			if (invListBox.getScrollIndex() > 0)
			{
				invListBox.scrollUp();
			}
		};
		return Button<ListBox&>(center, width, height, function);
	}();

	// Get pixel offsets for each head.
	const auto &player = this->getGame().getGameData().getPlayer();
	const std::string &headsFilename = PortraitFile::getHeads(
		player.getGenderName(), player.getRaceID(), false);

	CIFFile cifFile;
	if (!cifFile.init(headsFilename.c_str()))
	{
		DebugCrash("Could not init .CIF file \"" + headsFilename + "\".");
	}

	for (int i = 0; i < cifFile.getImageCount(); i++)
	{
		this->headOffsets.push_back(Int2(cifFile.getXOffset(i), cifFile.getYOffset(i)));
	}
}

std::pair<const Texture*, CursorAlignment> CharacterEquipmentPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default), renderer);
	return std::make_pair(&texture, CursorAlignment::TopLeft);
}

void CharacterEquipmentPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool tabPressed = inputManager.keyPressed(e, SDLK_TAB);

	if (escapePressed || tabPressed)
	{
		this->backToStatsButton.click(this->getGame());
	}

	const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	const bool mouseWheeledUp = inputManager.mouseWheeledUp(e);
	const bool mouseWheeledDown = inputManager.mouseWheeledDown(e);

	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 mouseOriginalPoint = this->getGame().getRenderer()
		.nativeToOriginal(mousePosition);

	if (leftClick)
	{
		if (this->backToStatsButton.contains(mouseOriginalPoint))
		{
			this->backToStatsButton.click(this->getGame());
		}
		else if (this->spellbookButton.contains(mouseOriginalPoint))
		{
			this->spellbookButton.click();
		}
		else if (this->dropButton.contains(mouseOriginalPoint))
		{
			// Eventually give the index of the clicked item instead.
			this->dropButton.click(this->getGame(), 0);
		}
		else if (this->scrollUpButton.contains(mouseOriginalPoint))
		{
			this->scrollUpButton.click(*this->inventoryListBox.get());
		}
		else if (this->scrollDownButton.contains(mouseOriginalPoint))
		{
			this->scrollDownButton.click(*this->inventoryListBox.get());
		}
	}
	else if (mouseWheeledUp)
	{
		this->scrollUpButton.click(*this->inventoryListBox.get());
	}
	else if (mouseWheeledDown)
	{
		this->scrollDownButton.click(*this->inventoryListBox.get());
	}
}

void CharacterEquipmentPanel::render(Renderer &renderer)
{
	DebugAssert(this->getGame().gameDataIsActive());

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
		PaletteFile::fromName(PaletteName::CharSheet), renderer).at(player.getPortraitID());
	const auto &body = textureManager.getTexture(bodyFilename, renderer);
	const auto &shirt = textureManager.getTexture(shirtFilename, renderer);
	const auto &pants = textureManager.getTexture(pantsFilename, renderer);
	renderer.drawOriginal(body, Renderer::ORIGINAL_WIDTH - body.getWidth(), 0);
	renderer.drawOriginal(pants, pantsOffset.x, pantsOffset.y);
	renderer.drawOriginal(head, headOffset.x, headOffset.y);
	renderer.drawOriginal(shirt, shirtOffset.x, shirtOffset.y);

	// Draw character equipment background.
	const auto &equipmentBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::CharacterEquipment), renderer);
	renderer.drawOriginal(equipmentBackground);

	// Draw text boxes: player name, race, class.
	renderer.drawOriginal(this->playerNameTextBox->getTexture(),
		this->playerNameTextBox->getX(), this->playerNameTextBox->getY());
	renderer.drawOriginal(this->playerRaceTextBox->getTexture(),
		this->playerRaceTextBox->getX(), this->playerRaceTextBox->getY());
	renderer.drawOriginal(this->playerClassTextBox->getTexture(),
		this->playerClassTextBox->getX(), this->playerClassTextBox->getY());
	
	// Draw inventory list box.
	const Int2 &inventoryListBoxPoint = this->inventoryListBox->getPoint();
	renderer.drawOriginal(this->inventoryListBox->getTexture(),
		inventoryListBoxPoint.x, inventoryListBoxPoint.y);
}
