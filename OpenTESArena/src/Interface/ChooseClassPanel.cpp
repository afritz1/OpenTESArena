#include <algorithm>

#include "SDL.h"

#include "ChooseClassCreationPanel.h"
#include "ChooseClassPanel.h"
#include "ChooseNamePanel.h"
#include "CursorAlignment.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Assets/ExeData.h"
#include "../Assets/MiscAssets.h"
#include "../Entities/CharacterClassCategory.h"
#include "../Entities/CharacterClassCategoryName.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Items/ArmorMaterial.h"
#include "../Items/MetalType.h"
#include "../Items/Shield.h"
#include "../Items/Weapon.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/FontLibrary.h"
#include "../Media/FontName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"

#include "components/debug/Debug.h"

const int ChooseClassPanel::MAX_TOOLTIP_LINE_LENGTH = 14;

ChooseClassPanel::ChooseClassPanel(Game &game)
	: Panel(game)
{
	// Read in character classes (just copy from misc. assets for now).
	const auto &classDefs = game.getMiscAssets().getClassDefinitions();
	this->charClasses = std::vector<CharacterClass>(classDefs.begin(), classDefs.end());
	DebugAssert(this->charClasses.size() > 0);

	// Sort character classes alphabetically for use with the list box.
	std::sort(this->charClasses.begin(), this->charClasses.end(),
		[](const CharacterClass &a, const CharacterClass &b)
	{
		return a.getName().compare(b.getName()) < 0;
	});

	this->titleTextBox = [&game]()
	{
		const int x = 89;
		const int y = 32;

		const auto &exeData = game.getMiscAssets().getExeData();
		const std::string &text = exeData.charCreation.chooseClassList;

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			FontName::C,
			Color(211, 211, 211),
			TextAlignment::Left,
			fontLibrary);

		return std::make_unique<TextBox>(x, y, richText, fontLibrary, game.getRenderer());
	}();

	this->classesListBox = [this, &game]()
	{
		const auto &exeData = game.getMiscAssets().getExeData();
		const Rect classListRect = ChooseClassPanel::getClassListRect(exeData);
		const int x = classListRect.getLeft();
		const int y = classListRect.getTop();
		const int maxDisplayed = 6;

		std::vector<std::string> elements;

		// This depends on the character classes being already sorted.
		for (const auto &charClass : this->charClasses)
		{
			elements.push_back(charClass.getName());
		}

		return std::make_unique<ListBox>(
			x,
			y,
			Color(85, 44, 20),
			elements,
			FontName::A,
			maxDisplayed,
			game.getFontLibrary(),
			game.getRenderer());
	}();

	this->backToClassCreationButton = []()
	{
		auto function = [](Game &game)
		{
			game.setPanel<ChooseClassCreationPanel>(game);
		};
		return Button<Game&>(function);
	}();

	this->upButton = [&game]
	{
		const auto &exeData = game.getMiscAssets().getExeData();
		const auto &chooseClassListUI = exeData.ui.chooseClassList;
		const int x = chooseClassListUI.buttonUp.x;
		const int y = chooseClassListUI.buttonUp.y;
		const int w = chooseClassListUI.buttonUp.w;
		const int h = chooseClassListUI.buttonUp.h;
		auto function = [](ChooseClassPanel &panel)
		{
			// Scroll the list box up one if able.
			if (panel.classesListBox->getScrollIndex() > 0)
			{
				panel.classesListBox->scrollUp();
			}
		};
		return Button<ChooseClassPanel&>(x, y, w, h, function);
	}();

	this->downButton = [&game]
	{
		const auto &exeData = game.getMiscAssets().getExeData();
		const auto &chooseClassListUI = exeData.ui.chooseClassList;
		const int x = chooseClassListUI.buttonDown.x;
		const int y = chooseClassListUI.buttonDown.y;
		const int w = chooseClassListUI.buttonDown.w;
		const int h = chooseClassListUI.buttonDown.h;
		auto function = [](ChooseClassPanel &panel)
		{
			// Scroll the list box down one if able.
			const int scrollIndex = panel.classesListBox->getScrollIndex();
			const int elementCount = panel.classesListBox->getElementCount();
			const int maxDisplayedCount = panel.classesListBox->getMaxDisplayedCount();
			if (scrollIndex < (elementCount - maxDisplayedCount))
			{
				panel.classesListBox->scrollDown();
			}
		};
		return Button<ChooseClassPanel&>(x, y, w, h, function);
	}();

	this->acceptButton = []
	{
		auto function = [](Game &game, const CharacterClass &charClass)
		{
			auto &charCreationState = game.getCharacterCreationState();
			charCreationState.setClassIndex(charClass.getClassIndex());

			game.setPanel<ChooseNamePanel>(game);
		};

		return Button<Game&, const CharacterClass&>(function);
	}();

	// Leave the tooltip textures empty for now. Let them be created on demand. 
	// Generating them all at once here is too slow in debug mode.
	DebugAssert(this->tooltipTextures.size() == 0);
}

