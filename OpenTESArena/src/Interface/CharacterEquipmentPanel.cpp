#include "SDL.h"

#include "CharacterEquipmentPanel.h"
#include "CharacterPanel.h"
#include "CursorAlignment.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "Texture.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/CIFFile.h"
#include "../Assets/ExeData.h"
#include "../Entities/CharacterClassDefinition.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Media/FontLibrary.h"
#include "../Media/FontName.h"
#include "../Media/PortraitFile.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"

CharacterEquipmentPanel::CharacterEquipmentPanel(Game &game)
	: Panel(game)
{
	this->playerNameTextBox = [&game]()
	{
		const int x = 10;
		const int y = 8;

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			game.getGameData().getPlayer().getDisplayName(),
			FontName::Arena,
			Color(199, 199, 199),
			TextAlignment::Left,
			fontLibrary);

		return std::make_unique<TextBox>(x, y, richText, fontLibrary, game.getRenderer());
	}();

	this->playerRaceTextBox = [&game]()
	{
		const int x = 10;
		const int y = 17;

		const auto &player = game.getGameData().getPlayer();
		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
		const std::string &text = exeData.races.singularNames.at(player.getRaceID());

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			FontName::Arena,
			Color(199, 199, 199),
			TextAlignment::Left,
			fontLibrary);

		return std::make_unique<TextBox>(x, y, richText, fontLibrary, game.getRenderer());
	}();

	this->playerClassTextBox = [&game]()
	{
		const int x = 10;
		const int y = 26;

		const auto &charClassDef = [&game]() -> const CharacterClassDefinition&
		{
			const auto &player = game.getGameData().getPlayer();
			const int playerCharClassDefID = player.getCharacterClassDefID();
			const auto &charClassLibrary = game.getCharacterClassLibrary();
			return charClassLibrary.getDefinition(playerCharClassDefID);
		}();

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			charClassDef.getName(),
			FontName::Arena,
			Color(199, 199, 199),
			TextAlignment::Left,
			fontLibrary);

		return std::make_unique<TextBox>(x, y, richText, fontLibrary, game.getRenderer());
	}();

	this->inventoryListBox = [&game]()
	{
		const int x = 14;
		const int y = 50;

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
			rowSpacing, game.getFontLibrary(), game.getRenderer());
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
		player.isMale(), player.getRaceID(), false);

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

std::optional<Panel::CursorData> CharacterEquipmentPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
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

	// Get the filenames for the portrait and clothes.
	auto &game = this->getGame();
	const auto &player = game.getGameData().getPlayer();
	const auto &charClassDef = [&game, &player]() -> const CharacterClassDefinition&
	{
		const auto &charClassLibrary = game.getCharacterClassLibrary();
		return charClassLibrary.getDefinition(player.getCharacterClassDefID());
	}();

	// Get pixel offsets for each clothes texture.
	const Int2 shirtOffset = PortraitFile::getShirtOffset(player.isMale(), charClassDef.canCastMagic());
	const Int2 pantsOffset = PortraitFile::getPantsOffset(player.isMale());

	auto &textureManager = game.getTextureManager();
	const std::string &charSheetPaletteFilename = ArenaPaletteName::CharSheet;
	const std::optional<PaletteID> charSheetPaletteID = textureManager.tryGetPaletteID(charSheetPaletteFilename.c_str());
	if (!charSheetPaletteID.has_value())
	{
		DebugLogError("Couldn't get character sheet palette ID \"" + charSheetPaletteFilename + "\".");
		return;
	}

	// Get all texture IDs in advance of any texture references.
	const TextureBuilderID headTextureBuilderID = [this, &textureManager, &player]()
	{
		const std::string &headsFilename = PortraitFile::getHeads(player.isMale(), player.getRaceID(), false);
		const std::optional<TextureBuilderIdGroup> headTextureBuilderIDs =
			textureManager.tryGetTextureBuilderIDs(headsFilename.c_str());
		if (!headTextureBuilderIDs.has_value())
		{
			DebugCrash("Couldn't get head texture builder IDs from \"" + headsFilename + "\".");
		}

		return headTextureBuilderIDs->getID(player.getPortraitID());
	}();

	const std::string &bodyFilename = PortraitFile::getBody(player.isMale(), player.getRaceID());
	const std::string &shirtFilename = PortraitFile::getShirt(player.isMale(), charClassDef.canCastMagic());
	const std::string &pantsFilename = PortraitFile::getPants(player.isMale());
	const std::string &charEquipmentFilename = ArenaTextureName::CharacterEquipment;

	const std::optional<TextureBuilderID> bodyTextureBuilderID =
		textureManager.tryGetTextureBuilderID(bodyFilename.c_str());
	const std::optional<TextureBuilderID> shirtTextureBuilderID = 
		textureManager.tryGetTextureBuilderID(shirtFilename.c_str());
	const std::optional<TextureBuilderID> pantsTextureBuilderID =
		textureManager.tryGetTextureBuilderID(pantsFilename.c_str());
	const std::optional<TextureBuilderID> equipmentBgTextureBuilderID =
		textureManager.tryGetTextureBuilderID(charEquipmentFilename.c_str());
	DebugAssert(bodyTextureBuilderID.has_value());
	DebugAssert(shirtTextureBuilderID.has_value());
	DebugAssert(pantsTextureBuilderID.has_value());
	DebugAssert(equipmentBgTextureBuilderID.has_value());

	const int bodyTextureX = [&textureManager, &bodyTextureBuilderID]()
	{
		const TextureBuilder &bodyTexture = textureManager.getTextureBuilderHandle(*bodyTextureBuilderID);
		return ArenaRenderUtils::SCREEN_WIDTH - bodyTexture.getWidth();
	}();

	const Int2 &headOffset = this->headOffsets.at(player.getPortraitID());

	// Draw the current portrait and clothes.
	renderer.drawOriginal(*bodyTextureBuilderID, *charSheetPaletteID, bodyTextureX, 0, textureManager);
	renderer.drawOriginal(*pantsTextureBuilderID, *charSheetPaletteID, pantsOffset.x, pantsOffset.y, textureManager);
	renderer.drawOriginal(headTextureBuilderID, *charSheetPaletteID, headOffset.x, headOffset.y, textureManager);
	renderer.drawOriginal(*shirtTextureBuilderID, *charSheetPaletteID, shirtOffset.x, shirtOffset.y, textureManager);

	// Draw character equipment background.
	renderer.drawOriginal(*equipmentBgTextureBuilderID, *charSheetPaletteID, textureManager);

	// Draw text boxes: player name, race, class.
	renderer.drawOriginal(this->playerNameTextBox->getTexture(),
		this->playerNameTextBox->getX(), this->playerNameTextBox->getY());
	renderer.drawOriginal(this->playerRaceTextBox->getTexture(),
		this->playerRaceTextBox->getX(), this->playerRaceTextBox->getY());
	renderer.drawOriginal(this->playerClassTextBox->getTexture(),
		this->playerClassTextBox->getX(), this->playerClassTextBox->getY());
	
	// Draw inventory list box.
	const Int2 &inventoryListBoxPoint = this->inventoryListBox->getPoint();
	renderer.drawOriginal(this->inventoryListBox->getTexture(), inventoryListBoxPoint.x, inventoryListBoxPoint.y);
}
