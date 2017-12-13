#include <cassert>

#include "SDL.h"

#include "ChooseGenderPanel.h"
#include "ChooseNamePanel.h"
#include "ChooseRacePanel.h"
#include "CursorAlignment.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Assets/ExeStrings.h"
#include "../Assets/MiscAssets.h"
#include "../Entities/GenderName.h"
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

ChooseGenderPanel::ChooseGenderPanel(Game &game, const CharacterClass &charClass,
	const std::string &name)
	: Panel(game), charClass(charClass), name(name)
{
	this->parchment = Texture(Texture::generate(
		Texture::PatternType::Parchment, 180, 40, game.getTextureManager(),
		game.getRenderer()));

	this->genderTextBox = [&game]()
	{
		const Int2 center(Renderer::ORIGINAL_WIDTH / 2, 80);

		const RichTextString richText(
			game.getMiscAssets().getAExeStrings().get(ExeStringKey::ChooseGender),
			FontName::A,
			Color(48, 12, 12),
			TextAlignment::Center,
			game.getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			center, richText, game.getRenderer()));
	}();

	this->maleTextBox = [&game]()
	{
		const Int2 center(Renderer::ORIGINAL_WIDTH / 2, 120);

		const RichTextString richText(
			game.getMiscAssets().getAExeStrings().get(ExeStringKey::ChooseGenderMale),
			FontName::A,
			Color(48, 12, 12),
			TextAlignment::Center,
			game.getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			center, richText, game.getRenderer()));
	}();

	this->femaleTextBox = [&game]()
	{
		const Int2 center(Renderer::ORIGINAL_WIDTH / 2, 160);

		const RichTextString richText(
			game.getMiscAssets().getAExeStrings().get(ExeStringKey::ChooseGenderFemale),
			FontName::A,
			Color(48, 12, 12),
			TextAlignment::Center,
			game.getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			center, richText, game.getRenderer()));
	}();

	this->backToNameButton = []()
	{
		auto function = [](Game &game, const CharacterClass &charClass)
		{
			game.setPanel<ChooseNamePanel>(game, charClass);
		};
		return Button<Game&, const CharacterClass&>(function);
	}();

	this->maleButton = []()
	{
		const Int2 center(Renderer::ORIGINAL_WIDTH / 2, 120);
		auto function = [](Game &game, const CharacterClass &charClass,
			const std::string &name)
		{
			game.setPanel<ChooseRacePanel>(game, charClass, name, GenderName::Male);
		};
		return Button<Game&, const CharacterClass&, 
			const std::string&>(center, 175, 35, function);
	}();

	this->femaleButton = []()
	{
		const Int2 center(Renderer::ORIGINAL_WIDTH / 2, 160);
		auto function = [](Game &game, const CharacterClass &charClass,
			const std::string &name)
		{
			game.setPanel<ChooseRacePanel>(game, charClass, name, GenderName::Female);
		};
		return Button<Game&, const CharacterClass&, 
			const std::string&>(center, 175, 35, function);
	}();
}

ChooseGenderPanel::~ChooseGenderPanel()
{

}

std::pair<SDL_Texture*, CursorAlignment> ChooseGenderPanel::getCurrentCursor() const
{
	auto &textureManager = this->getGame().getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default));
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
}

void ChooseGenderPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);

	if (escapePressed)
	{
		this->backToNameButton.click(this->getGame(), this->charClass);
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		if (this->maleButton.contains(mouseOriginalPoint))
		{
			this->maleButton.click(this->getGame(), this->charClass, this->name);
		}
		else if (this->femaleButton.contains(mouseOriginalPoint))
		{
			this->femaleButton.click(this->getGame(), this->charClass, this->name);
		}
	}
}

void ChooseGenderPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Set palette.
	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw background.
	const auto &background = textureManager.getTexture(
		TextureFile::fromName(TextureName::CharacterCreation),
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawOriginal(background.get());

	// Draw parchments: title, male, and female.
	const int parchmentX = (Renderer::ORIGINAL_WIDTH / 2) -
		(this->parchment.getWidth() / 2);
	const int parchmentY = (Renderer::ORIGINAL_HEIGHT / 2) -
		(this->parchment.getHeight() / 2);
	renderer.drawOriginal(this->parchment.get(), parchmentX, parchmentY - 20);
	renderer.drawOriginal(this->parchment.get(), parchmentX, parchmentY + 20);
	renderer.drawOriginal(this->parchment.get(), parchmentX, parchmentY + 60);

	// Draw text: title, male, and female.
	renderer.drawOriginal(this->genderTextBox->getTexture(),
		this->genderTextBox->getX(), this->genderTextBox->getY());
	renderer.drawOriginal(this->maleTextBox->getTexture(),
		this->maleTextBox->getX(), this->maleTextBox->getY());
	renderer.drawOriginal(this->femaleTextBox->getTexture(),
		this->femaleTextBox->getX(), this->femaleTextBox->getY());
}