Panel::CursorData ChooseClassPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void ChooseClassPanel::handleEvent(const SDL_Event &e)
{
	// Eventually handle mouse motion: if mouse is over scroll bar and
	// LMB state is down, move scroll bar to that Y position.
	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);

	if (escapePressed)
	{
		this->backToClassCreationButton.click(game);
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	bool mouseWheelUp = inputManager.mouseWheeledUp(e);
	bool mouseWheelDown = inputManager.mouseWheeledDown(e);

	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPoint = game.getRenderer()
		.nativeToOriginal(mousePosition);

	// See if a class in the list was clicked, or if it is being scrolled. Use a custom
	// width for the list box so it better fills the screen-space.
	const auto &exeData = game.getMiscAssets().getExeData();
	const Rect classListRect = ChooseClassPanel::getClassListRect(exeData);
	if (classListRect.contains(originalPoint))
	{
		if (leftClick)
		{
			// Verify that the clicked index is valid. If so, use that character class.
			const int index = this->classesListBox->getClickedIndex(originalPoint);
			if ((index >= 0) && (index < this->classesListBox->getElementCount()))
			{
				DebugAssertIndex(this->charClasses, index);
				const CharacterClass &charClass = this->charClasses[index];
				this->acceptButton.click(game, charClass);
			}
		}
		else if (mouseWheelUp)
		{
			this->upButton.click(*this);
		}
		else if (mouseWheelDown)
		{
			this->downButton.click(*this);
		}
	}
	else if (leftClick)
	{
		// Check scroll buttons (they are outside the list box to the left).
		if (this->upButton.contains(originalPoint))
		{
			this->upButton.click(*this);
		}
		else if (this->downButton.contains(originalPoint))
		{
			this->downButton.click(*this);
		}
	}
}

