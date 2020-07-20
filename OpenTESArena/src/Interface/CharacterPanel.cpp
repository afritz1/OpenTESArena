#include "SDL.h"

#include "CharacterEquipmentPanel.h"
#include "CharacterPanel.h"
#include "CursorAlignment.h"
#include "GameWorldPanel.h"
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

#include "components/debug/Debug.h"

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

	this->doneButton = []()
	{
		Int2 center(25, Renderer::ORIGINAL_HEIGHT - 15);
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

Panel::CursorData CharacterPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
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
			.nativeToOriginal(mousePosition);

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
	DebugAssert(this->getGame().gameDataIsActive());

	// Clear full screen.
	renderer.clear();

	// Get a reference to the active player data.
	const auto &player = this->getGame().getGameData().getPlayer();

	// Get the filenames for the portrait and clothes.
	const std::string &headsFilename = PortraitFile::getHeads(
		player.isMale(), player.getRaceID(), false);
	const std::string &bodyFilename = PortraitFile::getBody(
		player.isMale(), player.getRaceID());
	const std::string &shirtFilename = PortraitFile::getShirt(
		player.isMale(), player.getCharacterClass().canCastMagic());
	const std::string &pantsFilename = PortraitFile::getPants(player.isMale());

	// Get pixel offsets for each clothes texture.
	const Int2 shirtOffset = PortraitFile::getShirtOffset(
		player.isMale(), player.getCharacterClass().canCastMagic());
	const Int2 pantsOffset = PortraitFile::getPantsOffset(player.isMale());

	// Draw the current portrait and clothes.
	auto &textureManager = this->getGame().getTextureManager();
	const Int2 &headOffset = this->headOffsets.at(player.getPortraitID());
	const Texture &headTexture = [this, &textureManager, &headsFilename, &player]() -> const Texture&
	{
		const TextureManager::IdGroup<TextureID> headTextureIDs =
			this->getTextureIDs(headsFilename, PaletteFile::fromName(PaletteName::CharSheet));
		const TextureID headTextureID = headTextureIDs.startID + player.getPortraitID();
		return textureManager.getTexture(headTextureID);
	}();

	const Texture &bodyTexture = [this, &textureManager, &bodyFilename]() -> const Texture&
	{
		const TextureID bodyTextureID = this->getTextureID(
			bodyFilename, PaletteFile::fromName(PaletteName::CharSheet));
		return textureManager.getTexture(bodyTextureID);
	}();

	const Texture &shirtTexture = [this, &textureManager, &shirtFilename]() -> const Texture&
	{
		const TextureID shirtTextureID = this->getTextureID(
			shirtFilename, PaletteFile::fromName(PaletteName::CharSheet));
		return textureManager.getTexture(shirtTextureID);
	}();

	const Texture &pantsTexture = [this, &textureManager, &pantsFilename]() -> const Texture&
	{
		const TextureID pantsTextureID = this->getTextureID(
			pantsFilename, PaletteFile::fromName(PaletteName::CharSheet));
		return textureManager.getTexture(pantsTextureID);
	}();

	renderer.drawOriginal(bodyTexture, Renderer::ORIGINAL_WIDTH - bodyTexture.getWidth(), 0);
	renderer.drawOriginal(pantsTexture, pantsOffset.x, pantsOffset.y);
	renderer.drawOriginal(headTexture, headOffset.x, headOffset.y);
	renderer.drawOriginal(shirtTexture, shirtOffset.x, shirtOffset.y);

	// Draw character stats background.
	const TextureID statsBackgroundTextureID = this->getTextureID(
		TextureName::CharacterStats, PaletteName::CharSheet);
	const Texture &statsBackgroundTexture = textureManager.getTexture(statsBackgroundTextureID);
	renderer.drawOriginal(statsBackgroundTexture);

	// Draw "Next Page" texture.
	const TextureID nextPageTextureID = this->getTextureID(
		TextureName::NextPage, PaletteName::CharSheet);
	const Texture &nextPageTexture = textureManager.getTexture(nextPageTextureID);
	renderer.drawOriginal(nextPageTexture, 108, 179);

	// Draw text boxes: player name, race, class.
	renderer.drawOriginal(this->playerNameTextBox->getTexture(),
		this->playerNameTextBox->getX(), this->playerNameTextBox->getY());
	renderer.drawOriginal(this->playerRaceTextBox->getTexture(),
		this->playerRaceTextBox->getX(), this->playerRaceTextBox->getY());
	renderer.drawOriginal(this->playerClassTextBox->getTexture(),
		this->playerClassTextBox->getX(), this->playerClassTextBox->getY());
}
