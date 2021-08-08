#include <algorithm>

#include "SDL.h"

#include "ProvinceMapPanel.h"
#include "ProvinceMapUiController.h"
#include "ProvinceMapUiModel.h"
#include "ProvinceMapUiView.h"
#include "ProvinceSearchSubPanel.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/CityDataFile.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../Input/TextEvents.h"
#include "../Media/Color.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/CursorData.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextEntry.h"

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
	this->parchment = TextureUtils::generate(
		ProvinceSearchUiView::TexturePattern,
		ProvinceSearchUiView::TextureWidth,
		ProvinceSearchUiView::TextureHeight,
		game.getTextureManager(),
		game.getRenderer());

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

	this->provinceMapPanel = &provinceMapPanel;
	this->mode = ProvinceSearchUiModel::Mode::TextEntry;
	this->provinceID = provinceID;

	// Start with text input enabled.
	SDL_StartTextInput();

	return true;
}

std::optional<CursorData> ProvinceSearchSubPanel::getCurrentCursor() const
{
	if (this->mode == ProvinceSearchUiModel::Mode::TextEntry)
	{
		// No mouse cursor when typing.
		return std::nullopt;
	}
	else
	{
		return this->getDefaultCursor();
	}
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

void ProvinceSearchSubPanel::renderTextEntry(Renderer &renderer)
{
	const int parchmentX = ProvinceSearchUiView::getTextEntryTextureX(this->parchment.getWidth());
	const int parchmentY = ProvinceSearchUiView::getTextEntryTextureY(this->parchment.getHeight());
	renderer.drawOriginal(this->parchment, parchmentX, parchmentY);

	// Draw text: title, location name.
	const Rect &titleTextBoxRect = this->textTitleTextBox.getRect();
	const Rect &entryTextBoxRect = this->textEntryTextBox.getRect();
	renderer.drawOriginal(this->textTitleTextBox.getTexture(), titleTextBoxRect.getLeft(), titleTextBoxRect.getTop());
	renderer.drawOriginal(this->textEntryTextBox.getTexture(), entryTextBoxRect.getLeft(), entryTextBoxRect.getTop());

	// @todo: draw blinking cursor.
}

void ProvinceSearchSubPanel::renderList(Renderer &renderer)
{
	// Draw list background.
	auto &game = this->getGame();
	auto &textureManager = game.getTextureManager();
	const TextureAssetReference listBackgroundPaletteTextureAssetRef =
		ProvinceSearchUiView::getListPaletteTextureAssetRef(game, this->provinceID);
	const std::optional<PaletteID> listBackgroundPaletteID =
		textureManager.tryGetPaletteID(listBackgroundPaletteTextureAssetRef);
	if (!listBackgroundPaletteID.has_value())
	{
		DebugLogError("Couldn't get list background palette ID for \"" + listBackgroundPaletteTextureAssetRef.filename + "\".");
		return;
	}
	
	const TextureAssetReference listBackgroundTextureAssetRef = ProvinceSearchUiView::getListTextureAssetRef();
	const std::optional<TextureBuilderID> listBackgroundTextureBuilderID =
		textureManager.tryGetTextureBuilderID(listBackgroundTextureAssetRef);
	if (!listBackgroundTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get list background texture builder ID for \"" + listBackgroundTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*listBackgroundTextureBuilderID, *listBackgroundPaletteID,
		ProvinceSearchUiView::ListTextureX, ProvinceSearchUiView::ListTextureY,
		textureManager);

	// Draw list box text.
	const Rect &locationsListBoxRect = this->locationsListBox.getRect();
	renderer.drawOriginal(this->locationsListBox.getTexture(),
		locationsListBoxRect.getLeft(), locationsListBoxRect.getTop());
}

void ProvinceSearchSubPanel::render(Renderer &renderer)
{
	if (this->mode == ProvinceSearchUiModel::Mode::TextEntry)
	{
		this->renderTextEntry(renderer);
	}
	else
	{
		this->renderList(renderer);
	}
}
