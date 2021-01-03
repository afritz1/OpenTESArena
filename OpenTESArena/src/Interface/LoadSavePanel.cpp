#include "SDL.h"

#include "CursorAlignment.h"
#include "LoadSavePanel.h"
#include "MainMenuPanel.h"
#include "PauseMenuPanel.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextSubPanel.h"
#include "Texture.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaSave.h"
#include "../Assets/ArenaTextureName.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/FontLibrary.h"
#include "../Media/FontName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Platform.h"

#include "components/debug/Debug.h"
#include "components/utilities/File.h"
#include "components/utilities/String.h"

LoadSavePanel::LoadSavePanel(Game &game, LoadSavePanel::Type type)
	: Panel(game)
{
	// Load each name in NAMES.DAT.
	// - @todo: also check associated save files for each ".0x" index.
	const std::string savesPath = [&game]()
	{
		const std::string &arenaSavesPath = game.getOptions().getMisc_ArenaSavesPath();
		const bool savesPathIsRelative = File::pathIsRelative(arenaSavesPath.c_str());
		const std::string path = (savesPathIsRelative ? Platform::getBasePath() : "") + arenaSavesPath;
		return String::addTrailingSlashIfMissing(path);
	}();

	if (File::exists((savesPath + "NAMES.DAT").c_str()))
	{
		const auto names = ArenaSave::loadNAMES(savesPath);
		for (int i = 0; i < LoadSavePanel::SlotCount; i++)
		{
			const auto &entry = names->entries.at(i);

			const Int2 center(ArenaRenderUtils::SCREEN_WIDTH / 2, 8 + (i * 14));
			const auto &fontLibrary = game.getFontLibrary();
			const RichTextString richText(
				std::string(entry.name.data()),
				FontName::Arena,
				Color::White,
				TextAlignment::Center,
				fontLibrary);

			// Create text box from entry text.
			this->saveTextBoxes.at(i) = std::make_unique<TextBox>(
				center, richText, fontLibrary, game.getRenderer());
		}
	}
	else
	{
		DebugLog("No NAMES.DAT found in \"" + savesPath + "\".");
	}

	this->confirmButton = []()
	{
		auto function = [](Game &game, int index)
		{
			// Temp: draw not implemented pop-up.
			const Int2 center(
				ArenaRenderUtils::SCREEN_WIDTH / 2,
				ArenaRenderUtils::SCREEN_HEIGHT / 2);
			
			const int lineSpacing = 1;
			const RichTextString richText(
				"Not implemented\n(save slot " + std::to_string(index) + ")",
				FontName::Arena,
				Color(150, 97, 0),
				TextAlignment::Center,
				lineSpacing,
				game.getFontLibrary());

			auto popUpFunction = [](Game &game)
			{
				game.popSubPanel();
			};

			Texture texture = TextureUtils::generate(TextureUtils::PatternType::Dark,
				richText.getDimensions().x + 10, richText.getDimensions().y + 10,
				game.getTextureManager(), game.getRenderer());

			auto notImplPopUp = std::make_unique<TextSubPanel>(
				game, center, richText, popUpFunction, std::move(texture), center);

			game.pushSubPanel(std::move(notImplPopUp));
		};
		return Button<Game&, int>(function);
	}();

	this->backButton = []()
	{
		auto function = [](Game &game)
		{
			// Back button behavior depends on whether game data is active.
			if (game.gameDataIsActive())
			{
				game.setPanel<PauseMenuPanel>(game);
			}
			else
			{
				game.setPanel<MainMenuPanel>(game);
			}
		};
		return Button<Game&>(function);
	}();

	this->type = type;
}

int LoadSavePanel::getClickedIndex(const Int2 &point)
{
	int y = 2;
	for (int i = 0; i < LoadSavePanel::SlotCount; i++)
	{
		const int x = 2;
		const int clickWidth = 316;
		const int clickHeight = 13;
		const int ySpacing = 1;

		const Rect rect(x, y, clickWidth, clickHeight);
		if (rect.contains(point))
		{
			return i;
		}
		else
		{
			y += clickHeight + ySpacing;
		}
	}

	return -1;
}

std::optional<Panel::CursorData> LoadSavePanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void LoadSavePanel::handleEvent(const SDL_Event &e)
{
	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	const bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	if (escapePressed || rightClick)
	{
		this->backButton.click(game);
	}
	else if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPoint = game.getRenderer().nativeToOriginal(mousePosition);

		// Listen for saved game click.
		const int clickedIndex = LoadSavePanel::getClickedIndex(originalPoint);
		if (clickedIndex >= 0)
		{
			this->confirmButton.click(game, clickedIndex);
		}
	}
}

void LoadSavePanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw slots background.
	auto &textureManager = this->getGame().getTextureManager();
	const std::string &paletteFilename = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for \"" + paletteFilename + "\".");
		return;
	}

	const std::string &textureFilename = ArenaTextureName::LoadSave;
	const std::optional<TextureBuilderID> textureBuilderID =
		textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugLogError("Couldn't get texture builder ID for \"" + textureFilename + "\".");
		return;
	}

	renderer.drawOriginal(*textureBuilderID, *paletteID, textureManager);

	// Draw save text.
	for (const auto &textBox : this->saveTextBoxes)
	{
		if (textBox.get() != nullptr)
		{
			const Rect textBoxRect = textBox->getRect();
			renderer.drawOriginal(textBox->getTexture(), textBoxRect.getLeft(), textBoxRect.getTop());
		}
	}
}
