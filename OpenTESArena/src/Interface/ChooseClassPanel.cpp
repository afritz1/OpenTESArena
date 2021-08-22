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
#include "../Input/InputActionName.h"
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
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"

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
		[](const CharacterClassDefinition &a, const CharacterClassDefinition &b) // @todo: move this lambda to UiModel/UiView
	{
		const std::string &aName = a.getName();
		const std::string &bName = b.getName();
		return aName.compare(bName) < 0;
	});

	auto &renderer = game.getRenderer();
	const auto &fontLibrary = game.getFontLibrary();
	const std::string titleText = ChooseClassUiModel::getTitleText(game);
	const TextBox::InitInfo titleTextBoxInitInfo =
		ChooseClassUiView::getTitleTextBoxInitInfo(titleText, fontLibrary);
	if (!this->titleTextBox.init(titleTextBoxInitInfo, titleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	this->classesListBox.init(ChooseClassUiView::getListRect(game),
		ChooseClassUiView::makeListBoxProperties(game.getFontLibrary()), game.getRenderer());

	for (int i = 0; i < static_cast<int>(this->charClasses.size()); i++)
	{
		const CharacterClassDefinition &charClass = this->charClasses[i];
		this->classesListBox.add(std::string(charClass.getName()));
		this->classesListBox.setCallback(i, [&game, &charClass]()
		{
			const auto &charClassLibrary = game.getCharacterClassLibrary();
			int charClassDefID;
			if (!charClassLibrary.tryGetDefinitionIndex(charClass, &charClassDefID))
			{
				DebugLogError("Couldn't get index of character class definition \"" + charClass.getName() + "\".");
				return;
			}

			ChooseClassUiController::onItemButtonSelected(game, charClassDefID);
		});
	}

	this->upButton = [&game]
	{
		const Rect rect = ChooseClassUiView::getUpButtonRect(game);
		return Button<ListBox&>(
			rect.getLeft(),
			rect.getTop(),
			rect.getWidth(),
			rect.getHeight(),
			ChooseClassUiController::onUpButtonSelected);
	}();

	this->downButton = [&game]
	{
		const Rect rect = ChooseClassUiView::getDownButtonRect(game);
		return Button<ListBox&>(
			rect.getLeft(),
			rect.getTop(),
			rect.getWidth(),
			rect.getHeight(),
			ChooseClassUiController::onDownButtonSelected);
	}();

	this->addButtonProxy(MouseButtonType::Left, this->upButton.getRect(),
		[this, &game]() { this->upButton.click(this->classesListBox); });
	this->addButtonProxy(MouseButtonType::Left, this->downButton.getRect(),
		[this, &game]() { this->downButton.click(this->classesListBox); });

	// Add button proxy for each listbox item.
	for (int i = 0; i < this->classesListBox.getCount(); i++)
	{
		auto rectFunc = [this, i]()
		{
			return this->classesListBox.getItemGlobalRect(i);
		};

		auto callback = this->classesListBox.getCallback(i);

		auto isActiveFunc = [&game]()
		{
			const auto &inputManager = game.getInputManager();
			const Int2 mousePosition = inputManager.getMousePosition();
			const Int2 classicPosition = game.getRenderer().nativeToOriginal(mousePosition);
			const Rect classListRect = ChooseClassUiView::getListRect(game);
			return classListRect.contains(classicPosition);
		};

		this->addButtonProxy(MouseButtonType::Left, rectFunc, callback, isActiveFunc);
	}

	this->addInputActionListener(InputActionName::Back, ChooseClassUiController::onBackToChooseClassCreationInputAction);

	this->addMouseScrollChangedListener([this](Game &game, MouseWheelScrollType type, const Int2 &position)
	{
		const Int2 classicPoint = game.getRenderer().nativeToOriginal(position);
		const Rect classListRect = ChooseClassUiView::getListRect(game);
		if (classListRect.contains(classicPoint))
		{
			if (type == MouseWheelScrollType::Down)
			{
				this->downButton.click(this->classesListBox);
			}
			else if (type == MouseWheelScrollType::Up)
			{
				this->upButton.click(this->classesListBox);
			}
		}
	});

	// Leave the tooltip textures empty for now. Let them be created on demand. Generating them all at once here
	// is too slow in debug mode.
	DebugAssert(this->tooltipTextures.size() == 0);

	return true;
}

std::optional<CursorData> ChooseClassPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
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
		const std::string text = ChooseClassUiModel::getFullTooltipText(charClassDef, game);
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
	const TextureAssetReference listTextureAssetRef = ChooseClassUiView::getListBoxTextureAssetRef();
	const std::optional<TextureBuilderID> listTextureBuilderID = textureManager.tryGetTextureBuilderID(listTextureAssetRef);
	if (!listTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get list pop-up texture builder ID for \"" + listTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*listTextureBuilderID, *backgroundPaletteID,
		ChooseClassUiView::ListTextureX, ChooseClassUiView::ListTextureY, textureManager);

	// Draw text: title, list.
	const Rect &titleTextBoxRect = this->titleTextBox.getRect();
	renderer.drawOriginal(this->titleTextBox.getTextureID(), titleTextBoxRect.getLeft(), titleTextBoxRect.getTop());

	const Rect &classesListBoxRect = this->classesListBox.getRect();
	renderer.drawOriginal(this->classesListBox.getTexture(), classesListBoxRect.getLeft(), classesListBoxRect.getTop());

	// Draw tooltip if over a valid element in the list box.
	const auto &inputManager = game.getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPoint = renderer.nativeToOriginal(mousePosition);

	const Rect classListRect = ChooseClassUiView::getListRect(game);
	if (classListRect.contains(originalPoint))
	{
		for (int i = 0; i < this->classesListBox.getCount(); i++)
		{
			const Rect &itemGlobalRect = this->classesListBox.getItemGlobalRect(i);
			if (itemGlobalRect.contains(originalPoint))
			{
				this->drawClassTooltip(i, renderer);
				break;
			}
		}
	}
}
