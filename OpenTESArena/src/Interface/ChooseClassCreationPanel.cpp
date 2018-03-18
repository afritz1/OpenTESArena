#include <cassert>

#include "SDL.h"

#include "ChooseClassCreationPanel.h"
#include "ChooseClassPanel.h"
#include "CursorAlignment.h"
#include "MainMenuPanel.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Assets/ExeData.h"
#include "../Assets/MiscAssets.h"
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
#include "../Rendering/Surface.h"
#include "../Utilities/String.h"

ChooseClassCreationPanel::ChooseClassCreationPanel(Game &game)
	: Panel(game)
{
	this->parchment = Texture(Texture::generate(
		Texture::PatternType::Parchment, 180, 40, game.getTextureManager(), 
		game.getRenderer()));

	this->titleTextBox = [&game]()
	{
		const Int2 center((Renderer::ORIGINAL_WIDTH / 2) - 1, 80);

		const auto &exeData = game.getMiscAssets().getExeData();
		std::string text = exeData.charCreation.chooseClassCreation;
		text = String::replace(text, '\r', '\n');

		const int lineSpacing = 1;

		const RichTextString richText(
			text,
			FontName::A,
			Color(48, 12, 12),
			TextAlignment::Center,
			lineSpacing,
			game.getFontManager());

		return std::make_unique<TextBox>(center, richText, game.getRenderer());
	}();

	this->generateTextBox = [&game]()
	{
		const Int2 center((Renderer::ORIGINAL_WIDTH / 2) - 1, 120);

		const auto &exeData = game.getMiscAssets().getExeData();
		const std::string &text = exeData.charCreation.chooseClassCreationGenerate;

		const RichTextString richText(
			text,
			FontName::A,
			Color(48, 12, 12),
			TextAlignment::Center,
			game.getFontManager());

		return std::make_unique<TextBox>(center, richText, game.getRenderer());
	}();

	this->selectTextBox = [&game]()
	{
		const Int2 center((Renderer::ORIGINAL_WIDTH / 2) - 1, 160);

		const auto &exeData = game.getMiscAssets().getExeData();
		const std::string &text = exeData.charCreation.chooseClassCreationSelect;

		const RichTextString richText(
			text,
			FontName::A,
			Color(48, 12, 12),
			TextAlignment::Center,
			game.getFontManager());

		return std::make_unique<TextBox>(center, richText, game.getRenderer());
	}();

	this->backToMainMenuButton = []()
	{
		auto function = [](Game &game)
		{
			game.setPanel<MainMenuPanel>(game);
			game.setMusic(MusicName::PercIntro);
		};
		return Button<Game&>(function);
	}();

	this->generateButton = []()
	{
		const Int2 center((Renderer::ORIGINAL_WIDTH / 2) - 1, 120);
		auto function = [](Game &game)
		{
			// Eventually go to a "ChooseQuestionsPanel". What about the "pop-up" message?
		};
		return Button<Game&>(center, 175, 35, function);
	}();

	this->selectButton = []()
	{
		const Int2 center((Renderer::ORIGINAL_WIDTH / 2) - 1, 160);
		auto function = [](Game &game)
		{
			game.setPanel<ChooseClassPanel>(game);
		};
		return Button<Game&>(center, 175, 35, function);
	}();
}

std::pair<SDL_Texture*, CursorAlignment> ChooseClassCreationPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default), renderer);
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
}

void ChooseClassCreationPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);

	if (escapePressed)
	{
		this->backToMainMenuButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		if (this->generateButton.contains(mouseOriginalPoint))
		{
			this->generateButton.click(this->getGame());
		}
		else if (this->selectButton.contains(mouseOriginalPoint))
		{
			this->selectButton.click(this->getGame());
		}
	}
}

void ChooseClassCreationPanel::drawTooltip(const std::string &text, Renderer &renderer)
{
	const Texture tooltip(Panel::createTooltip(
		text, FontName::D, this->getGame().getFontManager(), renderer));

	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);
	const int mouseX = originalPosition.x;
	const int mouseY = originalPosition.y;
	const int x = ((mouseX + 8 + tooltip.getWidth()) < Renderer::ORIGINAL_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < Renderer::ORIGINAL_HEIGHT) ?
		(mouseY - 1) : (mouseY - tooltip.getHeight());

	renderer.drawOriginal(tooltip.get(), x, y);
}

void ChooseClassCreationPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Set palette.
	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw background.
	const auto &background = textureManager.getTexture(
		TextureFile::fromName(TextureName::CharacterCreation),
		PaletteFile::fromName(PaletteName::BuiltIn), renderer);
	renderer.drawOriginal(background.get());

	// Draw parchments: title, generate, select.
	const int parchmentX = (Renderer::ORIGINAL_WIDTH / 2) - 
		(this->parchment.getWidth() / 2) - 1;
	const int parchmentY = (Renderer::ORIGINAL_HEIGHT / 2) - 
		(this->parchment.getHeight() / 2) + 1;

	renderer.drawOriginal(this->parchment.get(), parchmentX, parchmentY - 20);
	renderer.drawOriginal(this->parchment.get(), parchmentX, parchmentY + 20);
	renderer.drawOriginal(this->parchment.get(), parchmentX, parchmentY + 60);

	// Draw text: title, generate, select.
	renderer.drawOriginal(this->titleTextBox->getTexture(),
		this->titleTextBox->getX(), this->titleTextBox->getY());
	renderer.drawOriginal(this->generateTextBox->getTexture(),
		this->generateTextBox->getX(), this->generateTextBox->getY());
	renderer.drawOriginal(this->selectTextBox->getTexture(),
		this->selectTextBox->getX(), this->selectTextBox->getY());

	// Check if the mouse is hovered over one of the boxes for tooltips.
	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPoint = renderer.nativeToOriginal(mousePosition);

	if (this->generateButton.contains(originalPoint))
	{
		this->drawTooltip("Answer questions\n(not implemented)", renderer);
	}
	else if (this->selectButton.contains(originalPoint))
	{
		this->drawTooltip("Choose from a list", renderer);
	}
}
