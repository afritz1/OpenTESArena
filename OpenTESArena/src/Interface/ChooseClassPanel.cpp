#include <algorithm>

#include "SDL.h"

#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseClassCreationPanel.h"
#include "ChooseClassPanel.h"
#include "ChooseNamePanel.h"
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
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/CursorData.h"
#include "../UI/FontLibrary.h"
#include "../UI/FontName.h"
#include "../UI/RichTextString.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"

#include "components/debug/Debug.h"

ChooseClassPanel::ChooseClassPanel(Game &game)
	: Panel(game) { }

bool ChooseClassPanel::init()
{
	auto &game = this->getGame();

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
		const std::string &aName = a.getName();
		const std::string &bName = b.getName();
		return aName.compare(bName) < 0;
	});

	this->titleTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			CharacterCreationUiModel::getChooseClassTitleText(game),
			CharacterCreationUiView::ChooseClassTitleFontName,
			CharacterCreationUiView::ChooseClassTitleColor,
			CharacterCreationUiView::ChooseClassTitleAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			CharacterCreationUiView::ChooseClassTitleX,
			CharacterCreationUiView::ChooseClassTitleY,
			richText,
			fontLibrary,
			game.getRenderer());
	}();

	this->classesListBox = [this, &game]()
	{
		const Rect classListRect = CharacterCreationUiView::getClassListRect(game);

		// This depends on the character classes being already sorted.
		std::vector<std::string> elements;
		for (const auto &charClass : this->charClasses)
		{
			elements.push_back(charClass.getName());
		}

		return std::make_unique<ListBox>(
			classListRect.getLeft(),
			classListRect.getTop(),
			CharacterCreationUiView::ChooseClassListBoxTextColor,
			elements,
			CharacterCreationUiView::ChooseClassListBoxFontName,
			CharacterCreationUiView::ChooseClassListBoxMaxDisplayedItems,
			game.getFontLibrary(),
			game.getRenderer());
	}();

	this->backToClassCreationButton = Button<Game&>(CharacterCreationUiController::onBackToChooseClassCreationButtonSelected);

	this->upButton = [&game]
	{
		const Rect rect = CharacterCreationUiView::getClassListUpButtonRect(game);
		return Button<ListBox&>(
			rect.getLeft(),
			rect.getTop(),
			rect.getWidth(),
			rect.getHeight(),
			CharacterCreationUiController::onChooseClassListBoxUpButtonSelected);
	}();

	this->downButton = [&game]
	{
		const Rect rect = CharacterCreationUiView::getClassListDownButtonRect(game);
		return Button<ListBox&>(
			rect.getLeft(),
			rect.getTop(),
			rect.getWidth(),
			rect.getHeight(),
			CharacterCreationUiController::onChooseClassListBoxDownButtonSelected);
	}();

	this->acceptButton = Button<Game&, int>(CharacterCreationUiController::onChooseClassListBoxAcceptButtonSelected);

	// Leave the tooltip textures empty for now. Let them be created on demand. Generating them all at once here
	// is too slow in debug mode.
	DebugAssert(this->tooltipTextures.size() == 0);

	return true;
}

std::optional<CursorData> ChooseClassPanel::getCurrentCursor() const
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
	const Int2 originalPoint = game.getRenderer().nativeToOriginal(mousePosition);

	// See if a class in the list was clicked, or if it is being scrolled. Use a custom
	// width for the list box so it better fills the screen-space.
	const Rect classListRect = CharacterCreationUiView::getClassListRect(game);
	if (classListRect.contains(originalPoint))
	{
		if (leftClick)
		{
			// Verify that the clicked index is valid. If so, use that character class.
			const int index = this->classesListBox->getClickedIndex(originalPoint);
			if ((index >= 0) && (index < this->classesListBox->getCount()))
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
			this->upButton.click(*this->classesListBox);
		}
		else if (mouseWheelDown)
		{
			this->downButton.click(*this->classesListBox);
		}
	}
	else if (leftClick)
	{
		// Check scroll buttons (they are outside the list box to the left).
		if (this->upButton.contains(originalPoint))
		{
			this->upButton.click(*this->classesListBox);
		}
		else if (this->downButton.contains(originalPoint))
		{
			this->downButton.click(*this->classesListBox);
		}
	}
}

void ChooseClassPanel::drawClassTooltip(int tooltipIndex, Renderer &renderer)
{
	// Make the tooltip if it doesn't already exist.
	auto tooltipIter = this->tooltipTextures.find(tooltipIndex);
	if (tooltipIter == this->tooltipTextures.end())
	{
		auto &game = this->getGame();

		DebugAssertIndex(this->charClasses, tooltipIndex);
		const auto &charClassDef = this->charClasses[tooltipIndex];
		const std::string text = CharacterCreationUiModel::getChooseClassFullTooltipText(charClassDef, game);
		Texture texture = TextureUtils::createTooltip(text, game.getFontLibrary(), renderer);
		tooltipIter = this->tooltipTextures.emplace(std::make_pair(tooltipIndex, std::move(texture))).first;
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
	const TextureAssetReference backgroundTextureAssetRef = CharacterCreationUiView::getNightSkyTextureAssetRef();
	const std::optional<PaletteID> backgroundPaletteID = textureManager.tryGetPaletteID(backgroundTextureAssetRef);
	if (!backgroundPaletteID.has_value())
	{
		DebugLogError("Couldn't get background palette ID for \"" + backgroundTextureAssetRef.filename + "\".");
		return;
	}

	const std::optional<TextureBuilderID> backgroundTextureBuilderID = textureManager.tryGetTextureBuilderID(backgroundTextureAssetRef);
	if (!backgroundTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get background texture builder ID for \"" + backgroundTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*backgroundTextureBuilderID, *backgroundPaletteID, textureManager);

	// Draw list pop-up.
	const TextureAssetReference listTextureAssetRef = CharacterCreationUiView::getChooseClassListBoxTextureAssetRef();
	const std::optional<TextureBuilderID> listTextureBuilderID = textureManager.tryGetTextureBuilderID(listTextureAssetRef);
	if (!listTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get list pop-up texture builder ID for \"" + listTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*listTextureBuilderID, *backgroundPaletteID,
		CharacterCreationUiView::ChooseClassListBoxTextureX, CharacterCreationUiView::ChooseClassListBoxTextureY, textureManager);

	// Draw text: title, list.
	renderer.drawOriginal(this->titleTextBox->getTexture(), this->titleTextBox->getX(), this->titleTextBox->getY());

	const Rect &classesListBoxRect = this->classesListBox->getRect();
	renderer.drawOriginal(this->classesListBox->getTexture(), classesListBoxRect.getLeft(), classesListBoxRect.getTop());

	// Draw tooltip if over a valid element in the list box.
	const auto &inputManager = game.getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPoint = renderer.nativeToOriginal(mousePosition);

	const Rect classListRect = CharacterCreationUiView::getClassListRect(game);
	if (classListRect.contains(originalPoint))
	{
		int index = this->classesListBox->getClickedIndex(originalPoint);
		if ((index >= 0) && (index < this->classesListBox->getCount()))
		{
			this->drawClassTooltip(index, renderer);
		}
	}
}
