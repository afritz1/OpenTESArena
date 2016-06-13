#include <algorithm>
#include <cassert>

#include "SDL.h"

#include "ChooseClassPanel.h"

#include "Button.h"
#include "ChooseClassCreationPanel.h"
#include "ChooseNamePanel.h"
#include "ListBox.h"
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
#include "../Math/Constants.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

const int ChooseClassPanel::MAX_TOOLTIP_LINE_LENGTH = 14;

ChooseClassPanel::ChooseClassPanel(GameState *gameState)
	: Panel(gameState)
{
	this->parchment = nullptr;
	this->upDownSurface = nullptr;
	this->titleTextBox = nullptr;
	this->classesListBox = nullptr;
	this->backToClassCreationButton = nullptr;
	this->upButton = nullptr;
	this->downButton = nullptr;
	this->acceptButton = nullptr;
	this->charClasses = CharacterClassParser::parse();
	this->charClass = nullptr;

	// Sort character classes alphabetically.
	std::sort(this->charClasses.begin(), this->charClasses.end(),
		[](const std::unique_ptr<CharacterClass> &a, const std::unique_ptr<CharacterClass> &b)
	{
		return a->getDisplayName().compare(b->getDisplayName()) < 0;
	});

	this->parchment = [gameState]()
	{
		auto *surface = gameState->getTextureManager().getSurface(
			TextureFile::fromName(TextureName::ParchmentPopup)).getSurface();
		return std::unique_ptr<Surface>(new Surface(surface));
	}();

	this->upDownSurface = [gameState]()
	{
		int x = (ORIGINAL_WIDTH / 2) - 62;
		int y = (ORIGINAL_HEIGHT / 2);
		auto *surface = gameState->getTextureManager().getSurface(
			TextureFile::fromName(TextureName::UpDown)).getSurface();
		return std::unique_ptr<Surface>(new Surface(x, y, surface));
	}();

	this->titleTextBox = [gameState]()
	{
		auto center = Int2(160, 56);
		auto color = Color(48, 12, 12);
		std::string text = "Choose thy class...";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->classesListBox = [this, gameState]()
	{
		// Intended to be left aligned against something like a scroll bar.
		int x = (ORIGINAL_WIDTH / 2) - 50;
		int y = (ORIGINAL_HEIGHT / 2);
		auto fontName = FontName::A;
		auto color = Color(48, 12, 12);
		int maxElements = 6;
		auto elements = std::vector<std::string>();
		for (const auto &item : this->charClasses)
		{
			elements.push_back(item->getDisplayName());
		}
		return std::unique_ptr<ListBox>(new ListBox(
			x,
			y,
			fontName,
			color,
			maxElements,
			elements,
			gameState->getTextureManager()));
	}();

	this->backToClassCreationButton = [gameState]()
	{
		auto function = [gameState]()
		{
			gameState->setPanel(std::unique_ptr<Panel>(
				new ChooseClassCreationPanel(gameState)));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	this->upButton = [this]
	{
		int x = (ORIGINAL_WIDTH / 2) - 62;
		int y = (ORIGINAL_HEIGHT / 2);
		int w = 8;
		int h = 8;
		auto function = [this]
		{
			// Scroll the list box up one if able.
			if (this->classesListBox->getScrollIndex() > 0)
			{
				this->classesListBox->scrollUp();
			}
		};
		return std::unique_ptr<Button>(new Button(x, y, w, h, function));
	}();

	this->downButton = [this]
	{
		int x = (ORIGINAL_WIDTH / 2) - 62;
		int y = (ORIGINAL_HEIGHT / 2) + 8;
		int w = 8;
		int h = 8;
		auto function = [this]
		{
			// Scroll the list box down one if able.
			if (this->classesListBox->getScrollIndex() <
				(this->classesListBox->getElementCount() -
					this->classesListBox->maxDisplayedElements()))
			{
				this->classesListBox->scrollDown();
			}
		};
		return std::unique_ptr<Button>(new Button(x, y, w, h, function));
	}();

	this->acceptButton = [this, gameState]
	{
		auto function = [this, gameState]
		{
			auto namePanel = std::unique_ptr<Panel>(new ChooseNamePanel(
				gameState, *this->charClass.get()));
			gameState->setPanel(std::move(namePanel));
		};
		return std::unique_ptr<Button>(new Button(function));
	}();

	assert(this->parchment.get() != nullptr);
	assert(this->upDownSurface.get() != nullptr);
	assert(this->titleTextBox.get() != nullptr);
	assert(this->classesListBox.get() != nullptr);
	assert(this->backToClassCreationButton.get() != nullptr);
	assert(this->acceptButton.get() != nullptr);
	assert(this->upButton.get() != nullptr);
	assert(this->downButton.get() != nullptr);
	assert(this->charClasses.size() > 0);
	assert(this->charClass.get() == nullptr);
}

ChooseClassPanel::~ChooseClassPanel()
{

}

void ChooseClassPanel::handleEvents(bool &running)
{
	auto mousePosition = this->getMousePosition();
	auto mouseOriginalPoint = this->nativePointToOriginal(mousePosition);

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
			this->backToClassCreationButton->click();
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
					this->acceptButton->click();
				}
			}
			else if (mouseWheelUp)
			{
				this->upButton->click();
			}
			else if (mouseWheelDown)
			{
				this->downButton->click();
			}
		}

		// Check scroll buttons (they are outside the list box).
		if (scrollUpClick)
		{
			this->upButton->click();
		}
		else if (scrollDownClick)
		{
			this->downButton->click();
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

std::string ChooseClassPanel::getClassArmors(const CharacterClass &characterClass) const
{
	auto armorString = std::string();

	int lengthCounter = 0;
	const int armorCount = static_cast<int>(characterClass.getAllowedArmors().size());

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
			const auto &materialType = characterClass.getAllowedArmors().at(i);
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
	auto shieldsString = std::string();

	int lengthCounter = 0;
	const int shieldCount = static_cast<int>(characterClass.getAllowedShields().size());

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
			const auto &shieldType = characterClass.getAllowedShields().at(i);
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
	auto weaponsString = std::string();

	int lengthCounter = 0;
	const int weaponCount = static_cast<int>(characterClass.getAllowedWeapons().size());

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
			const auto &weaponType = characterClass.getAllowedWeapons().at(i);
			auto dummyMetal = MetalType::Iron;
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

void ChooseClassPanel::drawClassTooltip(const CharacterClass &characterClass, SDL_Surface *dst)
{
	auto mouseOriginalPoint = this->nativePointToOriginal(this->getMousePosition());

	auto tooltipText = characterClass.getDisplayName() + "\n\n" +
		CharacterClassCategory(characterClass.getClassCategoryName()).toString() + " class" +
		"\n" + (characterClass.canCastMagic() ? "Can" : "Cannot") + " cast magic" + "\n" +
		"Health: " + std::to_string(characterClass.getStartingHealth()) +
		" + d" + std::to_string(characterClass.getHealthDice()) + "\n" +
		"Armors: " + this->getClassArmors(characterClass) + "\n" +
		"Shields: " + this->getClassShields(characterClass) + "\n" +
		"Weapons: " + this->getClassWeapons(characterClass);
	auto tooltip = std::unique_ptr<TextBox>(new TextBox(
		mouseOriginalPoint.getX(),
		mouseOriginalPoint.getY(),
		Color::White,
		tooltipText,
		FontName::A,
		this->getGameState()->getTextureManager()));
	auto tooltipBackground = Surface(tooltip->getX(), tooltip->getY(),
		tooltip->getWidth(), tooltip->getHeight());
	tooltipBackground.fill(Color(32, 32, 32));

	// Make the tooltip smaller.
	const int width = tooltip->getWidth() / 2;
	const int height = tooltip->getHeight() / 2;

	// I tried clamping to the edge, but then it hides the cursor, which isn't good.
	const int x = ((tooltip->getX() + width) < ORIGINAL_WIDTH) ? tooltip->getX() :
		(tooltip->getX() - width);//(ORIGINAL_WIDTH - width);
	const int y = ((tooltip->getY() + height) < ORIGINAL_HEIGHT) ? tooltip->getY() :
		(tooltip->getY() - height);//(ORIGINAL_HEIGHT - height);

	this->drawScaledToNative(tooltipBackground, x + 4, y - 1, width, height + 2, dst);
	this->drawScaledToNative(*tooltip.get(), x + 4, y, width, height, dst);
}

void ChooseClassPanel::render(SDL_Surface *dst, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(dst);

	// Draw background.
	const auto &background = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::CharacterCreation));
	this->drawLetterbox(background, dst, letterbox);

	// Draw parchments: title, list.
	this->parchment->setTransparentColor(Color::Magenta);

	auto listParchmentXScale = 1.0;
	auto listParchmentYScale = 1.0;
	auto listParchmentWidth =
		static_cast<int>(this->parchment->getWidth() * listParchmentXScale);
	auto listParchmentHeight =
		static_cast<int>(this->parchment->getHeight() * listParchmentYScale);

	this->drawScaledToNative(
		*this->parchment.get(),
		(ORIGINAL_WIDTH / 2) - (listParchmentWidth / 2),
		35,
		listParchmentWidth,
		listParchmentHeight,
		dst);

	listParchmentXScale = 0.80;
	listParchmentYScale = 2.20;
	listParchmentWidth =
		static_cast<int>(this->parchment->getWidth() * listParchmentXScale);
	listParchmentHeight =
		static_cast<int>(this->parchment->getHeight() * listParchmentYScale);

	this->drawScaledToNative(
		*this->parchment.get(),
		(ORIGINAL_WIDTH / 2) - (listParchmentWidth / 2),
		(ORIGINAL_HEIGHT / 2) - 11,
		listParchmentWidth,
		listParchmentHeight,
		dst);

	// Draw scroll buttons.
	this->upDownSurface->setTransparentColor(Color::Magenta);
	this->drawScaledToNative(*this->upDownSurface.get(), dst);

	// Draw text: title, list.
	this->drawScaledToNative(*this->titleTextBox.get(), dst);
	this->drawScaledToNative(*this->classesListBox.get(), dst);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, dst);

	// Draw tooltip if over a valid element in the list box.
	auto mouseOriginalPoint = this->nativePointToOriginal(this->getMousePosition());
	if (this->classesListBox->containsPoint(mouseOriginalPoint))
	{
		int index = this->classesListBox->getClickedIndex(mouseOriginalPoint);
		if ((index >= 0) && (index < this->classesListBox->getElementCount()))
		{
			auto characterClass = std::unique_ptr<CharacterClass>(new CharacterClass(
				*this->charClasses.at(index).get()));
			this->drawClassTooltip(*characterClass.get(), dst);
		}
	}
}
