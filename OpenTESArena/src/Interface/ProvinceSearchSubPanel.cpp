#include <algorithm>

#include "CommonUiView.h"
#include "ProvinceMapPanel.h"
#include "ProvinceMapUiController.h"
#include "ProvinceMapUiModel.h"
#include "ProvinceMapUiView.h"
#include "ProvinceSearchSubPanel.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/TextureManager.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../Input/TextEvents.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextEntry.h"
#include "../Utilities/Color.h"

#include "components/utilities/String.h"

ProvinceSearchSubPanel::ProvinceSearchSubPanel(Game &game)
	: Panel(game) { }

bool ProvinceSearchSubPanel::init(ProvinceMapPanel &provinceMapPanel, int provinceID)
{
	auto &game = this->getGame();
	auto &renderer = game.renderer;
	const auto &fontLibrary = FontLibrary::getInstance();

	// Don't initialize the locations list box until it's reached, since its contents
	// may depend on the search results.
	const std::string textTitleText = ProvinceSearchUiModel::getTitleText(game);
	const TextBoxInitInfo textTitleTextBoxInitInfo =
		ProvinceSearchUiView::getTitleTextBoxInitInfo(textTitleText, fontLibrary);
	if (!this->textTitleTextBox.init(textTitleTextBoxInitInfo, textTitleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	const TextBoxInitInfo textEntryTextBoxInitInfo = ProvinceSearchUiView::getTextEntryTextBoxInitInfo(fontLibrary);
	if (!this->textEntryTextBox.init(textEntryTextBoxInitInfo, renderer))
	{
		DebugLogError("Couldn't init text entry text box.");
		return false;
	}

	// Button proxies for these are added when the locations list is initialized.
	this->listUpButton = Button<ListBox&>(
		ProvinceSearchUiView::ListUpButtonCenterPoint,
		ProvinceSearchUiView::ListUpButtonWidth,
		ProvinceSearchUiView::ListUpButtonHeight,
		ProvinceSearchUiController::onListUpButtonSelected);
	this->listDownButton = Button<ListBox&>(
		ProvinceSearchUiView::ListDownButtonCenterPoint,
		ProvinceSearchUiView::ListDownButtonWidth,
		ProvinceSearchUiView::ListDownButtonHeight,
		ProvinceSearchUiController::onListDownButtonSelected);

	this->addInputActionListener(InputActionName::Accept,
		[this, &game](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			if (this->mode == ProvinceSearchUiModel::Mode::TextEntry)
			{
				// Begin the next step in the location search. Run the entered text through some
				// checks to see if it matches any location names.
				ProvinceSearchUiController::onTextAccepted(this->getGame(), *this);
			}
		}
	});

	this->addInputActionListener(InputActionName::Back,
		[this, &game](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			auto &inputManager = game.inputManager;
			inputManager.setTextInputMode(false);

			// Return to the province map panel.
			game.popSubPanel();
		}
	});

	this->addInputActionListener(InputActionName::Backspace,
		[this](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			if (this->mode == ProvinceSearchUiModel::Mode::TextEntry)
			{
				const bool textChanged = TextEntry::backspace(this->locationName);
				if (textChanged)
				{
					this->textEntryTextBox.setText(this->locationName);
				}
			}
		}
	});

	this->addMouseScrollChangedListener([this](Game &game, MouseWheelScrollType type, const Int2 &position)
	{
		if (this->mode == ProvinceSearchUiModel::Mode::List)
		{
			const Rect &listBoxRect = this->locationsListBox.getRect();
			const Int2 classicPosition = game.window.nativeToOriginal(position);
			if (listBoxRect.contains(classicPosition))
			{
				if (type == MouseWheelScrollType::Up)
				{
					this->listUpButton.click(this->locationsListBox);
				}
				else if (type == MouseWheelScrollType::Down)
				{
					this->listDownButton.click(this->locationsListBox);
				}
			}
		}
	});

	this->addTextInputListener([this](const std::string_view text)
	{
		if (this->mode == ProvinceSearchUiModel::Mode::TextEntry)
		{
			const bool textChanged = TextEntry::append(this->locationName, text,
				ProvinceSearchUiModel::isCharAllowed, ProvinceSearchUiModel::MaxNameLength);

			if (textChanged)
			{
				this->textEntryTextBox.setText(this->locationName);
			}
		}
	});

	TextureManager &textureManager = game.textureManager;
	const UiTextureID parchmentTextureID = ProvinceSearchUiView::allocParchmentTexture(textureManager, renderer);
	this->parchmentTextureRef.init(parchmentTextureID, renderer);

	UiDrawCallActiveFunc textEntryActiveFunc = [this]()
	{
		return this->mode == ProvinceSearchUiModel::Mode::TextEntry;
	};

	UiDrawCallInitInfo parchmentTextureDrawCallInitInfo;
	parchmentTextureDrawCallInitInfo.textureID = this->parchmentTextureRef.get();
	parchmentTextureDrawCallInitInfo.position = Int2((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, (ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	parchmentTextureDrawCallInitInfo.size = Int2(ProvinceSearchUiView::TextureWidth, ProvinceSearchUiView::TextureHeight);
	parchmentTextureDrawCallInitInfo.pivotType = PivotType::Middle;
	parchmentTextureDrawCallInitInfo.activeFunc = textEntryActiveFunc;
	this->addDrawCall(parchmentTextureDrawCallInitInfo);

	const Rect textTitleTextBoxRect = this->textTitleTextBox.getRect();
	UiDrawCallInitInfo titleDrawCallInitInfo;
	titleDrawCallInitInfo.textureID = this->textTitleTextBox.getTextureID();
	titleDrawCallInitInfo.position = textTitleTextBoxRect.getTopLeft();
	titleDrawCallInitInfo.size = textTitleTextBoxRect.getSize();
	titleDrawCallInitInfo.activeFunc = textEntryActiveFunc;
	this->addDrawCall(titleDrawCallInitInfo);

	const Rect textEntryTextBoxRect = this->textEntryTextBox.getRect();
	UiDrawCallInitInfo textEntryDrawCallInitInfo;
	textEntryDrawCallInitInfo.textureFunc = [this]() { return this->textEntryTextBox.getTextureID(); };
	textEntryDrawCallInitInfo.position = textEntryTextBoxRect.getTopLeft();
	textEntryDrawCallInitInfo.size = textEntryTextBoxRect.getSize();
	textEntryDrawCallInitInfo.activeFunc = textEntryActiveFunc;
	this->addDrawCall(textEntryDrawCallInitInfo);

	// @todo: draw blinking cursor for text entry

	const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const UiTextureID listBackgroundTextureID = ProvinceSearchUiView::allocListBackgroundTexture(provinceID, binaryAssetLibrary, textureManager, renderer);
	listBackgroundTextureRef.init(listBackgroundTextureID, renderer);

	UiDrawCallActiveFunc listActiveFunc = [this]()
	{
		return this->mode == ProvinceSearchUiModel::Mode::List;
	};

	UiDrawCallInitInfo listBgDrawCallInitInfo;
	listBgDrawCallInitInfo.textureID = this->listBackgroundTextureRef.get();
	listBgDrawCallInitInfo.position = Int2(ProvinceSearchUiView::ListTextureX, ProvinceSearchUiView::ListTextureY);
	listBgDrawCallInitInfo.size = this->listBackgroundTextureRef.getDimensions();
	listBgDrawCallInitInfo.activeFunc = listActiveFunc;
	this->addDrawCall(listBgDrawCallInitInfo);

	UiDrawCallInitInfo locationsListDrawCallInitInfo;
	locationsListDrawCallInitInfo.textureFunc = [this]() { return this->locationsListBox.getTextureID(); };

	// Have to get position and size dynamically since the list only gets initialized after finishing searching.
	locationsListDrawCallInitInfo.positionFunc = [this]() { return this->locationsListBox.getRect().getTopLeft(); };
	locationsListDrawCallInitInfo.sizeFunc = [this]() { return this->locationsListBox.getRect().getSize(); };

	locationsListDrawCallInitInfo.activeFunc = listActiveFunc;
	this->addDrawCall(locationsListDrawCallInitInfo);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), PivotType::TopLeft, listActiveFunc);

	this->provinceMapPanel = &provinceMapPanel;
	this->mode = ProvinceSearchUiModel::Mode::TextEntry;
	this->provinceID = provinceID;

	// Start with text input enabled.
	auto &inputManager = game.inputManager;
	inputManager.setTextInputMode(true);

	return true;
}

