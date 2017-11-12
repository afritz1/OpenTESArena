#include <cassert>

#include "SDL.h"

#include "ChooseAttributesPanel.h"
#include "ChooseGenderPanel.h"
#include "ChooseRacePanel.h"
#include "CursorAlignment.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextSubPanel.h"
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

ChooseRacePanel::ChooseRacePanel(Game &game, const CharacterClass &charClass,
	const std::string &name, GenderName gender)
	: Panel(game), charClass(charClass), name(name), gender(gender)
{
	this->backToGenderButton = []()
	{
		auto function = [](Game &game, const CharacterClass &charClass, 
			const std::string &name)
		{
			std::unique_ptr<Panel> namePanel(new ChooseGenderPanel(
				game, charClass, name));
			game.setPanel(std::move(namePanel));
		};
		return std::unique_ptr<Button<Game&, const CharacterClass&, const std::string&>>(
			new Button<Game&, const CharacterClass&, const std::string&>(function));
	}();

	this->acceptButton = []()
	{
		auto function = [](Game &game, const CharacterClass &charClass,
			const std::string &name, GenderName gender, int raceID)
		{
			std::unique_ptr<Panel> attributesPanel(new ChooseAttributesPanel(
				game, charClass, name, gender, raceID));
			game.setPanel(std::move(attributesPanel));
		};
		return std::unique_ptr<Button<Game&, const CharacterClass&, const std::string&, 
			GenderName, int>>(new Button<Game&, const CharacterClass&, const std::string&, 
				GenderName, int>(function));
	}();
	
	// @todo: maybe allocate std::unique_ptr<std::function> for unravelling the map?
	// When done, set to null and push initial parchment sub-panel?

	// Push the initial text sub-panel.
	std::unique_ptr<Panel> textSubPanel = [&game, &charClass, &name]()
	{
		const Int2 center((Renderer::ORIGINAL_WIDTH / 2) - 1, 98);
		const Color color(48, 12, 12);

		const std::string text = [&game, &charClass, &name]()
		{
			std::string segment = game.getTextAssets().getAExeStrings().get(
				ExeStringKey::ChooseRace);
			segment = String::replace(segment, '\r', '\n');

			// Replace first "%s" with player name.
			size_t index = segment.find("%s");
			segment.replace(index, 2, name);

			// Replace second "%s" with character class.
			index = segment.find("%s");
			segment.replace(index, 2, charClass.getName());

			return segment;
		}();

		const int lineSpacing = 1;

		const RichTextString richText(
			text,
			FontName::A,
			color,
			TextAlignment::Center,
			lineSpacing,
			game.getFontManager());

		Texture texture(Texture::generate(
			Texture::PatternType::Parchment, 240, 60, game.getTextureManager(),
			game.getRenderer()));

		const Int2 textureCenter(
			(Renderer::ORIGINAL_WIDTH / 2) - 1,
			(Renderer::ORIGINAL_HEIGHT / 2) - 1);

		// The sub-panel does nothing after it's removed.
		auto function = [](Game &game) {};
		
		return std::unique_ptr<Panel>(new TextSubPanel(
			game, center, richText, function, std::move(texture), textureCenter));
	}();

	game.pushSubPanel(std::move(textSubPanel));
}

ChooseRacePanel::~ChooseRacePanel()
{
	
}

std::pair<SDL_Texture*, CursorAlignment> ChooseRacePanel::getCurrentCursor() const
{
	auto &textureManager = this->getGame().getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default));
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
}

void ChooseRacePanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	// Interact with the map screen.
	if (escapePressed)
	{
		this->backToGenderButton->click(this->getGame(), this->charClass, this->name);
	}
	else if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame().getRenderer()
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
	const std::string &raceName = this->getGame().getTextAssets().getAExeStrings().getList(
		ExeStringKey::RaceNamesPlural).at(provinceID);
	
	const Texture tooltip(Panel::createTooltip(
		"Land of the " + raceName, FontName::D, this->getGame().getFontManager(), renderer));

	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativePointToOriginal(mousePosition);
	const int mouseX = originalPosition.x;
	const int mouseY = originalPosition.y;
	const int x = ((mouseX + 8 + tooltip.getWidth()) < Renderer::ORIGINAL_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < Renderer::ORIGINAL_HEIGHT) ?
		mouseY : (mouseY - tooltip.getHeight());

	renderer.drawOriginal(tooltip.get(), x, y);
}

void ChooseRacePanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Set palette.
	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw background map.
	const auto &raceSelectMap = textureManager.getTexture(
		TextureFile::fromName(TextureName::RaceSelect), 
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawOriginal(raceSelectMap.get());

	// Arena just covers up the "exit" text at the bottom right.
	const auto &exitCover = textureManager.getTexture(
		TextureFile::fromName(TextureName::NoExit),
		TextureFile::fromName(TextureName::RaceSelect));
	renderer.drawOriginal(exitCover.get(),
		Renderer::ORIGINAL_WIDTH - exitCover.getWidth(),
		Renderer::ORIGINAL_HEIGHT - exitCover.getHeight());

	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();

	// Draw hovered province tooltip.
	const Int2 mouseOriginalPoint = this->getGame().getRenderer()
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
