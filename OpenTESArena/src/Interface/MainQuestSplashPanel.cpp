#include <array>

#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "MainQuestSplashPanel.h"
#include "TextAlignment.h"
#include "../Assets/ExeData.h"
#include "../Game/Game.h"
#include "../Game/GameData.h"
#include "../Math/Random.h"
#include "../Media/FontName.h"
#include "../Media/MusicName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"
#include "../Utilities/String.h"

MainQuestSplashPanel::MainQuestSplashPanel(Game &game, int provinceID)
	: Panel(game)
{
	this->textBox = [&game, provinceID]()
	{
		const std::string text = [&game, provinceID]()
		{
			const auto &miscAssets = game.getMiscAssets();

			// To do: maybe don't split these two strings in the first place. And convert
			// the carriage return to a newline instead of removing it.
			const std::pair<std::string, std::string> &pair = [provinceID, &miscAssets]()
			{
				const auto &exeData = miscAssets.getExeData();
				const int index = exeData.travel.staffDungeonSplashIndices.at(provinceID);
				return miscAssets.getDungeonTxtDungeons().at(index);
			}();

			return pair.first + '\n' + pair.second;
		}();

		const int lineSpacing = 1;

		const RichTextString richText(
			text,
			FontName::Teeny,
			Color(195, 158, 0),
			TextAlignment::Center,
			lineSpacing,
			game.getFontManager());
		
		const int x = (Renderer::ORIGINAL_WIDTH / 2) - (richText.getDimensions().x / 2);
		const int y = 133;
		return std::make_unique<TextBox>(x, y, richText, game.getRenderer());
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
			Random random;
			const MusicName musicName = GameData::getDungeonMusicName(random);
			game.setMusic(musicName);
			game.setPanel<GameWorldPanel>(game);
		};
		return Button<Game&>(x, y, width, height, function);
	}();

	// Get the filename of the staff dungeon splash image.
	this->splashFilename = [&game, provinceID]()
	{
		const auto &exeData = game.getMiscAssets().getExeData();
		const int index = exeData.travel.staffDungeonSplashIndices.at(provinceID);
		return String::toUppercase(exeData.travel.staffDungeonSplashes.at(index));
	}();
}

std::pair<SDL_Texture*, CursorAlignment> MainQuestSplashPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default), renderer);
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
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

	// Set palette.
	auto &game = this->getGame();
	auto &textureManager = game.getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw staff dungeon splash image.
	const auto &splashImage = textureManager.getTexture(
		this->splashFilename, PaletteFile::fromName(PaletteName::BuiltIn), renderer);
	renderer.drawOriginal(splashImage.get());

	// Draw text.
	renderer.drawOriginal(this->textBox->getTexture(),
		this->textBox->getX(), this->textBox->getY());
}
