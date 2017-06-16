#include <cassert>

#include "SDL.h"

#include "ChooseRacePanel.h"

#include "ChooseAttributesPanel.h"
#include "ChooseGenderPanel.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Assets/ExeStrings.h"
#include "../Assets/TextAssets.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
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
#include "../Utilities/String.h"

namespace
{
	// Mouse click areas for the world map, ordered by how Arena originally
	// indexes them (read top left to bottom right on world map, center province 
	// is last).
	// - Eventually replace this with an index into the IMG file.
	const std::vector<Rect> ProvinceClickAreas =
	{
		Rect(52, 51, 44, 11),
		Rect(72, 75, 50, 11),
		Rect(142, 44, 34, 11),
		Rect(222, 84, 52, 11),
		Rect(37, 149, 49, 19),
		Rect(106, 147, 49, 10),
		Rect(148, 127, 37, 11),
		Rect(216, 144, 55, 12),
		Rect(133, 105, 83, 11)
	};
}

ChooseRacePanel::ChooseRacePanel(Game *game, const CharacterClass &charClass,
	const std::string &name, GenderName gender)
	: Panel(game), charClass(charClass), name(name), gender(gender)
{
	this->parchment = Texture(Texture::generate(
		Texture::PatternType::Parchment, 240, 60, game->getTextureManager(),
		game->getRenderer()));

	this->initialTextBox = [game, charClass, name]()
	{
		Int2 center((Renderer::ORIGINAL_WIDTH / 2) - 1, 100);
		Color color(48, 12, 12);

		std::string text = [game, charClass, name]()
		{
			std::string segment = game->getTextAssets().getAExeSegment(
				ExeStrings::ChooseRace);

			segment = String::replace(segment, '\r', '\n');

			// Replace first "%s" with player name.
			size_t index = segment.find("%s");
			segment.replace(index, 2, name);

			// Replace second "%s" with character class.
			index = segment.find("%s");
			segment.replace(index, 2, charClass.getDisplayName());

			return segment;
		}();

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

	this->backToGenderButton = []()
	{
		auto function = [](Game *game, const CharacterClass &charClass, 
			const std::string &name)
		{
			std::unique_ptr<Panel> namePanel(new ChooseGenderPanel(
				game, charClass, name));
			game->setPanel(std::move(namePanel));
		};
		return std::unique_ptr<Button<Game*, const CharacterClass&, const std::string&>>(
			new Button<Game*, const CharacterClass&, const std::string&>(function));
	}();

	this->acceptButton = []()
	{
		auto function = [](Game *game, const CharacterClass &charClass,
			const std::string &name, GenderName gender, int raceID)
		{
			std::unique_ptr<Panel> attributesPanel(new ChooseAttributesPanel(
				game, charClass, name, gender, raceID));
			game->setPanel(std::move(attributesPanel));
		};
		return std::unique_ptr<Button<Game*, const CharacterClass&, const std::string&, 
			GenderName, int>>(new Button<Game*, const CharacterClass&, const std::string&, 
				GenderName, int>(function));
	}();

	this->initialTextBoxVisible = true;
}

ChooseRacePanel::~ChooseRacePanel()
{
	
}

void ChooseRacePanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame()->getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	// Interact with the pop-up text box if visible.
	// To do: find a better way to do this (via a stack of pop-ups?).
	if (this->initialTextBoxVisible)
	{
		bool enterPressed = inputManager.keyPressed(e, SDLK_RETURN) ||
			inputManager.keyPressed(e, SDLK_KP_ENTER);
		bool spacePressed = inputManager.keyPressed(e, SDLK_SPACE);

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
		this->backToGenderButton->click(this->getGame(), this->charClass, this->name);
	}
	else if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
			.nativePointToOriginal(mousePosition);

		// Listen for map clicks.
		const int provinceCount = static_cast<int>(ProvinceClickAreas.size());
		for (int provinceID = 0; provinceID < provinceCount; provinceID++)
		{
			const Rect &clickArea = ProvinceClickAreas.at(provinceID);

			// Ignore the Imperial province.
			if (clickArea.contains(mouseOriginalPoint) && 
				(provinceID != (ProvinceClickAreas.size() - 1)))
			{
				// Go to the attributes panel.
				this->acceptButton->click(this->getGame(), this->charClass,
					this->name, this->gender, provinceID);
				break;
			}
		}
	}	
}

void ChooseRacePanel::drawProvinceTooltip(int provinceID, Renderer &renderer)
{
	// Get the race name associated with the province.
	assert(provinceID != (ProvinceClickAreas.size() - 1));
	const std::string &raceName = this->getGame()->getTextAssets().getAExeSegment(
		ExeStrings::RaceNamesPlural.at(provinceID));

	const Font &font = this->getGame()->getFontManager().getFont(FontName::D);

	Texture tooltip(Panel::createTooltip("Land of the " + raceName, font, renderer));

	const auto &inputManager = this->getGame()->getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
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
		renderer.drawToOriginal(this->parchment.get(),
			(Renderer::ORIGINAL_WIDTH / 2) - (this->parchment.getWidth() / 2) - 1,
			(Renderer::ORIGINAL_HEIGHT / 2) - (this->parchment.getHeight() / 2) - 1);
		renderer.drawToOriginal(this->initialTextBox->getTexture(),
			this->initialTextBox->getX(), this->initialTextBox->getY());
	}

	const auto &inputManager = this->getGame()->getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();

	// Draw hovered province tooltip.
	if (!this->initialTextBoxVisible)
	{
		const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
			.nativePointToOriginal(mousePosition);

		// Draw tooltip if the mouse is in a province.
		const int provinceCount = static_cast<int>(ProvinceClickAreas.size());
		for (int provinceID = 0; provinceID < provinceCount; provinceID++)
		{
			const Rect &clickArea = ProvinceClickAreas.at(provinceID);

			// Ignore the Imperial province.
			if (clickArea.contains(mouseOriginalPoint) &&
				(provinceID != (ProvinceClickAreas.size() - 1)))
			{
				this->drawProvinceTooltip(provinceID, renderer);
			}
		}
	}

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();

	// Draw cursor.
	const auto &cursor = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor));
	const auto &options = this->getGame()->getOptions();
	renderer.drawToNative(cursor.get(),
		mousePosition.x, mousePosition.y,
		static_cast<int>(cursor.getWidth() * options.getCursorScale()),
		static_cast<int>(cursor.getHeight() * options.getCursorScale()));
}
