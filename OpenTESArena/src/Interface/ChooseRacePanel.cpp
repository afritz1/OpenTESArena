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
#include "../Assets/MiscAssets.h"
#include "../Assets/WorldMapMask.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Rect.h"
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

const int ChooseRacePanel::NO_ID = -1;

ChooseRacePanel::ChooseRacePanel(Game &game, const CharacterClass &charClass,
	const std::string &name, GenderName gender)
	: Panel(game), charClass(charClass), name(name), gender(gender)
{
	this->backToGenderButton = []()
	{
		auto function = [](Game &game, const CharacterClass &charClass,
			const std::string &name)
		{
			game.setPanel<ChooseGenderPanel>(game, charClass, name);
		};
		return Button<Game&, const CharacterClass&, const std::string&>(function);
	}();

	this->acceptButton = []()
	{
		auto function = [](Game &game, const CharacterClass &charClass,
			const std::string &name, GenderName gender, int raceID)
		{
			game.setPanel<ChooseAttributesPanel>(game, charClass, name, gender, raceID);
		};
		return Button<Game&, const CharacterClass&,
			const std::string&, GenderName, int>(function);
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
			std::string segment = game.getMiscAssets().getAExeStrings().get(
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

int ChooseRacePanel::getProvinceMaskID(const Int2 &position) const
{
	const auto &worldMapMasks = this->getGame().getMiscAssets().getWorldMapMasks();
	const int maskCount = static_cast<int>(worldMapMasks.size());
	for (int maskID = 0; maskID < maskCount; maskID++)
	{
		// Ignore the center province and the "Exit" button.
		const int lastProvinceID = 8;
		const int exitButtonID = 9;
		if ((maskID == lastProvinceID) || (maskID == exitButtonID))
		{
			continue;
		}

		const WorldMapMask &mapMask = worldMapMasks.at(maskID);
		const Rect &maskRect = mapMask.getRect();

		if (maskRect.contains(position))
		{
			// See if the pixel is set in the bitmask.
			const bool success = mapMask.get(position.x, position.y);

			if (success)
			{
				// Return the mask's ID.
				return maskID;
			}
		}
	}

	// No province mask found at the given location.
	return ChooseRacePanel::NO_ID;
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
		this->backToGenderButton.click(this->getGame(), this->charClass, this->name);
	}
	else if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		// Listen for clicks on the map, checking if the mouse is over a province mask.
		const int maskID = this->getProvinceMaskID(originalPoint);
		if (maskID != ChooseRacePanel::NO_ID)
		{
			// Choose the selected province.
			this->acceptButton.click(this->getGame(), this->charClass,
				this->name, this->gender, maskID);
		}
	}
}

void ChooseRacePanel::drawProvinceTooltip(int provinceID, Renderer &renderer)
{
	// Get the race name associated with the province.
	const std::string &raceName = this->getGame().getMiscAssets().getAExeStrings().getList(
		ExeStringKey::RaceNamesPlural).at(provinceID);

	const Texture tooltip(Panel::createTooltip(
		"Land of the " + raceName, FontName::D, this->getGame().getFontManager(), renderer));

	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);
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
	const Int2 originalPoint = this->getGame().getRenderer()
		.nativeToOriginal(mousePosition);

	// Draw tooltip if the mouse is in a province.
	const int maskID = this->getProvinceMaskID(originalPoint);
	if (maskID != ChooseRacePanel::NO_ID)
	{
		this->drawProvinceTooltip(maskID, renderer);
	}
}
