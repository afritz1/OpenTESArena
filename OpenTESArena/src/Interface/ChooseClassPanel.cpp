#include <algorithm>

#include "SDL.h"

#include "ChooseClassCreationPanel.h"
#include "ChooseClassPanel.h"
#include "ChooseNamePanel.h"
#include "CursorAlignment.h"
#include "RichTextString.h"
#include "Surface.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ExeData.h"
#include "../Entities/CharacterClassLibrary.h"
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
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"

ChooseClassPanel::ChooseClassPanel(Game &game)
	: Panel(game)
{
	// Read in character classes.
	const auto &charClassLibrary = game.getCharacterClassLibrary();
	this->charClasses = std::vector<CharacterClassDefinition>(charClassLibrary.getDefinitionCount());
	DebugAssert(this->charClasses.size() > 0);
	for (int i = 0; i < static_cast<int>(this->charClasses.size()); i++)
	{
		this->charClasses[i] = charClassLibrary.getDefinition(i);
	}

	// Sort character classes alphabetically for use with the list box.
	std::sort(this->charClasses.begin(), this->charClasses.end(),
		[](const CharacterClassDefinition &a, const CharacterClassDefinition &b)
	{
		return a.getName().compare(b.getName()) < 0;
	});

	this->titleTextBox = [&game]()
	{
		const int x = 89;
		const int y = 32;

		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
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
		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
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
		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
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
		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
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
		auto function = [](Game &game, int charClassDefID)
		{
			auto &charCreationState = game.getCharacterCreationState();
			charCreationState.setClassDefID(charClassDefID);

			game.setPanel<ChooseNamePanel>(game);
		};

		return Button<Game&, int>(function);
	}();

	// Leave the tooltip textures empty for now. Let them be created on demand. 
	// Generating them all at once here is too slow in debug mode.
	DebugAssert(this->tooltipTextures.size() == 0);
}

std::optional<Panel::CursorData> ChooseClassPanel::getCurrentCursor() const
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
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
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
				const CharacterClassDefinition &charClassDef = this->charClasses[index];

				const auto &charClassLibrary = game.getCharacterClassLibrary();
				int charClassDefID;
				if (!charClassLibrary.tryGetDefinitionIndex(charClassDef, &charClassDefID))
				{
					DebugLogError("Couldn't get index of character class definition \"" +
						charClassDef.getName() + "\".");
					return;
				}

				this->acceptButton.click(game, charClassDefID);
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

