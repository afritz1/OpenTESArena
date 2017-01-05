#include <cassert>

#include "SDL.h"

#include "ChooseGenderPanel.h"

#include "Button.h"
#include "ChooseNamePanel.h"
#include "ChooseRacePanel.h"
#include "PopUp.h"
#include "PopUpType.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/CharacterGenderName.h"
#include "../Game/Game.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"
#include "../Rendering/Texture.h"

ChooseGenderPanel::ChooseGenderPanel(Game *game, const CharacterClass &charClass,
	const std::string &name)
	: Panel(game)
{
	this->parchment = std::unique_ptr<Texture>(new Texture(PopUp::create(
		PopUpType::Parchment, 180, 40, game->getTextureManager(),
		game->getRenderer())));

	this->genderTextBox = [game]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, 80);
		Color color(48, 12, 12);
		std::string text = "Choose thy gender...";
		auto &font = game->getFontManager().getFont(FontName::A);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->maleTextBox = [game]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, 120);
		Color color(48, 12, 12);
		std::string text = "Male";
		auto &font = game->getFontManager().getFont(FontName::A);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->femaleTextBox = [game]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, 160);
		Color color(48, 12, 12);
		std::string text = "Female";
		auto &font = game->getFontManager().getFont(FontName::A);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->backToNameButton = [charClass]()
	{
		auto function = [charClass](Game *game)
		{
			game->setPanel(std::unique_ptr<Panel>(new ChooseNamePanel(
				game, charClass)));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->maleButton = [charClass, name]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, 120);
		auto function = [charClass, name](Game *game)
		{
			std::unique_ptr<Panel> classPanel(new ChooseRacePanel(
				game, charClass, name, CharacterGenderName::Male));
			game->setPanel(std::move(classPanel));
		};
		return std::unique_ptr<Button>(new Button(center, 175, 35, function));
	}();

	this->femaleButton = [charClass, name]()
	{
		Int2 center(Renderer::ORIGINAL_WIDTH / 2, 160);
		auto function = [charClass, name](Game *game)
		{
			std::unique_ptr<Panel> classPanel(new ChooseRacePanel(
				game, charClass, name, CharacterGenderName::Female));
			game->setPanel(std::move(classPanel));
		};
		return std::unique_ptr<Button>(new Button(center, 175, 35, function));
	}();

	this->charClass = std::unique_ptr<CharacterClass>(new CharacterClass(charClass));
	this->name = name;
}

ChooseGenderPanel::~ChooseGenderPanel()
{

}

void ChooseGenderPanel::handleEvent(const SDL_Event &e)
{
	bool escapePressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_ESCAPE);

	if (escapePressed)
	{
		this->backToNameButton->click(this->getGame());
	}

	bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = this->getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
			.nativePointToOriginal(mousePosition);

		if (this->maleButton->contains(mouseOriginalPoint))
		{
			this->maleButton->click(this->getGame());
		}
		else if (this->femaleButton->contains(mouseOriginalPoint))
		{
			this->femaleButton->click(this->getGame());
		}
	}
}

void ChooseGenderPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGame()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw background.
	const auto &background = textureManager.getTexture(
		TextureFile::fromName(TextureName::CharacterCreation),
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawToOriginal(background.get());

	// Draw parchments: title, male, and female.
	const int parchmentX = (Renderer::ORIGINAL_WIDTH / 2) -
		(this->parchment->getWidth() / 2);
	const int parchmentY = (Renderer::ORIGINAL_HEIGHT / 2) -
		(this->parchment->getHeight() / 2);
	renderer.drawToOriginal(this->parchment->get(), parchmentX, parchmentY - 20);
	renderer.drawToOriginal(this->parchment->get(), parchmentX, parchmentY + 20);
	renderer.drawToOriginal(this->parchment->get(), parchmentX, parchmentY + 60);

	// Draw text: title, male, and female.
	renderer.drawToOriginal(this->genderTextBox->getTexture(),
		this->genderTextBox->getX(), this->genderTextBox->getY());
	renderer.drawToOriginal(this->maleTextBox->getTexture(),
		this->maleTextBox->getX(), this->maleTextBox->getY());
	renderer.drawToOriginal(this->femaleTextBox->getTexture(),
		this->femaleTextBox->getX(), this->femaleTextBox->getY());

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();

	// Draw cursor.
	const auto &cursor = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor));
	auto mousePosition = this->getMousePosition();
	renderer.drawToNative(cursor.get(),
		mousePosition.getX(), mousePosition.getY(),
		static_cast<int>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int>(cursor.getHeight() * this->getCursorScale()));
}
