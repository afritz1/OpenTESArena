#include "SDL.h"

#include "CharacterEquipmentPanel.h"
#include "CharacterPanel.h"
#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "Texture.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/CIFFile.h"
#include "../Assets/ExeData.h"
#include "../Entities/CharacterClassDefinition.h"
#include "../Entities/CharacterClassLibrary.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Media/Color.h"
#include "../Media/FontLibrary.h"
#include "../Media/FontName.h"
#include "../Media/PortraitFile.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"

CharacterPanel::CharacterPanel(Game &game)
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
			const auto &charClassLibrary = game.getCharacterClassLibrary();
			const auto &player = game.getGameData().getPlayer();
			return charClassLibrary.getDefinition(player.getCharacterClassDefID());
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

	this->doneButton = []()
	{
		Int2 center(25, ArenaRenderUtils::SCREEN_HEIGHT - 15);
		int width = 21;
		int height = 13;
		auto function = [](Game &game)
		{
			game.setPanel<GameWorldPanel>(game);
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
			game.setPanel<CharacterEquipmentPanel>(game);
		};
		return Button<Game&>(x, y, width, height, function);
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

std::optional<Panel::CursorData> CharacterPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void CharacterPanel::handleEvent(const SDL_Event &e)
{
	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool tabPressed = inputManager.keyPressed(e, SDLK_TAB);

	if (escapePressed || tabPressed)
	{
		this->doneButton.click(game);
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = game.getRenderer().nativeToOriginal(mousePosition);

		if (this->doneButton.contains(mouseOriginalPoint))
		{
			this->doneButton.click(game);
		}
		else if (this->nextPageButton.contains(mouseOriginalPoint))
		{
			this->nextPageButton.click(game);
		}
	}
}

void CharacterPanel::render(Renderer &renderer)
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

	auto &textureManager = game.getTextureManager();
	const std::string &charSheetPaletteFilename = ArenaPaletteName::CharSheet;
	const std::optional<PaletteID> charSheetPaletteID = textureManager.tryGetPaletteID(charSheetPaletteFilename.c_str());
	if (!charSheetPaletteID.has_value())
	{
		DebugLogError("Couldn't get character sheet palette ID \"" + charSheetPaletteFilename + "\".");
		return;
	}

	const std::string &bodyFilename = PortraitFile::getBody(player.isMale(), player.getRaceID());
	const std::string &shirtFilename = PortraitFile::getShirt(player.isMale(), charClassDef.canCastMagic());
	const std::string &pantsFilename = PortraitFile::getPants(player.isMale());

	// Get pixel offsets for each clothes texture.
	const Int2 shirtOffset = PortraitFile::getShirtOffset(player.isMale(), charClassDef.canCastMagic());
	const Int2 pantsOffset = PortraitFile::getPantsOffset(player.isMale());

	// Get all texture IDs in advance of any texture references.
	const TextureBuilderID headTextureBuilderID = [this, &textureManager, &player]()
	{
		const std::string &headsFilename = PortraitFile::getHeads(player.isMale(), player.getRaceID(), false);
		const std::optional<TextureBuilderIdGroup> headTextureBuilderIDs =
			textureManager.tryGetTextureBuilderIDs(headsFilename.c_str());
		if (!headTextureBuilderIDs.has_value())
		{
			DebugCrash("Couldn't get head texture builder IDs for \"" + headsFilename + "\".");
		}

		return headTextureBuilderIDs->getID(player.getPortraitID());
	}();

	const std::string &statsBackgroundTextureFilename = ArenaTextureName::CharacterStats;
	const std::string &nextPageTextureFilename = ArenaTextureName::NextPage;
	const std::optional<TextureBuilderID> bodyTextureBuilderID =
		textureManager.tryGetTextureBuilderID(bodyFilename.c_str());
	const std::optional<TextureBuilderID> shirtTextureBuilderID =
		textureManager.tryGetTextureBuilderID(shirtFilename.c_str());
	const std::optional<TextureBuilderID> pantsTextureBuilderID = 
		textureManager.tryGetTextureBuilderID(pantsFilename.c_str());
	const std::optional<TextureBuilderID> statsBackgroundTextureID = 
		textureManager.tryGetTextureBuilderID(statsBackgroundTextureFilename.c_str());
	const std::optional<TextureBuilderID> nextPageTextureID =
		textureManager.tryGetTextureBuilderID(nextPageTextureFilename.c_str());
	DebugAssert(bodyTextureBuilderID.has_value());
	DebugAssert(shirtTextureBuilderID.has_value());
	DebugAssert(pantsTextureBuilderID.has_value());
	DebugAssert(statsBackgroundTextureID.has_value());
	DebugAssert(nextPageTextureID.has_value());

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

	// Draw character stats background.
	renderer.drawOriginal(*statsBackgroundTextureID, *charSheetPaletteID, textureManager);

	// Draw "Next Page" texture.
	renderer.drawOriginal(*nextPageTextureID, *charSheetPaletteID, 108, 179, textureManager);

	// Draw text boxes: player name, race, class.
	renderer.drawOriginal(this->playerNameTextBox->getTexture(),
		this->playerNameTextBox->getX(), this->playerNameTextBox->getY());
	renderer.drawOriginal(this->playerRaceTextBox->getTexture(),
		this->playerRaceTextBox->getX(), this->playerRaceTextBox->getY());
	renderer.drawOriginal(this->playerClassTextBox->getTexture(),
		this->playerClassTextBox->getX(), this->playerClassTextBox->getY());
}