std::string ChooseClassPanel::getClassArmors(const CharacterClassDefinition &charClassDef) const
{
	std::vector<int> allowedArmors(charClassDef.getAllowedArmorCount());
	for (int i = 0; i < static_cast<int>(allowedArmors.size()); i++)
	{
		allowedArmors[i] = charClassDef.getAllowedArmor(i);
	}

	std::sort(allowedArmors.begin(), allowedArmors.end());

	std::string armorString;

	// Decide what the armor string says.
	if (allowedArmors.size() == 0)
	{
		armorString = "None";
	}
	else
	{
		int lengthCounter = 0;

		// Collect all allowed armor display names for the class.
		for (int i = 0; i < static_cast<int>(allowedArmors.size()); i++)
		{
			const int materialType = allowedArmors[i];
			auto materialString = ArmorMaterial::typeToString(
				static_cast<ArmorMaterialType>(materialType));
			lengthCounter += static_cast<int>(materialString.size());
			armorString.append(materialString);

			// If not the last element, add a comma.
			if (i < (static_cast<int>(allowedArmors.size()) - 1))
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

std::string ChooseClassPanel::getClassShields(const CharacterClassDefinition &charClassDef) const
{
	std::vector<int> allowedShields(charClassDef.getAllowedShieldCount());
	for (int i = 0; i < static_cast<int>(allowedShields.size()); i++)
	{
		allowedShields[i] = charClassDef.getAllowedShield(i);
	}

	std::sort(allowedShields.begin(), allowedShields.end());

	std::string shieldsString;

	// Decide what the shield string says.
	if (allowedShields.size() == 0)
	{
		shieldsString = "None";
	}
	else
	{
		int lengthCounter = 0;

		// Collect all allowed shield display names for the class.
		for (int i = 0; i < static_cast<int>(allowedShields.size()); i++)
		{
			const int shieldType = allowedShields[i];
			MetalType dummyMetal = MetalType::Iron;
			auto typeString = Shield(static_cast<ShieldType>(shieldType), dummyMetal).typeToString();
			lengthCounter += static_cast<int>(typeString.size());
			shieldsString.append(typeString);

			// If not the last element, add a comma.
			if (i < (static_cast<int>(allowedShields.size()) - 1))
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

std::string ChooseClassPanel::getClassWeapons(const CharacterClassDefinition &charClassDef) const
{
	// Get weapon names from the executable.
	const auto &exeData = this->getGame().getBinaryAssetLibrary().getExeData();
	const auto &weaponStrings = exeData.equipment.weaponNames;

	std::vector<int> allowedWeapons(charClassDef.getAllowedWeaponCount());
	for (int i = 0; i < static_cast<int>(allowedWeapons.size()); i++)
	{
		allowedWeapons[i] = charClassDef.getAllowedWeapon(i);
	}

	std::sort(allowedWeapons.begin(), allowedWeapons.end(),
		[&weaponStrings](int a, int b)
	{
		const std::string &aStr = weaponStrings.at(a);
		const std::string &bStr = weaponStrings.at(b);
		return aStr.compare(bStr) < 0;
	});

	std::string weaponsString;

	// Decide what the weapon string says.
	if (allowedWeapons.size() == 0)
	{
		// If the class is allowed zero weapons, it still doesn't exclude fists, I think.
		weaponsString = "None";
	}
	else
	{
		int lengthCounter = 0;

		// Collect all allowed weapon display names for the class.
		for (int i = 0; i < static_cast<int>(allowedWeapons.size()); i++)
		{
			const int weaponID = allowedWeapons.at(i);
			const std::string &weaponName = weaponStrings.at(weaponID);
			lengthCounter += static_cast<int>(weaponName.size());
			weaponsString.append(weaponName);

			// If not the the last element, add a comma.
			if (i < (static_cast<int>(allowedWeapons.size()) - 1))
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
		const auto &charClassDef = this->charClasses.at(tooltipIndex);

		// Doesn't look like the category name is easy to get from the original data.
		// Potentially could attach something to the char class definition like a bool
		// saying "the class name is also a category name".
		constexpr std::array<const char*, 3> ClassCategoryNames =
		{
			"Mage", "Thief", "Warrior"
		};

		const int categoryIndex = charClassDef.getCategoryID();
		DebugAssertIndex(ClassCategoryNames, categoryIndex);
		const std::string categoryName = ClassCategoryNames[categoryIndex];

		const std::string text = charClassDef.getName() + " (" +
			categoryName + " class)\n" +
			"\n" + (charClassDef.canCastMagic() ? "Can" : "Cannot") + " cast magic" + "\n" +
			"Health die: " + "d" + std::to_string(charClassDef.getHealthDie()) + "\n" +
			"Armors: " + this->getClassArmors(charClassDef) + "\n" +
			"Shields: " + this->getClassShields(charClassDef) + "\n" +
			"Weapons: " + this->getClassWeapons(charClassDef);

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
	const int x = ((mouseX + 8 + tooltip.getWidth()) < ArenaRenderUtils::SCREEN_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < ArenaRenderUtils::SCREEN_HEIGHT) ?
		(mouseY - 1) : (mouseY - tooltip.getHeight());

	renderer.drawOriginal(tooltip, x, y);
}

void ChooseClassPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw background.
	auto &game = this->getGame();
	auto &textureManager = game.getTextureManager();
	const std::string &backgroundFilename = ArenaTextureName::CharacterCreation;
	const std::optional<PaletteID> backgroundPaletteID = textureManager.tryGetPaletteID(backgroundFilename.c_str());
	if (!backgroundPaletteID.has_value())
	{
		DebugLogError("Couldn't get background palette ID for \"" + backgroundFilename + "\".");
		return;
	}

	const std::optional<TextureBuilderID> backgroundTextureBuilderID =
		textureManager.tryGetTextureBuilderID(backgroundFilename.c_str());
	if (!backgroundTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get background texture builder ID for \"" + backgroundFilename + "\".");
		return;
	}

	renderer.drawOriginal(*backgroundTextureBuilderID, *backgroundPaletteID, textureManager);

	// Draw list pop-up.
	const std::string &listPopUpFilename = ArenaTextureName::PopUp2;
	const std::optional<TextureBuilderID> listPopUpTextureBuilderID =
		textureManager.tryGetTextureBuilderID(listPopUpFilename.c_str());
	if (!listPopUpTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get list pop-up texture builder ID for \"" + listPopUpFilename + "\".");
		return;
	}

	renderer.drawOriginal(*listPopUpTextureBuilderID, *backgroundPaletteID, 55, 9, textureManager);

	// Draw text: title, list.
	renderer.drawOriginal(this->titleTextBox->getTexture(),
		this->titleTextBox->getX(), this->titleTextBox->getY());
	renderer.drawOriginal(this->classesListBox->getTexture(),
		this->classesListBox->getPoint().x, this->classesListBox->getPoint().y);

	// Draw tooltip if over a valid element in the list box.
	const auto &inputManager = game.getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPoint = renderer.nativeToOriginal(mousePosition);

	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
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
