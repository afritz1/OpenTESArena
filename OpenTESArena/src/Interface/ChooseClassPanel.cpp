#include <algorithm>
#include <cstring>

#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseClassPanel.h"
#include "CommonUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../UI/FontLibrary.h"

#include "components/debug/Debug.h"
#include "components/utilities/StringView.h"

ChooseClassPanel::ChooseClassPanel(Game &game)
	: Panel(game) { }

bool ChooseClassPanel::init()
{
	auto &game = this->getGame();

	// Read in character classes.
	const auto &charClassLibrary = CharacterClassLibrary::getInstance();
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
		return StringView::compare(a.name, b.name) < 0;
	});

	auto &renderer = game.renderer;
	const auto &fontLibrary = FontLibrary::getInstance();
	const std::string titleText = ChooseClassUiModel::getTitleText(game);
	const TextBoxInitInfo titleTextBoxInitInfo = ChooseClassUiView::getTitleTextBoxInitInfo(titleText, fontLibrary);
	if (!this->titleTextBox.init(titleTextBoxInitInfo, titleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	const TextBoxInitInfo classDescriptionTextBoxInitInfo = ChooseClassUiView::getClassDescriptionTextBoxInitInfo(fontLibrary);
	if (!this->classDescriptionTextBox.init(classDescriptionTextBoxInitInfo, renderer))
	{
		DebugLogError("Couldn't init class description text box.");
		return false;
	}

	this->classesListBox.init(ChooseClassUiView::getListRect(game),
		ChooseClassUiView::makeListBoxProperties(FontLibrary::getInstance()), game.renderer);

	for (int i = 0; i < static_cast<int>(this->charClasses.size()); i++)
	{
		const CharacterClassDefinition &charClass = this->charClasses[i];
		this->classesListBox.add(std::string(charClass.name));
		this->classesListBox.setCallback(i, [&game, &charClass]()
		{
			const auto &charClassLibrary = CharacterClassLibrary::getInstance();
			int charClassDefID;
			if (!charClassLibrary.tryGetDefinitionIndex(charClass, &charClassDefID))
			{
				DebugLogErrorFormat("Couldn't get index of character class definition \"%s\".", charClass.name);
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
			rect.width,
			rect.height,
			ChooseClassUiController::onUpButtonSelected);
	}();

	this->downButton = [&game]
	{
		const Rect rect = ChooseClassUiView::getDownButtonRect(game);
		return Button<ListBox&>(
			rect.getLeft(),
			rect.getTop(),
			rect.width,
			rect.height,
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

		this->addButtonProxy(MouseButtonType::Left, rectFunc, callback, this->classesListBox.getRect());
	}

	this->addInputActionListener(InputActionName::Back, ChooseClassUiController::onBackToChooseClassCreationInputAction);

	auto updateHoveredClassIndex = [this, &game]()
	{
		// Draw tooltip if over a valid element in the list box.
		const Window &window = game.window;
		const InputManager &inputManager = game.inputManager;
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPoint = window.nativeToOriginal(mousePosition);

		const Rect classListRect = ChooseClassUiView::getListRect(game);
		if (classListRect.contains(originalPoint))
		{
			for (int i = 0; i < this->classesListBox.getCount(); i++)
			{
				const Rect &itemGlobalRect = this->classesListBox.getItemGlobalRect(i);
				if (itemGlobalRect.contains(originalPoint))
				{
					if (i != this->hoveredClassIndex)
					{
						this->hoveredClassIndex = i;
						DebugAssertIndex(this->charClasses, i);
						const auto &charClassDef = this->charClasses[i];
						const std::string text = ChooseClassUiModel::getFullTooltipText(charClassDef, game);
						this->classDescriptionTextBox.setText(text);
						break;
					}
				}
			}
		}
		else
		{
			this->hoveredClassIndex = std::nullopt;
			this->classDescriptionTextBox.setText(std::string());
		}
	};

	this->addMouseScrollChangedListener([this, updateHoveredClassIndex](Game &game, MouseWheelScrollType type, const Int2 &position)
	{
		const Int2 classicPoint = game.window.nativeToOriginal(position);
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

			updateHoveredClassIndex();
		}
	});

	this->addMouseMotionListener([this, updateHoveredClassIndex](Game &game, int dx, int dy)
	{
		updateHoveredClassIndex();
	});

	TextureManager &textureManager = game.textureManager;
	const UiTextureID nightSkyTextureID = CharacterCreationUiView::allocNightSkyTexture(textureManager, renderer);
	const UiTextureID popUpTextureID = ChooseClassUiView::allocPopUpTexture(textureManager, renderer);
	this->nightSkyTextureRef.init(nightSkyTextureID, renderer);
	this->popUpTextureRef.init(popUpTextureID, renderer);

	UiDrawCallInitInfo nightSkyDrawCallInitInfo;
	nightSkyDrawCallInitInfo.textureID = this->nightSkyTextureRef.get();
	nightSkyDrawCallInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	this->addDrawCall(nightSkyDrawCallInitInfo);

	UiDrawCallInitInfo popUpTextureDrawCallInitInfo;
	popUpTextureDrawCallInitInfo.textureID = this->popUpTextureRef.get();
	popUpTextureDrawCallInitInfo.position = Int2(ChooseClassUiView::ListTextureX, ChooseClassUiView::ListTextureY);
	popUpTextureDrawCallInitInfo.size = this->popUpTextureRef.getDimensions();
	this->addDrawCall(popUpTextureDrawCallInitInfo);

	const Rect titleTextBoxRect = this->titleTextBox.getRect();
	UiDrawCallInitInfo titleDrawCallInitInfo;
	titleDrawCallInitInfo.textureID = this->titleTextBox.getTextureID();
	titleDrawCallInitInfo.position = titleTextBoxRect.getCenter();
	titleDrawCallInitInfo.size = titleTextBoxRect.getSize();
	titleDrawCallInitInfo.pivotType = UiPivotType::Middle;
	this->addDrawCall(titleDrawCallInitInfo);

	const Rect classDescTextBoxRect = this->classDescriptionTextBox.getRect();
	UiDrawCallInitInfo classDescriptionDrawCallInitInfo;
	classDescriptionDrawCallInitInfo.textureFunc = [this]() { return this->classDescriptionTextBox.getTextureID(); };
	classDescriptionDrawCallInitInfo.position = classDescTextBoxRect.getCenter();
	classDescriptionDrawCallInitInfo.size = classDescTextBoxRect.getSize();
	classDescriptionDrawCallInitInfo.pivotType = UiPivotType::Middle;
	this->addDrawCall(classDescriptionDrawCallInitInfo);

	const Rect listBoxRect = this->classesListBox.getRect();
	UiDrawCallInitInfo listBoxDrawCallInitInfo;
	listBoxDrawCallInitInfo.textureFunc = [this]() { return this->classesListBox.getTextureID(); };
	listBoxDrawCallInitInfo.position = listBoxRect.getCenter();
	listBoxDrawCallInitInfo.size = listBoxRect.getSize();
	listBoxDrawCallInitInfo.pivotType = UiPivotType::Middle;
	this->addDrawCall(listBoxDrawCallInitInfo);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	updateHoveredClassIndex();

	return true;
}
