#include <cassert>
#include <unordered_map>

#include "SDL.h"

#include "ChooseClassPanel.h"
#include "ChooseGenderPanel.h"
#include "ChooseNamePanel.h"
#include "CursorAlignment.h"
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
#include "../Rendering/Surface.h"
#include "../Utilities/String.h"

const int ChooseNamePanel::MAX_NAME_LENGTH = 25;

ChooseNamePanel::ChooseNamePanel(Game &game, const CharacterClass &charClass)
	: Panel(game), charClass(charClass)
{
	this->parchment = Texture(Texture::generate(
		Texture::PatternType::Parchment, 300, 60, game.getTextureManager(),
		game.getRenderer()));

	this->titleTextBox = [&game, &charClass]()
	{
		const int x = 26;
		const int y = 82;

		const auto &exeData = game.getMiscAssets().getExeData();
		std::string text = exeData.charCreation.chooseName;
		text = String::replace(text, "%s", charClass.getName());

		const RichTextString richText(
			text,
			FontName::A,
			Color(48, 12, 12),
			TextAlignment::Left,
			game.getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			x, y, richText, game.getRenderer()));
	}();

	this->nameTextBox = [&game]()
	{
		const int x = 61;
		const int y = 101;

		const RichTextString richText(
			std::string(),
			FontName::A,
			Color(48, 12, 12),
			TextAlignment::Left,
			game.getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			x, y, richText, game.getRenderer()));
	}();

	this->backToClassButton = []()
	{
		auto function = [](Game &game)
		{
			game.setPanel<ChooseClassPanel>(game);
		};
		return Button<Game&>(function);
	}();

	this->acceptButton = []()
	{
		auto function = [](Game &game, const CharacterClass &charClass,
			const std::string &name)
		{
			game.setPanel<ChooseGenderPanel>(game, charClass, name);
		};
		return Button<Game&, const CharacterClass&, const std::string&>(function);
	}();
}

ChooseNamePanel::~ChooseNamePanel()
{

}

std::pair<SDL_Texture*, CursorAlignment> ChooseNamePanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default), renderer);
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
}

void ChooseNamePanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool enterPressed = inputManager.keyPressed(e, SDLK_RETURN) ||
		inputManager.keyPressed(e, SDLK_KP_ENTER);

	if (escapePressed)
	{
		this->backToClassButton.click(this->getGame());
	}

	// Only accept the name if it has a positive size.
	if (enterPressed && (this->name.size() > 0))
	{
		this->acceptButton.click(this->getGame(), this->charClass, this->name);
	}

	// --------------
	// To do: either use the input manager with the code below, or use SDL text input.
	// --------------

	// Upper and lower case English characters.
	const std::unordered_map<SDL_Keycode, std::pair<char, char>> letters =
	{
		{ SDLK_a, { 'A', 'a' } },
		{ SDLK_b, { 'B', 'b' } },
		{ SDLK_c, { 'C', 'c' } },
		{ SDLK_d, { 'D', 'd' } },
		{ SDLK_e, { 'E', 'e' } },
		{ SDLK_f, { 'F', 'f' } },
		{ SDLK_g, { 'G', 'g' } },
		{ SDLK_h, { 'H', 'h' } },
		{ SDLK_i, { 'I', 'i' } },
		{ SDLK_j, { 'J', 'j' } },
		{ SDLK_k, { 'K', 'k' } },
		{ SDLK_l, { 'L', 'l' } },
		{ SDLK_m, { 'M', 'm' } },
		{ SDLK_n, { 'N', 'n' } },
		{ SDLK_o, { 'O', 'o' } },
		{ SDLK_p, { 'P', 'p' } },
		{ SDLK_q, { 'Q', 'q' } },
		{ SDLK_r, { 'R', 'r' } },
		{ SDLK_s, { 'S', 's' } },
		{ SDLK_t, { 'T', 't' } },
		{ SDLK_u, { 'U', 'u' } },
		{ SDLK_v, { 'V', 'v' } },
		{ SDLK_w, { 'W', 'w' } },
		{ SDLK_x, { 'X', 'x' } },
		{ SDLK_y, { 'Y', 'y' } },
		{ SDLK_z, { 'Z', 'z' } }
	};

	// Punctuation (some duplicates exist to keep the shift behavior for quotes).
	const std::unordered_map<SDL_Keycode, std::pair<char, char>> punctuation =
	{
		{ SDLK_COMMA, { ',', ',' } },
		{ SDLK_MINUS, { '-', '-' } },
		{ SDLK_PERIOD, { '.', '.' } },
		{ SDLK_QUOTE, { '"', '\'' } }
	};

	if (e.type == SDL_KEYDOWN)
	{
		const uint8_t *keys = SDL_GetKeyboardState(nullptr);
		const SDL_Keycode keyCode = e.key.keysym.sym;
		bool shiftPressed = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];

		// See if the pressed key is a recognized letter.
		if (letters.find(keyCode) != letters.end())
		{
			// Add the letter to the name if there is room.
			if (this->name.size() < ChooseNamePanel::MAX_NAME_LENGTH)
			{
				const auto &pair = letters.at(keyCode);
				this->name.push_back(shiftPressed ? pair.first : pair.second);
			}
		}
		else if (punctuation.find(keyCode) != punctuation.end())
		{
			// The pressed key is recognized punctuation. Add it.
			if (this->name.size() < ChooseNamePanel::MAX_NAME_LENGTH)
			{
				const auto &pair = punctuation.at(keyCode);
				this->name.push_back(shiftPressed ? pair.first : pair.second);
			}
		}
		else if (keyCode == SDLK_SPACE)
		{
			// The pressed key is space. Add a space.
			if (this->name.size() < ChooseNamePanel::MAX_NAME_LENGTH)
			{
				this->name.push_back(' ');
			}
		}
		else if (keyCode == SDLK_BACKSPACE)
		{
			// The pressed key is backspace. Erase one letter if able.
			if (this->name.size() > 0)
			{
				this->name.pop_back();
			}
		}

		// Update the displayed name.
		this->nameTextBox = [this]
		{
			const int x = 61;
			const int y = 101;

			auto &game = this->getGame();
			const RichTextString &oldRichText = this->nameTextBox->getRichText();

			const RichTextString richText(
				this->name,
				oldRichText.getFontName(),
				oldRichText.getColor(),
				oldRichText.getAlignment(),
				game.getFontManager());

			return std::unique_ptr<TextBox>(new TextBox(
				x, y, richText, game.getRenderer()));
		}();
	}
}

void ChooseNamePanel::render(Renderer &renderer)
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

	// Draw parchment: title.
	renderer.drawOriginal(this->parchment.get(),
		(Renderer::ORIGINAL_WIDTH / 2) - (this->parchment.getWidth() / 2),
		(Renderer::ORIGINAL_HEIGHT / 2) - (this->parchment.getHeight() / 2));

	// Draw text: title, name.
	renderer.drawOriginal(this->titleTextBox->getTexture(),
		this->titleTextBox->getX(), this->titleTextBox->getY());
	renderer.drawOriginal(this->nameTextBox->getTexture(),
		this->nameTextBox->getX(), this->nameTextBox->getY());
}
