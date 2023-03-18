#include <algorithm>

#include "CommonUiView.h"
#include "ProvinceMapPanel.h"
#include "ProvinceMapUiController.h"
#include "ProvinceMapUiModel.h"
#include "ProvinceMapUiView.h"
#include "ProvinceSearchSubPanel.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/TextureManager.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../Input/TextEvents.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/CursorData.h"
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
	auto &renderer = game.getRenderer();
	const auto &fontLibrary = game.getFontLibrary();

	// Don't initialize the locations list box until it's reached, since its contents
	// may depend on the search results.
	const std::string textTitleText = ProvinceSearchUiModel::getTitleText(game);
	const TextBox::InitInfo textTitleTextBoxInitInfo =
		ProvinceSearchUiView::getTitleTextBoxInitInfo(textTitleText, fontLibrary);
	if (!this->textTitleTextBox.init(textTitleTextBoxInitInfo, textTitleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	const TextBox::InitInfo textEntryTextBoxInitInfo = ProvinceSearchUiView::getTextEntryTextBoxInitInfo(fontLibrary);
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
			auto &inputManager = game.getInputManager();
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
			const Int2 classicPosition = game.getRenderer().nativeToOriginal(position);
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

	this->addTextInputListener([this](const std::string_view &text)
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

	auto &textureManager = game.getTextureManager();
	const UiTextureID parchmentTextureID = ProvinceSearchUiView::allocParchmentTexture(textureManager, renderer);
	this->parchmentTextureRef.init(parchmentTextureID, renderer);

	UiDrawCall::ActiveFunc textEntryActiveFunc = [this]()
	{
		return this->mode == ProvinceSearchUiModel::Mode::TextEntry;
	};

	this->addDrawCall(
		[this]() { return this->parchmentTextureRef.get(); },
		[]() { return Int2((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, (ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1); },
		[]() { return Int2(ProvinceSearchUiView::TextureWidth, ProvinceSearchUiView::TextureHeight); },
		[]() { return PivotType::Middle; },
		textEntryActiveFunc);

	const Rect &textTitleTextBoxRect = this->textTitleTextBox.getRect();
	this->addDrawCall(
		[this]() { return this->textTitleTextBox.getTextureID(); },
		[textTitleTextBoxRect]() { return textTitleTextBoxRect.getTopLeft(); },
		[textTitleTextBoxRect]() { return Int2(textTitleTextBoxRect.getWidth(), textTitleTextBoxRect.getHeight()); },
		[]() { return PivotType::TopLeft; },
		textEntryActiveFunc);

	const Rect &textEntryTextBoxRect = this->textEntryTextBox.getRect();
	this->addDrawCall(
		[this]() { return this->textEntryTextBox.getTextureID(); },
		[textEntryTextBoxRect]() { return textEntryTextBoxRect.getTopLeft(); },
		[textEntryTextBoxRect]() { return Int2(textEntryTextBoxRect.getWidth(), textEntryTextBoxRect.getHeight()); },
		[]() { return PivotType::TopLeft; },
		textEntryActiveFunc);

	// @todo: draw blinking cursor for text entry

	const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const UiTextureID listBackgroundTextureID = ProvinceSearchUiView::allocListBackgroundTexture(
		provinceID, binaryAssetLibrary, textureManager, renderer);
	listBackgroundTextureRef.init(listBackgroundTextureID, renderer);

	UiDrawCall::ActiveFunc listActiveFunc = [this]()
	{
		return this->mode == ProvinceSearchUiModel::Mode::List;
	};

	this->addDrawCall(
		[this]() { return this->listBackgroundTextureRef.get(); },
		[]() { return Int2(ProvinceSearchUiView::ListTextureX, ProvinceSearchUiView::ListTextureY); },
		[this]() { return Int2(this->listBackgroundTextureRef.getWidth(), this->listBackgroundTextureRef.getHeight()); },
		[]() { return PivotType::TopLeft; },
		listActiveFunc);

	UiDrawCall::PositionFunc listBoxPositionFunc = [this]()
	{
		const Rect &locationsListBoxRect = this->locationsListBox.getRect();
		return locationsListBoxRect.getTopLeft();
	};

	UiDrawCall::SizeFunc listBoxSizeFunc = [this]()
	{
		// Have to get the size dynamically due to the list not being initialized or populated yet.
		const Rect &locationsListBoxRect = this->locationsListBox.getRect();
		return Int2(locationsListBoxRect.getWidth(), locationsListBoxRect.getHeight());
	};

	this->addDrawCall(
		[this]() { return this->locationsListBox.getTextureID(); },
		listBoxPositionFunc,
		listBoxSizeFunc,
		[]() { return PivotType::TopLeft; },
		listActiveFunc);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), PivotType::TopLeft, listActiveFunc);

	this->provinceMapPanel = &provinceMapPanel;
	this->mode = ProvinceSearchUiModel::Mode::TextEntry;
	this->provinceID = provinceID;

	// Start with text input enabled.
	auto &inputManager = game.getInputManager();
	inputManager.setTextInputMode(true);

	return true;
}

void ProvinceSearchSubPanel::initLocationsList()
{
	// @todo: move the locationNames into UiModel
	auto &game = this->getGame();
	auto &gameState = game.getGameState();
	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
	const WorldMapInstance &worldMapInst = gameState.getWorldMapInstance();
	const ProvinceInstance &provinceInst = worldMapInst.getProvinceInstance(this->provinceID);
	const int provinceDefIndex = provinceInst.getProvinceDefIndex();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceDefIndex);

	this->locationsListBox.init(ProvinceSearchUiView::ListBoxRect,
		ProvinceSearchUiView::makeListBoxProperties(game.getFontLibrary()), game.getRenderer());

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
			const ListBox::ItemCallback &itemCallback = this->locationsListBox.getCallback(i);
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
