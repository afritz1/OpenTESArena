#include <array>

#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "MainQuestSplashPanel.h"
#include "TextAlignment.h"
#include "Texture.h"
#include "../Assets/ExeData.h"
#include "../Game/Game.h"
#include "../Game/GameData.h"
#include "../Math/Random.h"
#include "../Media/FontName.h"
#include "../Media/MusicUtils.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"

#include "components/utilities/String.h"

MainQuestSplashPanel::MainQuestSplashPanel(Game &game, int provinceID)
	: Panel(game)
{
	this->textBox = [&game, provinceID]()
	{
		const std::string text = [&game, provinceID]()
		{
			const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
			const auto &textAssetLibrary = game.getTextAssetLibrary();

			// @todo: maybe don't split these two strings in the first place. And convert
			// the carriage return to a newline instead of removing it.
			const std::pair<std::string, std::string> &pair =
				[provinceID, &binaryAssetLibrary, &textAssetLibrary]()
			{
				const auto &exeData = binaryAssetLibrary.getExeData();
				const int index = exeData.travel.staffDungeonSplashIndices.at(provinceID);
				return textAssetLibrary.getDungeonTxtDungeons().at(index);
			}();

			return pair.first + '\n' + pair.second;
		}();

		const int lineSpacing = 1;
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			FontName::Teeny,
			Color(195, 158, 0),
			TextAlignment::Center,
			lineSpacing,
			fontLibrary);
		
		const int x = (ArenaRenderUtils::SCREEN_WIDTH / 2) - (richText.getDimensions().x / 2);
		const int y = 133;
		return std::make_unique<TextBox>(x, y, richText, fontLibrary, game.getRenderer());
	}();

	this->exitButton = []()
	{
		const int x = 272;
		const int y = 183;
		const int width = 43;
		const int height = 13;
		auto function = [](Game &game)
		{
			// Choose random dungeon music and enter game world.
			const MusicLibrary &musicLibrary = game.getMusicLibrary();
			const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(
				MusicDefinition::Type::Dungeon, game.getRandom());

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing dungeon music.");
			}

			AudioManager &audioManager = game.getAudioManager();
			audioManager.setMusic(musicDef);

			game.setPanel<GameWorldPanel>(game);
		};
		return Button<Game&>(x, y, width, height, function);
	}();

	// Get the filename of the staff dungeon splash image.
	this->splashFilename = [&game, provinceID]()
	{
		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
		const int index = exeData.travel.staffDungeonSplashIndices.at(provinceID);
		return String::toUppercase(exeData.travel.staffDungeonSplashes.at(index));
	}();
}

std::optional<Panel::CursorData> MainQuestSplashPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void MainQuestSplashPanel::handleEvent(const SDL_Event &e)
{
	// When the exit button is clicked, go to the game world panel.
	const auto &inputManager = this->getGame().getInputManager();
	const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		if (this->exitButton.contains(originalPoint))
		{
			this->exitButton.click(this->getGame());
		}
	}
}

void MainQuestSplashPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw staff dungeon splash image.
	auto &textureManager = this->getGame().getTextureManager();
	const std::string &textureName = this->splashFilename;
	const std::string &paletteName = textureName;
	const std::optional<PaletteID> splashPaletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!splashPaletteID.has_value())
	{
		DebugLogError("Couldn't get splash palette ID for \"" + paletteName + "\".");
		return;
	}

	const std::optional<TextureBuilderID> splashTextureBuilderID =
		textureManager.tryGetTextureBuilderID(textureName.c_str());
	if (!splashTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get splash texture builder ID for \"" + textureName + "\".");
		return;
	}

	renderer.drawOriginal(*splashTextureBuilderID, *splashPaletteID, textureManager);

	// Draw text.
	renderer.drawOriginal(this->textBox->getTexture(), this->textBox->getX(), this->textBox->getY());
}
