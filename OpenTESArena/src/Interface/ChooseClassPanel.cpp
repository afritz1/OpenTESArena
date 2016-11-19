#include <algorithm>
#include <cassert>

#include "SDL.h"

#include "ChooseClassPanel.h"

#include "Button.h"
#include "ChooseClassCreationPanel.h"
#include "ChooseNamePanel.h"
#include "ListBox.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/CharacterClassCategory.h"
#include "../Entities/CharacterClassCategoryName.h"
#include "../Entities/CharacterClassParser.h"
#include "../Game/Game.h"
#include "../Items/ArmorMaterial.h"
#include "../Items/MetalType.h"
#include "../Items/Shield.h"
#include "../Items/Weapon.h"
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

const int ChooseClassPanel::MAX_TOOLTIP_LINE_LENGTH = 14;

ChooseClassPanel::ChooseClassPanel(Game *game)
	: Panel(game)
{
	// Read in character classes.
	this->charClasses = CharacterClassParser::parse();
	assert(this->charClasses.size() > 0);

	// Sort character classes alphabetically for use with the list box.
	std::sort(this->charClasses.begin(), this->charClasses.end(),
		[](const std::unique_ptr<CharacterClass> &a, const std::unique_ptr<CharacterClass> &b)
	{
		return a->getDisplayName().compare(b->getDisplayName()) < 0;
	});

	this->titleTextBox = [game]()
	{
		int x = 89;
		int y = 32;
		Color color(211, 211, 211);
		std::string text = "Choose thy class...";
		auto &font = game->getFontManager().getFont(FontName::C);
		auto alignment = TextAlignment::Left;
		return std::unique_ptr<TextBox>(new TextBox(
			x,
			y,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->classesListBox = [this, game]()
	{
		int x = 85;
		int y = 46;
		auto &font = game->getFontManager().getFont(FontName::A);
		Color color(85, 44, 20);
		int maxDisplayed = 6;
		std::vector<std::string> elements;

		// This depends on the character classes being already sorted.
		for (const auto &item : this->charClasses)
		{
			elements.push_back(item->getDisplayName());
		}

		return std::unique_ptr<ListBox>(new ListBox(
			x,
			y,
			color,
			elements,
			font,
			maxDisplayed,
			game->getRenderer()));
	}();

	this->backToClassCreationButton = []()
	{
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> creationPanel(new ChooseClassCreationPanel(game));
			game->setPanel(std::move(creationPanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->upButton = [this]
	{
		Int2 center(68, 22);
		int w = 8;
		int h = 8;
		auto function = [this](Game *game)
		{
			// Scroll the list box up one if able.
			if (this->classesListBox->getScrollIndex() > 0)
			{
				this->classesListBox->scrollUp();
			}
		};
		return std::unique_ptr<Button>(new Button(center, w, h, function));
	}();

	this->downButton = [this]
	{
		Int2 center(68, 117);
		int w = 8;
		int h = 8;
		auto function = [this](Game *game)
		{
			// Scroll the list box down one if able.
			if (this->classesListBox->getScrollIndex() <
				(this->classesListBox->getElementCount() -
					this->classesListBox->getMaxDisplayedCount()))
			{
				this->classesListBox->scrollDown();
			}
		};
		return std::unique_ptr<Button>(new Button(center, w, h, function));
	}();

	this->acceptButton = [this]
	{
		auto function = [this](Game *game)
		{
			std::unique_ptr<Panel> namePanel(new ChooseNamePanel(
				game, *this->charClass.get()));
			game->setPanel(std::move(namePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	// Leave the tooltip textures empty for now. Let them be created on demand. 
	// Generating them all at once here is too slow in debug mode.
	assert(this->tooltipTextures.size() == 0);

	// Don't initialize the character class until one is clicked.
	assert(this->charClass.get() == nullptr);
}

ChooseClassPanel::~ChooseClassPanel()
{
	for (auto &pair : this->tooltipTextures)
	{
		SDL_DestroyTexture(pair.second);
	}
}

void ChooseClassPanel::handleEvent(const SDL_Event &e)
{
	// Eventually handle mouse motion: if mouse is over scroll bar and
	// LMB state is down, move scroll bar to that Y position.

	bool escapePressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_ESCAPE);

	if (escapePressed)
	{
		this->backToClassCreationButton->click(this->getGame());
	}

	bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_LEFT);
	bool mouseWheelUp = (e.type == SDL_MOUSEWHEEL) && (e.wheel.y > 0);
	bool mouseWheelDown = (e.type == SDL_MOUSEWHEEL) && (e.wheel.y < 0);

	const Int2 mousePosition = this->getMousePosition();
	const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
		.nativePointToOriginal(mousePosition);

	// See if a class in the list was clicked, or if it is being scrolled.
	if (this->classesListBox->contains(mouseOriginalPoint))
	{
		if (leftClick)
		{
			// Verify that the clicked index is valid. If so, use that character class.
			int index = this->classesListBox->getClickedIndex(mouseOriginalPoint);
			if ((index >= 0) && (index < this->classesListBox->getElementCount()))
			{
				this->charClass = std::unique_ptr<CharacterClass>(new CharacterClass(
					*this->charClasses.at(index).get()));
				this->acceptButton->click(this->getGame());
			}
		}
		else if (mouseWheelUp)
		{
			this->upButton->click(this->getGame());
		}
		else if (mouseWheelDown)
		{
			this->downButton->click(this->getGame());
		}
	}

	if (leftClick)
	{
		// Check scroll buttons (they are outside the list box to the left).
		if (this->upButton->contains(mouseOriginalPoint))
		{
			this->upButton->click(this->getGame());
		}
		else if (this->downButton->contains(mouseOriginalPoint))
		{
			this->downButton->click(this->getGame());
		}
	}	
}

void ChooseClassPanel::createTooltip(int tooltipIndex, Renderer &renderer)
{
	const auto &characterClass = *this->charClasses.at(tooltipIndex).get();

	std::string tooltipText = characterClass.getDisplayName() + " (" +
		CharacterClassCategory(characterClass.getClassCategoryName()).toString() + " class)\n" +
		"\n" + (characterClass.canCastMagic() ? "Can" : "Cannot") + " cast magic" + "\n" +
		"Health: " + std::to_string(characterClass.getStartingHealth()) +
		" + d" + std::to_string(characterClass.getHealthDice()) + "\n" +
		"Armors: " + this->getClassArmors(characterClass) + "\n" +
		"Shields: " + this->getClassShields(characterClass) + "\n" +
		"Weapons: " + this->getClassWeapons(characterClass);

	std::unique_ptr<TextBox> tooltipTextBox(new TextBox(
		0, 0,
		Color::White,
		tooltipText,
		this->getGame()->getFontManager().getFont(FontName::D),
		TextAlignment::Left,
		this->getGame()->getRenderer()));

	SDL_Surface *tooltipTextBoxSurface = tooltipTextBox->getSurface();

	const int padding = 3;

	Surface tooltipSurface(Surface::createSurfaceWithFormat(
		tooltipTextBoxSurface->w, tooltipTextBoxSurface->h + padding,
		Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT));
	SDL_FillRect(tooltipSurface.get(), nullptr,
		SDL_MapRGBA(tooltipSurface.get()->format, 32, 32, 32, 192));

	SDL_Rect rect;
	rect.x = 0;
	rect.y = 1;
	rect.w = tooltipTextBoxSurface->w;
	rect.h = tooltipTextBoxSurface->h;

	SDL_BlitSurface(tooltipTextBoxSurface, nullptr, tooltipSurface.get(), &rect);

	SDL_Texture *texture = renderer.createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_STATIC, tooltipSurface.get()->w, tooltipSurface.get()->h);
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(texture, nullptr, tooltipSurface.get()->pixels,
		tooltipSurface.get()->pitch);

	this->tooltipTextures.insert(std::make_pair(tooltipIndex, texture));
}

std::string ChooseClassPanel::getClassArmors(const CharacterClass &characterClass) const
{
	int lengthCounter = 0;
	const int armorCount = static_cast<int>(characterClass.getAllowedArmors().size());

	// Sort as they are listed in the CharacterClassParser.
	auto allowedArmors = characterClass.getAllowedArmors();
	std::sort(allowedArmors.begin(), allowedArmors.end());

	std::string armorString;

	// Decide what the armor string says.
	if (armorCount == 0)
	{
		armorString = "None";
	}
	else
	{
		// Collect all allowed armor display names for the class.
		for (int i = 0; i < armorCount; ++i)
		{
			const auto materialType = allowedArmors.at(i);
			auto materialString = ArmorMaterial::typeToString(materialType);
			lengthCounter += static_cast<int>(materialString.size());
			armorString.append(materialString);

			// If not the last element, add a comma.
			if (i < (armorCount - 1))
			{
				armorString.append(", ");

				// If too long, add a new line.
				if (lengthCounter > ChooseClassPanel::MAX_TOOLTIP_LINE_LENGTH)
				{
					lengthCounter = 0;
					armorString.append("\n   ");
				}
			}
		}
	}

	armorString.append(".");

	return armorString;
}

std::string ChooseClassPanel::getClassShields(const CharacterClass &characterClass) const
{
	int lengthCounter = 0;
	const int shieldCount = static_cast<int>(characterClass.getAllowedShields().size());

	// Sort as they are listed in the CharacterClassParser.
	auto allowedShields = characterClass.getAllowedShields();
	std::sort(allowedShields.begin(), allowedShields.end());

	std::string shieldsString;

	// Decide what the shield string says.
	if (shieldCount == 0)
	{
		shieldsString = "None";
	}
	else
	{
		// Collect all allowed shield display names for the class.
		for (int i = 0; i < shieldCount; ++i)
		{
			const auto shieldType = allowedShields.at(i);
			auto dummyMetal = MetalType::Iron;
			auto typeString = Shield(shieldType, dummyMetal).typeToString();
			lengthCounter += static_cast<int>(typeString.size());
			shieldsString.append(typeString);

			// If not the last element, add a comma.
			if (i < (shieldCount - 1))
			{
				shieldsString.append(", ");

				// If too long, add a new line.
				if (lengthCounter > ChooseClassPanel::MAX_TOOLTIP_LINE_LENGTH)
				{
					lengthCounter = 0;
					shieldsString.append("\n   ");
				}
			}
		}
	}

	shieldsString.append(".");

	return shieldsString;
}

std::string ChooseClassPanel::getClassWeapons(const CharacterClass &characterClass) const
{
	int lengthCounter = 0;
	const int weaponCount = static_cast<int>(characterClass.getAllowedWeapons().size());

	// Sort as they are listed in the CharacterClassParser.
	auto allowedWeapons = characterClass.getAllowedWeapons();
	std::sort(allowedWeapons.begin(), allowedWeapons.end());

	std::string weaponsString;

	// Decide what the weapon string says.
	if (weaponCount == 0)
	{
		// If the class is allowed zero weapons, it still doesn't exclude fists, I think.
		weaponsString = "None";
	}
	else
	{
		// Collect all allowed weapon display names for the class.
		for (int i = 0; i < weaponCount; ++i)
		{
			const auto weaponType = allowedWeapons.at(i);
			const auto dummyMetal = MetalType::Iron;
			auto typeString = Weapon(weaponType, dummyMetal).typeToString();
			lengthCounter += static_cast<int>(typeString.size());
			weaponsString.append(typeString);

			// If not the the last element, add a comma.
			if (i < (weaponCount - 1))
			{
				weaponsString.append(", ");

				// If too long, add a new line.
				if (lengthCounter > ChooseClassPanel::MAX_TOOLTIP_LINE_LENGTH)
				{
					lengthCounter = 0;
					weaponsString.append("\n   ");
				}
			}
		}
	}

	weaponsString.append(".");

	return weaponsString;
}

void ChooseClassPanel::drawClassTooltip(int tooltipIndex, Renderer &renderer)
{
	auto mouseOriginalPoint = this->getGame()->getRenderer()
		.nativePointToOriginal(this->getMousePosition());

	// Make the tooltip if it does not exist already.
	if (this->tooltipTextures.find(tooltipIndex) == this->tooltipTextures.end())
	{
		this->createTooltip(tooltipIndex, renderer);
	}

	SDL_Texture *tooltipTexture = this->tooltipTextures.at(tooltipIndex);
	int tooltipWidth, tooltipHeight;
	SDL_QueryTexture(tooltipTexture, nullptr, nullptr, &tooltipWidth, &tooltipHeight);

	// Calculate the top left corner, given the mouse position.
	const int mouseX = mouseOriginalPoint.getX();
	const int mouseY = mouseOriginalPoint.getY();
	const int x = ((mouseX + 8 + tooltipWidth) < Renderer::ORIGINAL_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltipWidth);
	const int y = ((mouseY + tooltipHeight) < Renderer::ORIGINAL_HEIGHT) ?
		(mouseY - 1) : (mouseY - tooltipHeight);

	renderer.drawToOriginal(tooltipTexture, x, y, tooltipWidth, tooltipHeight);
}

void ChooseClassPanel::render(Renderer &renderer)
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

	// Draw list pop-up.
	const auto &listPopUp = textureManager.getTexture(
		TextureFile::fromName(TextureName::PopUp2),
		TextureFile::fromName(TextureName::CharacterCreation));
	renderer.drawToOriginal(listPopUp.get(), 55, 9,
		listPopUp.getWidth(), listPopUp.getHeight());

	// Draw text: title, list.
	renderer.drawToOriginal(this->titleTextBox->getTexture(),
		this->titleTextBox->getX(), this->titleTextBox->getY());
	renderer.drawToOriginal(this->classesListBox->getTexture(),
		this->classesListBox->getPoint().getX(), 
		this->classesListBox->getPoint().getY());

	// Draw tooltip if over a valid element in the list box.
	auto mouseOriginalPoint = renderer.nativePointToOriginal(
		this->getMousePosition());

	if (this->classesListBox->contains(mouseOriginalPoint))
	{
		int index = this->classesListBox->getClickedIndex(mouseOriginalPoint);
		if ((index >= 0) && (index < this->classesListBox->getElementCount()))
		{
			this->drawClassTooltip(index, renderer);
		}
	}

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