std::string ChooseClassPanel::getClassArmors(const CharacterClass &charClass) const
{
	const int armorCount = static_cast<int>(charClass.getAllowedArmors().size());

	// Sort as they are listed in the CharacterClassParser.
	auto allowedArmors = charClass.getAllowedArmors();
	std::sort(allowedArmors.begin(), allowedArmors.end());

	std::string armorString;

	// Decide what the armor string says.
	if (armorCount == 0)
	{
		armorString = "None";
	}
	else
	{
		int lengthCounter = 0;

		// Collect all allowed armor display names for the class.
		for (int i = 0; i < armorCount; i++)
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

std::string ChooseClassPanel::getClassShields(const CharacterClass &charClass) const
{
	const int shieldCount = static_cast<int>(charClass.getAllowedShields().size());

	// Sort as they are listed in the CharacterClassParser.
	auto allowedShields = charClass.getAllowedShields();
	std::sort(allowedShields.begin(), allowedShields.end());

	std::string shieldsString;

	// Decide what the shield string says.
	if (shieldCount == 0)
	{
		shieldsString = "None";
	}
	else
	{
		int lengthCounter = 0;

		// Collect all allowed shield display names for the class.
		for (int i = 0; i < shieldCount; i++)
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

std::string ChooseClassPanel::getClassWeapons(const CharacterClass &charClass) const
{
	const int weaponCount = static_cast<int>(charClass.getAllowedWeapons().size());

	// Get weapon names from the executable.
	const auto &exeData = this->getGame().getMiscAssets().getExeData();
	const auto &weaponStrings = exeData.equipment.weaponNames;

	// Sort as they are listed in the CharacterClassParser.
	std::vector<int> allowedWeapons = charClass.getAllowedWeapons();
	std::sort(allowedWeapons.begin(), allowedWeapons.end(),
		[&weaponStrings](int a, int b)
	{
		const std::string &aStr = weaponStrings.at(a);
		const std::string &bStr = weaponStrings.at(b);
		return aStr.compare(bStr) < 0;
	});

	std::string weaponsString;

	// Decide what the weapon string says.
	if (weaponCount == 0)
	{
		// If the class is allowed zero weapons, it still doesn't exclude fists, I think.
		weaponsString = "None";
	}
	else
	{
		int lengthCounter = 0;

		// Collect all allowed weapon display names for the class.
		for (int i = 0; i < weaponCount; i++)
		{
			const int weaponID = allowedWeapons.at(i);
			const std::string &weaponName = weaponStrings.at(weaponID);
			lengthCounter += static_cast<int>(weaponName.size());
			weaponsString.append(weaponName);

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

Rect ChooseClassPanel::getClassListRect(const ExeData &exeData)
{
	const auto &chooseClassListUI = exeData.ui.chooseClassList;
	return Rect(chooseClassListUI.area.x, chooseClassListUI.area.y,
		chooseClassListUI.area.w, chooseClassListUI.area.h);
}

void ChooseClassPanel::drawClassTooltip(int tooltipIndex, Renderer &renderer)
{
	// Make the tooltip if it doesn't already exist.
	auto tooltipIter = this->tooltipTextures.find(tooltipIndex);
	if (tooltipIter == this->tooltipTextures.end())
	{
		const auto &characterClass = this->charClasses.at(tooltipIndex);

		const std::string text = characterClass.getName() + " (" +
			CharacterClassCategory::toString(characterClass.getCategoryName()) + " class)\n" +
			"\n" + (characterClass.canCastMagic() ? "Can" : "Cannot") + " cast magic" + "\n" +
			"Health die: " + "d" + std::to_string(characterClass.getHealthDie()) + "\n" +
			"Armors: " + this->getClassArmors(characterClass) + "\n" +
			"Shields: " + this->getClassShields(characterClass) + "\n" +
			"Weapons: " + this->getClassWeapons(characterClass);

		Texture texture = Panel::createTooltip(
			text, FontName::D, this->getGame().getFontLibrary(), renderer);

		tooltipIter = this->tooltipTextures.emplace(std::make_pair(
			tooltipIndex, std::move(texture))).first;
	}

	const Texture &tooltip = tooltipIter->second;

	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);
	const int mouseX = originalPosition.x;
	const int mouseY = originalPosition.y;
	const int x = ((mouseX + 8 + tooltip.getWidth()) < Renderer::ORIGINAL_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < Renderer::ORIGINAL_HEIGHT) ?
		(mouseY - 1) : (mouseY - tooltip.getHeight());

	renderer.drawOriginal(tooltip, x, y);
}

void ChooseClassPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw background.
	auto &game = this->getGame();
	const auto &textureManager = game.getTextureManager();
	const TextureID backgroundTextureID = this->getTextureID(
		TextureName::CharacterCreation, PaletteName::BuiltIn);
	const TextureRef backgroundTexture = textureManager.getTextureRef(backgroundTextureID);
	renderer.drawOriginal(backgroundTexture.get());

	// Draw list pop-up.
	const TextureID listPopUpTextureID = this->getTextureID(
		TextureFile::fromName(TextureName::PopUp2),
		TextureFile::fromName(TextureName::CharacterCreation));
	const TextureRef listPopUpTexture = textureManager.getTextureRef(listPopUpTextureID);
	renderer.drawOriginal(listPopUpTexture.get(), 55, 9,
		listPopUpTexture.getWidth(), listPopUpTexture.getHeight());

	// Draw text: title, list.
	renderer.drawOriginal(this->titleTextBox->getTexture(),
		this->titleTextBox->getX(), this->titleTextBox->getY());
	renderer.drawOriginal(this->classesListBox->getTexture(),
		this->classesListBox->getPoint().x,
		this->classesListBox->getPoint().y);

	// Draw tooltip if over a valid element in the list box.
	const auto &inputManager = game.getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPoint = renderer.nativeToOriginal(mousePosition);

	const auto &exeData = game.getMiscAssets().getExeData();
	const Rect classListRect = ChooseClassPanel::getClassListRect(exeData);
	if (classListRect.contains(originalPoint))
	{
		int index = this->classesListBox->getClickedIndex(originalPoint);
		if ((index >= 0) && (index < this->classesListBox->getElementCount()))
		{
			this->drawClassTooltip(index, renderer);
		}
	}
}