void ProvinceSearchSubPanel::initLocationsList()
{
	// @todo: move the locationNames into UiModel
	auto &game = this->getGame();
	auto &gameState = game.gameState;
	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
	const WorldMapInstance &worldMapInst = gameState.getWorldMapInstance();
	const ProvinceInstance &provinceInst = worldMapInst.getProvinceInstance(this->provinceID);
	const int provinceDefIndex = provinceInst.getProvinceDefIndex();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceDefIndex);

	this->locationsListBox.init(ProvinceSearchUiView::ListBoxRect,
		ProvinceSearchUiView::makeListBoxProperties(FontLibrary::getInstance()), game.renderer);

	this->clearButtonProxies();

	// Add list box scroll button proxies.
	this->addButtonProxy(MouseButtonType::Left, this->listUpButton.getRect(),
		[this]()
	{
		this->listUpButton.click(this->locationsListBox);
	});

	this->addButtonProxy(MouseButtonType::Left, this->listDownButton.getRect(),
		[this]()
	{
		this->listDownButton.click(this->locationsListBox);
	});

	// Add list box items and button proxies.
	for (int i = 0; i < static_cast<int>(this->locationsListIndices.size()); i++)
	{
		const int locationIndex = this->locationsListIndices[i];
		const LocationInstance &locationInst = provinceInst.getLocationInstance(locationIndex);
		const int locationDefIndex = locationInst.getLocationDefIndex();
		const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);
		std::string locationName = locationInst.getName(locationDef);

		this->locationsListBox.add(std::move(locationName));
		this->locationsListBox.setCallback(i, [this, &game, i]()
		{
			DebugAssertIndex(this->locationsListIndices, i);
			const int locationsListIndex = this->locationsListIndices[i];
			ProvinceSearchUiController::onListLocationSelected(game, *this, locationsListIndex);
		});

		auto rectFunc = [this, i]()
		{
			return this->locationsListBox.getItemGlobalRect(i);
		};

		auto callback = [this, i]()
		{
			const ListBoxItemCallback &itemCallback = this->locationsListBox.getCallback(i);
			itemCallback();
		};

		this->addButtonProxy(MouseButtonType::Left, rectFunc, callback);
	}
}

void ProvinceSearchSubPanel::tick(double dt)
{
	// @todo: eventually blink text input cursor in text entry, and listen for scrolling in list box.
	static_cast<void>(dt);
}
