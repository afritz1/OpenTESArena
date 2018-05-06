#include "SDL.h"

#include "CursorAlignment.h"
#include "LoadSavePanel.h"
#include "MainMenuPanel.h"
#include "PauseMenuPanel.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextSubPanel.h"
#include "../Assets/ArenaSave.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/MusicName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"
#include "../Utilities/Debug.h"
#include "../Utilities/File.h"
#include "../Utilities/Platform.h"
#include "../Utilities/String.h"

const int LoadSavePanel::SlotCount = 10;

LoadSavePanel::LoadSavePanel(Game &game, LoadSavePanel::Type type)
	: Panel(game)
{
	// Load each name in NAMES.DAT.
	// - To do: also check associated save files for each ".0x" index.
	const std::string savesPath = [&game]()
	{
		const std::string &arenaSavesPath = game.getOptions().getMisc_ArenaSavesPath();
		const bool savesPathIsRelative = File::pathIsRelative(arenaSavesPath);
		const std::string path = (savesPathIsRelative ? Platform::getBasePath() : "") + arenaSavesPath;
		return String::addTrailingSlashIfMissing(path);
	}();

	if (File::exists(savesPath + "NAMES.DAT"))
	{
		const ArenaTypes::Names names = ArenaSave::loadNAMES(savesPath);
		for (int i = 0; i < LoadSavePanel::SlotCount; i++)
		{
			const auto &entry = names.entries.at(i);

			const Int2 center(Renderer::ORIGINAL_WIDTH / 2, 8 + (i * 14));
			const RichTextString richText(
				std::string(entry.name.data()),
				FontName::Arena,
				Color::White,
				TextAlignment::Center,
				game.getFontManager());

			// Create text box from entry text.
			this->saveTextBoxes.at(i) = std::make_unique<TextBox>(
				center, richText, game.getRenderer());
		}
	}
	else
	{
		DebugMention("No NAMES.DAT found in \"" + savesPath + "\".");
	}

	this->confirmButton = []()
	{
		auto function = [](Game &game, int index)
		{
			// Temp: draw not implemented pop-up.
			const Int2 center(
				Renderer::ORIGINAL_WIDTH / 2,
				Renderer::ORIGINAL_HEIGHT / 2);
			
			const int lineSpacing = 1;
			const RichTextString richText(
				"Not implemented\n(save slot " + std::to_string(index) + ")",
				FontName::Arena,
				Color(150, 97, 0),
				TextAlignment::Center,
				lineSpacing,
				game.getFontManager());

			auto popUpFunction = [](Game &game)
			{
				game.popSubPanel();
			};

			Texture texture = Texture::generate(Texture::PatternType::Dark,
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

std::pair<SDL_Texture*, CursorAlignment> LoadSavePanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default), renderer);
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
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

	// Set palette.
	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw slots background.
	const auto &slotsBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::LoadSave), renderer);
	renderer.drawOriginal(slotsBackground.get());

	// Draw save text.
	for (const auto &textBox : this->saveTextBoxes)
	{
		if (textBox.get() != nullptr)
		{
			const Rect textBoxRect = textBox->getRect();
			renderer.drawOriginal(textBox->getTexture(),
				textBoxRect.getLeft(), textBoxRect.getTop());
		}
	}
}
