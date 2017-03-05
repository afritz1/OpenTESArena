#include <cassert>

#include "SDL.h"

#include "ChooseRacePanel.h"

#include "Button.h"
#include "ChooseAttributesPanel.h"
#include "ChooseGenderPanel.h"
#include "PopUp.h"
#include "PopUpType.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Game/Game.h"
#include "../Math/Rect.h"
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
#include "../Rendering/Texture.h"
#include "../World/Province.h"
#include "../World/ProvinceName.h"

ChooseRacePanel::ChooseRacePanel(Game *game, const CharacterClass &charClass,
	const std::string &name, GenderName gender)
	: Panel(game), charClass(charClass), name(name), gender(gender)
{
	this->parchment = std::unique_ptr<Texture>(new Texture(PopUp::create(
		PopUpType::Parchment, 240, 60, game->getTextureManager(),
		game->getRenderer())));

	this->initialTextBox = [game, charClass, name]()
	{
		Int2 center((Renderer::ORIGINAL_WIDTH / 2) - 1, 100);
		Color color(48, 12, 12);
		std::string text = "From where dost thou hail,\n" +
			name + "\nthe\n" + charClass.getDisplayName() + "?";
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

	this->backToGenderButton = [charClass, name]()
	{
		auto function = [charClass, name](Game *game)
		{
			std::unique_ptr<Panel> namePanel(new ChooseGenderPanel(
				game, charClass, name));
			game->setPanel(std::move(namePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->acceptButton = [this, gender, charClass, name]()
	{
		auto function = [this, gender, charClass, name](Game *game)
		{
			std::unique_ptr<Panel> attributesPanel(new ChooseAttributesPanel(
				game, charClass, name, gender, *this->raceName.get()));
			game->setPanel(std::move(attributesPanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->raceName = nullptr;
	this->initialTextBoxVisible = true;
}

ChooseRacePanel::~ChooseRacePanel()
{
	
}

void ChooseRacePanel::handleEvent(const SDL_Event &e)
{
	bool escapePressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_ESCAPE);
	bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_LEFT);
	bool rightClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_RIGHT);

	// Interact with the pop-up text box if visible.
	// To do: find a better way to do this (via a stack of pop-ups?).
	if (this->initialTextBoxVisible)
	{
		bool enterPressed = (e.type == SDL_KEYDOWN) &&
			((e.key.keysym.sym == SDLK_RETURN) ||
			(e.key.keysym.sym == SDLK_KP_ENTER));
		bool spacePressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_SPACE);

		bool hideInitialPopUp = leftClick || rightClick || enterPressed ||
			spacePressed || escapePressed;

		if (hideInitialPopUp)
		{
			// Hide the initial text box.
			this->initialTextBoxVisible = false;
		}

		return;
	}

	// Interact with the map screen instead.
	if (escapePressed)
	{
		this->backToGenderButton->click(this->getGame());
	}
	else if (leftClick)
	{
		const Int2 mousePosition = this->getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
			.nativePointToOriginal(mousePosition);

		// Listen for map clicks.
		for (const auto provinceName : Province::getAllProvinceNames())
		{
			Province province(provinceName);
			const Rect &clickArea = province.getWorldMapClickArea();

			// Ignore the Imperial race because it is not implemented yet.
			if (clickArea.contains(mouseOriginalPoint) &&
				(provinceName != ProvinceName::ImperialProvince))
			{
				// Save the clicked province's race.
				this->raceName = std::unique_ptr<CharacterRaceName>(new CharacterRaceName(
					province.getRaceName()));

				// Go to the attributes panel.
				this->acceptButton->click(this->getGame());
				break;
			}
		}
	}	
}

void ChooseRacePanel::drawProvinceTooltip(ProvinceName provinceName, Renderer &renderer)
{
	const std::string raceName = Province(provinceName).getRaceDisplayName(true);
	const Font &font = this->getGame()->getFontManager().getFont(FontName::D);

	Texture tooltip(Panel::createTooltip("Land of the " + raceName, font, renderer));

	const Int2 mousePosition = this->getMousePosition();
	const Int2 originalPosition = renderer.nativePointToOriginal(mousePosition);
	const int mouseX = originalPosition.x;
	const int mouseY = originalPosition.y;
	const int x = ((mouseX + 8 + tooltip.getWidth()) < Renderer::ORIGINAL_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < Renderer::ORIGINAL_HEIGHT) ?
		mouseY : (mouseY - tooltip.getHeight());

	renderer.drawToOriginal(tooltip.get(), x, y);
}

void ChooseRacePanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGame()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw background map.
	const auto &raceSelectMap = textureManager.getTexture(
		TextureFile::fromName(TextureName::RaceSelect), 
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawToOriginal(raceSelectMap.get());

	// Arena just covers up the "exit" text at the bottom right.
	const auto &exitCover = textureManager.getTexture(
		TextureFile::fromName(TextureName::NoExit),
		TextureFile::fromName(TextureName::RaceSelect));
	renderer.drawToOriginal(exitCover.get(),
		Renderer::ORIGINAL_WIDTH - exitCover.getWidth(),
		Renderer::ORIGINAL_HEIGHT - exitCover.getHeight());

	// Draw visible parchments and text.
	if (this->initialTextBoxVisible)
	{
		renderer.drawToOriginal(this->parchment->get(),
			(Renderer::ORIGINAL_WIDTH / 2) - (this->parchment->getWidth() / 2) - 1,
			(Renderer::ORIGINAL_HEIGHT / 2) - (this->parchment->getHeight() / 2) - 1);
		renderer.drawToOriginal(this->initialTextBox->getTexture(),
			this->initialTextBox->getX(), this->initialTextBox->getY());
	}

	// Draw hovered province tooltip.
	if (!this->initialTextBoxVisible)
	{
		auto mouseOriginalPosition = this->getGame()->getRenderer()
			.nativePointToOriginal(this->getMousePosition());

		// Draw tooltip if the mouse is in a province.
		for (const auto provinceName : Province::getAllProvinceNames())
		{
			Province province(provinceName);
			const Rect &clickArea = province.getWorldMapClickArea();

			// Ignore the Imperial race for now as it is not implemented.
			if (clickArea.contains(mouseOriginalPosition) &&
				(provinceName != ProvinceName::ImperialProvince))
			{
				this->drawProvinceTooltip(provinceName, renderer);
			}
		}
	}

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();

	// Draw cursor.
	const auto &cursor = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor));
	auto mousePosition = this->getMousePosition();
	renderer.drawToNative(cursor.get(),
		mousePosition.x, mousePosition.y,
		static_cast<int>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int>(cursor.getHeight() * this->getCursorScale()));
}
