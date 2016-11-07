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
#include "../Game/GameState.h"
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

const int ChooseClassPanel::MAX_TOOLTIP_LINE_LENGTH = 14;

ChooseClassPanel::ChooseClassPanel(GameState *gameState)
	: Panel(gameState)
{
	this->charClasses = CharacterClassParser::parse();
	assert(this->charClasses.size() > 0);

	// Sort character classes alphabetically for use with the list box.
	std::sort(this->charClasses.begin(), this->charClasses.end(),
		[](const std::unique_ptr<CharacterClass> &a, const std::unique_ptr<CharacterClass> &b)
	{
		return a->getDisplayName().compare(b->getDisplayName()) < 0;
	});

	this->titleTextBox = [gameState]()
	{
		int x = 89;
		int y = 32;
		Color color(211, 211, 211);
		std::string text = "Choose thy class...";
		auto &font = gameState->getFontManager().getFont(FontName::C);
		auto alignment = TextAlignment::Left;
		return std::unique_ptr<TextBox>(new TextBox(
			x,
			y,
			color,
			text,
			font,
			alignment,
			gameState->getRenderer()));
	}();

	this->classesListBox = [this, gameState]()
	{
		int x = 85;
		int y = 46;
		auto &font = gameState->getFontManager().getFont(FontName::A);
		Color color(85, 44, 20);
		int maxElements = 6;
		std::vector<std::string> elements;

		// This depends on the character classes being already sorted.
		for (const auto &item : this->charClasses)
		{
			elements.push_back(item->getDisplayName());
		}

		return std::unique_ptr<ListBox>(new ListBox(
			x,
			y,
			font,
			color,
			maxElements,
			elements,
			gameState->getRenderer()));
	}();

	this->backToClassCreationButton = []()
	{
		auto function = [](GameState *gameState)
		{
			std::unique_ptr<Panel> creationPanel(new ChooseClassCreationPanel(gameState));
			gameState->setPanel(std::move(creationPanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->upButton = [this]
	{
		Int2 center(68, 22);
		int w = 8;
		int h = 8;
		auto function = [this](GameState *gameState)
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
		auto function = [this](GameState *gameState)
		{
			// Scroll the list box down one if able.
			if (this->classesListBox->getScrollIndex() <
				(this->classesListBox->getElementCount() -
					this->classesListBox->maxDisplayedElements()))
			{
				this->classesListBox->scrollDown();
			}
		};
		return std::unique_ptr<Button>(new Button(center, w, h, function));
	}();

	this->acceptButton = [this]
	{
		auto function = [this](GameState *gameState)
		{
			std::unique_ptr<Panel> namePanel(new ChooseNamePanel(
				gameState, *this->charClass.get()));
			gameState->setPanel(std::move(namePanel));
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

void ChooseClassPanel::handleEvents(bool &running)
{
	auto mousePosition = this->getMousePosition();
	auto mouseOriginalPoint = this->getGameState()->getRenderer()
		.nativePointToOriginal(mousePosition);

	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		bool applicationExit = (e.type == SDL_QUIT);
		bool resized = (e.type == SDL_WINDOWEVENT) &&
			(e.window.event == SDL_WINDOWEVENT_RESIZED);
		bool escapePressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_ESCAPE);

		if (applicationExit)
		{
			running = false;
		}
		if (resized)
		{
			int width = e.window.data1;
			int height = e.window.data2;
			this->getGameState()->resizeWindow(width, height);
		}
		if (escapePressed)
		{
			this->backToClassCreationButton->click(this->getGameState());
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);

		bool mouseWheelUp = (e.type == SDL_MOUSEWHEEL) && (e.wheel.y > 0);
		bool mouseWheelDown = (e.type == SDL_MOUSEWHEEL) && (e.wheel.y < 0);

		bool scrollUpClick = leftClick && this->upButton->containsPoint(mouseOriginalPoint);
		bool scrollDownClick = leftClick && this->downButton->containsPoint(mouseOriginalPoint);

		if (this->classesListBox->containsPoint(mouseOriginalPoint))
		{
			if (leftClick)
			{
				// Verify that the clicked index is valid. If so, use that character class.
				int index = this->classesListBox->getClickedIndex(mouseOriginalPoint);
				if ((index >= 0) && (index < this->classesListBox->getElementCount()))
				{
					this->charClass = std::unique_ptr<CharacterClass>(new CharacterClass(
						*this->charClasses.at(index).get()));
					this->acceptButton->click(this->getGameState());
				}
			}
			else if (mouseWheelUp)
			{
				this->upButton->click(this->getGameState());
			}
			else if (mouseWheelDown)
			{
				this->downButton->click(this->getGameState());
			}
		}

		// Check scroll buttons (they are outside the list box).
		if (scrollUpClick)
		{
			this->upButton->click(this->getGameState());
		}
		else if (scrollDownClick)
		{
			this->downButton->click(this->getGameState());
		}
	}
}

void ChooseClassPanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void ChooseClassPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void ChooseClassPanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
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
		this->getGameState()->getFontManager().getFont(FontName::D),
		TextAlignment::Left,
		this->getGameState()->getRenderer()));

	int tooltipTextBoxWidth, tooltipTextBoxHeight;
	SDL_QueryTexture(tooltipTextBox->getTexture(), nullptr, nullptr,
		&tooltipTextBoxWidth, &tooltipTextBoxHeight);

	const int padding = 3;

	Surface tooltip(tooltipTextBoxWidth, tooltipTextBoxHeight + padding);
	tooltip.fill(Color(32, 32, 32));

	SDL_Surface *tooltipTextBoxSurface = tooltipTextBox->getSurface();

	SDL_Rect rect;
	rect.x = 0;
	rect.y = 1;
	rect.w = tooltipTextBoxWidth;
	rect.h = tooltipTextBoxHeight;

	SDL_BlitSurface(tooltipTextBoxSurface, nullptr, tooltip.getSurface(), &rect);

	SDL_Texture *texture = renderer.createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_STATIC, tooltip.getWidth(), tooltip.getHeight());
	SDL_UpdateTexture(texture, nullptr, tooltip.getSurface()->pixels,
		tooltip.getWidth() * (Renderer::DEFAULT_BPP / 8));

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
	auto mouseOriginalPoint = this->getGameState()->getRenderer()
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
	auto &textureManager = this->getGameState()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw background.
	auto *background = textureManager.getTexture(
		TextureFile::fromName(TextureName::CharacterCreation),
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawToOriginal(background);

	// Draw list pop-up.
	auto *listPopUp = textureManager.getTexture(
		TextureFile::fromName(TextureName::PopUp2), 
		TextureFile::fromName(TextureName::CharacterCreation));

	int listWidth, listHeight;
	SDL_QueryTexture(listPopUp, nullptr, nullptr, &listWidth, &listHeight);
	renderer.drawToOriginal(listPopUp,
		55,
		9,
		listWidth,
		listHeight);

	// Draw text: title, list.
	renderer.drawToOriginal(this->titleTextBox->getTexture(),
		this->titleTextBox->getX(), this->titleTextBox->getY());
	renderer.drawToOriginal(this->classesListBox->getSurface(),
		this->classesListBox->getX(), this->classesListBox->getY());

	// Draw tooltip if over a valid element in the list box.
	auto mouseOriginalPoint = this->getGameState()->getRenderer()
		.nativePointToOriginal(this->getMousePosition());

	if (this->classesListBox->containsPoint(mouseOriginalPoint))
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
	auto *cursor = textureManager.getSurface(
		TextureFile::fromName(TextureName::SwordCursor));
	auto mousePosition = this->getMousePosition();
	renderer.drawToNative(cursor,
		mousePosition.getX(), mousePosition.getY(),
		static_cast<int>(cursor->w * this->getCursorScale()),
		static_cast<int>(cursor->h * this->getCursorScale()));
}
