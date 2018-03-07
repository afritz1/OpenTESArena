#include <cassert>

#include "SDL.h"

#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "LogbookPanel.h"
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
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"

LogbookPanel::LogbookPanel(Game &game)
	: Panel(game)
{
	this->titleTextBox = [&game]()
	{
		const Int2 center(
			Renderer::ORIGINAL_WIDTH / 2,
			Renderer::ORIGINAL_HEIGHT / 2);

		const auto &exeData = game.getMiscAssets().getExeData();
		const std::string &text = exeData.logbook.logbookIsEmpty;

		const RichTextString richText(
			text,
			FontName::A,
			Color(255, 207, 12),
			TextAlignment::Center,
			game.getFontManager());

		return std::make_unique<TextBox>(center, richText, game.getRenderer());
	}();

	this->backButton = []()
	{
		const Int2 center(
			Renderer::ORIGINAL_WIDTH - 40,
			Renderer::ORIGINAL_HEIGHT - 13);

		auto function = [](Game &game)
		{
			game.setPanel<GameWorldPanel>(game);
		};
		return Button<Game&>(center, 34, 14, function);
	}();
}

LogbookPanel::~LogbookPanel()
{

}

std::pair<SDL_Texture*, CursorAlignment> LogbookPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default), renderer);
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
}

void LogbookPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool lPressed = inputManager.keyPressed(e, SDLK_l);

	if (escapePressed || lPressed)
	{
		this->backButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		if (this->backButton.contains(mouseOriginalPoint))
		{
			this->backButton.click(this->getGame());
		}
	}
}

void LogbookPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Set palette.
	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw logbook background.
	const auto &logbookBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::Logbook),
		PaletteFile::fromName(PaletteName::BuiltIn), renderer);
	renderer.drawOriginal(logbookBackground.get());

	// Draw text: title.
	renderer.drawOriginal(this->titleTextBox->getTexture(),
		this->titleTextBox->getX(), this->titleTextBox->getY());
}
