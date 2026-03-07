#include "FastTravelSubPanel.h"
#include "ProvinceMapPanel.h"
#include "ProvinceMapUiController.h"
#include "ProvinceMapUiModel.h"
#include "ProvinceMapUiState.h"
#include "ProvinceMapUiView.h"
#include "WorldMapPanel.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../UI/FontLibrary.h"
#include "../UI/TextEntry.h"
#include "../WorldMap/ArenaLocationUtils.h"

#include "components/utilities/String.h"

namespace
{
	constexpr char ContextName_TextPopUp[] = "ProvinceMapTextPopUp";
	constexpr char ContextName_SearchInputPopUp[] = "ProvinceMapSearchInputPopUp";
	constexpr char ContextName_SearchResultsPopUp[] = "ProvinceMapSearchResultsPopUp";

	constexpr char ElementName_LocationTextBox[] = "ProvinceMapLocationTextBox";

	constexpr char ElementName_SearchInputTextEntryTextBox[] = "ProvinceMapSearchInputTextEntryTextBox";
	constexpr char ElementName_SearchResultsListBox[] = "ProvinceMapSearchResultsListBox";

	constexpr MouseButtonTypeFlags PopUpMouseButtonTypeFlags = MouseButtonType::Left | MouseButtonType::Right;

	std::string GetLocationIconImageElementName(const LocationDefinition &locationDef)
	{
		char elementName[96];
		std::snprintf(elementName, sizeof(elementName), "ProvinceMapLocationIconImage %s(%d, %d)", locationDef.getName().c_str(), locationDef.getScreenX(), locationDef.getScreenY());
		return elementName;
	}

	std::string GetLocationIconHighlightImageElementName(const LocationDefinition &locationDef)
	{
		char elementName[96];
		std::snprintf(elementName, sizeof(elementName), "ProvinceMapLocationIconHighlightImage %s(%d, %d)", locationDef.getName().c_str(), locationDef.getScreenX(), locationDef.getScreenY());
		return elementName;
	}
}

ProvinceMapLocationTextures::ProvinceMapLocationTextures()
{
	this->textureID = -1;
	this->playerCurrentTextureID = -1;
	this->travelDestinationTextureID = -1;
}

void ProvinceMapLocationTextures::init(UiTextureID textureID, UiTextureID playerCurrentTextureID, UiTextureID travelDestinationTextureID, Renderer &renderer)
{
	this->textureID = textureID;
	this->playerCurrentTextureID = playerCurrentTextureID;
	this->travelDestinationTextureID = travelDestinationTextureID;
}

void ProvinceMapLocationTextures::free(Renderer &renderer)
{
	if (this->textureID >= 0)
	{
		renderer.freeUiTexture(this->textureID);
		this->textureID = -1;
	}

	if (this->playerCurrentTextureID >= 0)
	{
		renderer.freeUiTexture(this->playerCurrentTextureID);
		this->playerCurrentTextureID = -1;
	}

	if (this->travelDestinationTextureID >= 0)
	{
		renderer.freeUiTexture(this->travelDestinationTextureID);
		this->travelDestinationTextureID = -1;
	}
}

ProvinceMapUiState::ProvinceMapUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->textPopUpContextInstID = -1;
	this->searchInputPopUpContextInstID = -1;
	this->searchResultsPopUpContextInstID = -1;
	this->backgroundTextureID = -1;
	this->searchInputImageTextureID = -1;
	this->provinceID = -1;
	this->hoveredLocationID = -1;
}

void ProvinceMapUiState::init(Game &game)
{
	DebugAssert(this->provinceID >= 0); // Must be set by world map logic already.
	this->game = &game;

	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	this->backgroundTextureID = ProvinceMapUiView::allocBackgroundTexture(this->provinceID, binaryAssetLibrary, textureManager, renderer);
	this->searchInputImageTextureID = ProvinceSearchUiView::allocParchmentTexture(textureManager, renderer);

	this->blinkState.init(ProvinceMapUiView::BlinkPeriodSeconds, true);
	this->hoveredLocationID = -1;
}

void ProvinceMapUiState::freeTextures(Renderer &renderer)
{
	if (this->backgroundTextureID >= 0)
	{
		renderer.freeUiTexture(this->backgroundTextureID);
		this->backgroundTextureID = -1;
	}

	if (this->searchInputImageTextureID >= 0)
	{
		renderer.freeUiTexture(this->searchInputImageTextureID);
		this->searchInputImageTextureID = -1;
	}

	this->cityStateTextures.free(renderer);
	this->townTextures.free(renderer);
	this->villageTextures.free(renderer);
	this->dungeonTextures.free(renderer);
	this->staffDungeonTextures.free(renderer);
}

void ProvinceMapUI::create(Game &game)
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(ProvinceMapUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	UiElementInitInfo backgroundImageElementInitInfo;
	backgroundImageElementInitInfo.name = "ProvinceMapBackgroundImage";
	uiManager.createImage(backgroundImageElementInitInfo, state.backgroundTextureID, state.contextInstID, renderer);

	uiManager.addMouseMotionListener(ProvinceMapUI::onMouseMotion, ProvinceMapUI::ContextName, inputManager);

	UiContextInitInfo textPopUpContextInitInfo;
	textPopUpContextInitInfo.name = ContextName_TextPopUp;
	textPopUpContextInitInfo.drawOrder = 1;
	state.textPopUpContextInstID = uiManager.createContext(textPopUpContextInitInfo);

	UiContextInitInfo searchInputPopUpContextInitInfo;
	searchInputPopUpContextInitInfo.name = ContextName_SearchInputPopUp;
	searchInputPopUpContextInitInfo.drawOrder = 1;
	state.searchInputPopUpContextInstID = uiManager.createContext(searchInputPopUpContextInitInfo);

	UiElementInitInfo searchInputImageElementInitInfo;
	searchInputImageElementInitInfo.name = "ProvinceMapSearchInputImage";
	searchInputImageElementInitInfo.position = ProvinceSearchUiView::TextureCenter;
	searchInputImageElementInitInfo.pivotType = UiPivotType::Middle;
	uiManager.createImage(searchInputImageElementInitInfo, state.searchInputImageTextureID, state.searchInputPopUpContextInstID, renderer);

	UiElementInitInfo searchInputTitleTextBoxElementInitInfo;
	searchInputTitleTextBoxElementInitInfo.name = "ProvinceMapSearchInputTitleTextBox";
	searchInputTitleTextBoxElementInitInfo.position = Int2(ProvinceSearchUiView::TitleTextBoxX, ProvinceSearchUiView::TitleTextBoxY);
	searchInputTitleTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo searchInputTitleTextBoxInitInfo;
	searchInputTitleTextBoxInitInfo.text = ProvinceSearchUiModel::getTitleText(game);
	searchInputTitleTextBoxInitInfo.fontName = ProvinceSearchUiView::TitleFontName;
	searchInputTitleTextBoxInitInfo.defaultColor = ProvinceSearchUiView::TitleColor;
	uiManager.createTextBox(searchInputTitleTextBoxElementInitInfo, searchInputTitleTextBoxInitInfo, state.searchInputPopUpContextInstID, renderer);

	UiElementInitInfo searchInputTextEntryTextBoxElementInitInfo;
	searchInputTextEntryTextBoxElementInitInfo.name = ElementName_SearchInputTextEntryTextBox;
	searchInputTextEntryTextBoxElementInitInfo.position = ProvinceSearchUiView::DefaultTextCursorPosition;
	searchInputTextEntryTextBoxElementInitInfo.drawOrder = 2;

	UiTextBoxInitInfo searchInputTextEntryTextBoxInitInfo;
	searchInputTextEntryTextBoxInitInfo.worstCaseText = TextRenderUtils::makeWorstCaseText(ProvinceSearchUiModel::MaxNameLength);
	searchInputTextEntryTextBoxInitInfo.fontName = ProvinceSearchUiView::TextEntryFontName;
	searchInputTextEntryTextBoxInitInfo.defaultColor = ProvinceSearchUiView::TextEntryColor;
	uiManager.createTextBox(searchInputTextEntryTextBoxElementInitInfo, searchInputTextEntryTextBoxInitInfo, state.searchInputPopUpContextInstID, renderer);

	uiManager.addTextInputListener(ProvinceMapUI::onSearchInputTextInput, ContextName_SearchInputPopUp, inputManager);
	uiManager.addInputActionListener(InputActionName::Accept, ProvinceMapUI::onSearchInputAcceptInputAction, ContextName_SearchInputPopUp, inputManager);
	uiManager.addInputActionListener(InputActionName::Back, ProvinceMapUI::onSearchInputBackInputAction, ContextName_SearchInputPopUp, inputManager);
	uiManager.addInputActionListener(InputActionName::Backspace, ProvinceMapUI::onSearchInputBackspaceInputAction, ContextName_SearchInputPopUp, inputManager);

	UiContextInitInfo searchResultsPopUpContextInitInfo;
	searchResultsPopUpContextInitInfo.name = ContextName_SearchResultsPopUp;
	searchResultsPopUpContextInitInfo.drawOrder = 1;
	state.searchResultsPopUpContextInstID = uiManager.createContext(searchResultsPopUpContextInitInfo);

	UiElementInitInfo searchResultsImageElementInitInfo;
	searchResultsImageElementInitInfo.name = "ProvinceMapSearchResultsImage";
	searchResultsImageElementInitInfo.position = Int2(ProvinceSearchUiView::ListTextureX, ProvinceSearchUiView::ListTextureY);

	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const TextureAsset searchResultsImageTextureAsset = ProvinceSearchUiView::getListTextureAsset();
	const TextureAsset searchResultsImagePaletteTextureAsset = ProvinceSearchUiView::getListPaletteTextureAsset(binaryAssetLibrary, state.provinceID);
	const UiTextureID searchResultsImageTextureID = uiManager.getOrAddTexture(searchResultsImageTextureAsset, searchResultsImagePaletteTextureAsset, textureManager, renderer);
	uiManager.createImage(searchResultsImageElementInitInfo, searchResultsImageTextureID, state.searchResultsPopUpContextInstID, renderer);

	UiElementInitInfo searchResultsListBoxElementInitInfo;
	searchResultsListBoxElementInitInfo.name = "ProvinceMapSearchResultsListBox";
	searchResultsListBoxElementInitInfo.position = ProvinceSearchUiView::ListBoxRect.getTopLeft();
	searchResultsListBoxElementInitInfo.drawOrder = 1;

	const ListBoxProperties searchResultsListBoxProperties = ProvinceSearchUiView::makeListBoxProperties();
	UiListBoxInitInfo searchResultsListBoxInitInfo;
	searchResultsListBoxInitInfo.textureWidth = searchResultsListBoxProperties.textureGenInfo.width;
	searchResultsListBoxInitInfo.textureHeight = searchResultsListBoxProperties.textureGenInfo.height;
	searchResultsListBoxInitInfo.itemPixelSpacing = searchResultsListBoxProperties.itemSpacing;
	searchResultsListBoxInitInfo.fontName = ArenaFontName::Arena;
	searchResultsListBoxInitInfo.defaultTextColor = searchResultsListBoxProperties.defaultColor;
	uiManager.createListBox(searchResultsListBoxElementInitInfo, searchResultsListBoxInitInfo, state.searchResultsPopUpContextInstID, renderer);

	UiElementInitInfo searchResultsListBoxUpButtonElementInitInfo;
	searchResultsListBoxUpButtonElementInitInfo.name = "ProvinceMapSearchResultsListBoxUpButton";
	searchResultsListBoxUpButtonElementInitInfo.position = ProvinceSearchUiView::ListUpButtonCenterPoint;
	searchResultsListBoxUpButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	searchResultsListBoxUpButtonElementInitInfo.size = Int2(ProvinceSearchUiView::ListUpButtonWidth, ProvinceSearchUiView::ListUpButtonHeight);
	searchResultsListBoxUpButtonElementInitInfo.pivotType = UiPivotType::Middle;

	UiButtonInitInfo searchResultsListBoxUpButtonInitInfo;
	searchResultsListBoxUpButtonInitInfo.callback = ProvinceMapUI::onSearchResultsListUpButtonSelected;
	uiManager.createButton(searchResultsListBoxUpButtonElementInitInfo, searchResultsListBoxUpButtonInitInfo, state.searchResultsPopUpContextInstID);

	UiElementInitInfo searchResultsListBoxDownButtonElementInitInfo;
	searchResultsListBoxDownButtonElementInitInfo.name = "ProvinceMapSearchResultsListBoxDownButton";
	searchResultsListBoxDownButtonElementInitInfo.position = ProvinceSearchUiView::ListDownButtonCenterPoint;
	searchResultsListBoxDownButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	searchResultsListBoxDownButtonElementInitInfo.size = Int2(ProvinceSearchUiView::ListDownButtonWidth, ProvinceSearchUiView::ListDownButtonHeight);
	searchResultsListBoxDownButtonElementInitInfo.pivotType = UiPivotType::Middle;

	UiButtonInitInfo searchResultsListBoxDownButtonInitInfo;
	searchResultsListBoxUpButtonInitInfo.callback = ProvinceMapUI::onSearchResultsListDownButtonSelected;
	uiManager.createButton(searchResultsListBoxDownButtonElementInitInfo, searchResultsListBoxUpButtonInitInfo, state.searchResultsPopUpContextInstID);

	uiManager.addMouseScrollChangedListener(ProvinceMapUI::onSearchResultsMouseScrollChanged, ContextName_SearchResultsPopUp, inputManager);
	uiManager.addInputActionListener(InputActionName::Back, ProvinceMapUI::onSearchResultsBackInputAction, ContextName_SearchResultsPopUp, inputManager);

	uiManager.setContextEnabled(state.textPopUpContextInstID, false);
	uiManager.setContextEnabled(state.searchInputPopUpContextInstID, false);
	uiManager.setContextEnabled(state.searchResultsPopUpContextInstID, false);

	ProvinceMapUI::initLocationIconUI(state.provinceID);

	const Window &window = game.window;
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = window.nativeToOriginal(mousePosition);
	ProvinceMapUI::updateHoveredLocationID(originalPosition);

	inputManager.setInputActionMapActive(InputActionMapName::WorldMap, true);
}

void ProvinceMapUI::destroy()
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	Renderer &renderer = game.renderer;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, renderer);
		state.contextInstID = -1;
	}

	if (state.textPopUpContextInstID >= 0)
	{
		uiManager.freeContext(state.textPopUpContextInstID, inputManager, renderer);
		state.textPopUpContextInstID = -1;
	}

	if (state.searchInputPopUpContextInstID >= 0)
	{
		uiManager.freeContext(state.searchInputPopUpContextInstID, inputManager, renderer);
		state.searchInputPopUpContextInstID = -1;
	}

	if (state.searchResultsPopUpContextInstID >= 0)
	{
		uiManager.freeContext(state.searchResultsPopUpContextInstID, inputManager, renderer);
		state.searchResultsPopUpContextInstID = -1;
	}

	state.freeTextures(renderer);

	inputManager.setInputActionMapActive(InputActionMapName::WorldMap, false);
}

void ProvinceMapUI::update(double dt)
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	// Update hovered location text box position.
	// Can't use the text box dimensions with clamping since it's allocated for worst-case location name now.
	const FontLibrary &fontLibrary = FontLibrary::getInstance();
	const std::string &fontName = ProvinceMapUiView::LocationFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get hovered location font name \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const std::string locationName = ProvinceMapUiModel::getLocationName(game, state.provinceID, state.hoveredLocationID);
	TextRenderShadowInfo shadowInfo;
	shadowInfo.init(ProvinceMapUiView::LocationTextShadowOffsetX, ProvinceMapUiView::LocationTextShadowOffsetY, ProvinceMapUiView::LocationTextShadowColor);

	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(locationName, fontDef, shadowInfo);
	const Int2 locationCenter = ProvinceMapUiView::getLocationCenterPoint(game, state.provinceID, state.hoveredLocationID);
	const Int2 locationTextBoxCenter = locationCenter - Int2(0, 10);
	const Rect textBoxRect(locationTextBoxCenter, textureGenInfo.width, textureGenInfo.height);
	const Int2 locationTextBoxClampedCenter = ProvinceMapUiView::getLocationTextClampedCenter(textBoxRect);
	const UiElementInstanceID locationTextBoxElementInstID = uiManager.getElementByName(ElementName_LocationTextBox);
	uiManager.setTransformPosition(locationTextBoxElementInstID, locationTextBoxClampedCenter);

	ProvinceMapUI::updateLocationHighlights();

	GameState &gameState = game.gameState;
	if (gameState.getTravelData() != nullptr)
	{
		state.blinkState.update(dt);
	}

	// @todo: draw blinking cursor for search input text entry (not sure where to put that update()).
}

void ProvinceMapUI::initLocationIconUI(int provinceID)
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();

	const TextureAsset backgroundPaletteTextureAsset = ProvinceMapUiView::getBackgroundPaletteTextureAsset(provinceID, binaryAssetLibrary);
	state.cityStateTextures.init(
		ProvinceMapUiView::allocCityStateIconTexture(ProvinceMapUiView::HighlightType::None, backgroundPaletteTextureAsset, textureManager, renderer),
		ProvinceMapUiView::allocCityStateIconTexture(ProvinceMapUiView::HighlightType::PlayerLocation, backgroundPaletteTextureAsset, textureManager, renderer),
		ProvinceMapUiView::allocCityStateIconTexture(ProvinceMapUiView::HighlightType::TravelDestination, backgroundPaletteTextureAsset, textureManager, renderer),
		renderer);
	state.townTextures.init(
		ProvinceMapUiView::allocTownIconTexture(ProvinceMapUiView::HighlightType::None, backgroundPaletteTextureAsset, textureManager, renderer),
		ProvinceMapUiView::allocTownIconTexture(ProvinceMapUiView::HighlightType::PlayerLocation, backgroundPaletteTextureAsset, textureManager, renderer),
		ProvinceMapUiView::allocTownIconTexture(ProvinceMapUiView::HighlightType::TravelDestination, backgroundPaletteTextureAsset, textureManager, renderer),
		renderer);
	state.villageTextures.init(
		ProvinceMapUiView::allocVillageIconTexture(ProvinceMapUiView::HighlightType::None, backgroundPaletteTextureAsset, textureManager, renderer),
		ProvinceMapUiView::allocVillageIconTexture(ProvinceMapUiView::HighlightType::PlayerLocation, backgroundPaletteTextureAsset, textureManager, renderer),
		ProvinceMapUiView::allocVillageIconTexture(ProvinceMapUiView::HighlightType::TravelDestination, backgroundPaletteTextureAsset, textureManager, renderer),
		renderer);
	state.dungeonTextures.init(
		ProvinceMapUiView::allocDungeonIconTexture(ProvinceMapUiView::HighlightType::None, backgroundPaletteTextureAsset, textureManager, renderer),
		ProvinceMapUiView::allocDungeonIconTexture(ProvinceMapUiView::HighlightType::PlayerLocation, backgroundPaletteTextureAsset, textureManager, renderer),
		ProvinceMapUiView::allocDungeonIconTexture(ProvinceMapUiView::HighlightType::TravelDestination, backgroundPaletteTextureAsset, textureManager, renderer),
		renderer);

	if (ProvinceMapUiView::provinceHasStaffDungeonIcon(provinceID))
	{
		state.staffDungeonTextures.init(
			ProvinceMapUiView::allocStaffDungeonIconTexture(provinceID, ProvinceMapUiView::HighlightType::None, backgroundPaletteTextureAsset, textureManager, renderer),
			ProvinceMapUiView::allocStaffDungeonIconTexture(provinceID, ProvinceMapUiView::HighlightType::PlayerLocation, backgroundPaletteTextureAsset, textureManager, renderer),
			ProvinceMapUiView::allocStaffDungeonIconTexture(provinceID, ProvinceMapUiView::HighlightType::TravelDestination, backgroundPaletteTextureAsset, textureManager, renderer),
			renderer);
	}

	GameState &gameState = game.gameState;
	const WorldMapInstance &worldMapInst = gameState.getWorldMapInstance();
	const ProvinceInstance &provinceInst = worldMapInst.getProvinceInstance(provinceID);
	const int provinceDefIndex = provinceInst.getProvinceDefIndex();
	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceDefIndex);
	const ProvinceDefinition &playerProvinceDef = gameState.getProvinceDefinition();

	for (int i = 0; i < provinceInst.getLocationCount(); i++)
	{
		const LocationInstance &locationInst = provinceInst.getLocationInstance(i);
		if (!locationInst.isVisible())
		{
			continue;
		}

		const int locationDefIndex = locationInst.getLocationDefIndex();
		const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);

		const ProvinceMapLocationTextures *locationTexturesPtr = [&state, &locationDef]() -> const ProvinceMapLocationTextures*
		{
			const LocationDefinitionType locationDefType = locationDef.getType();
			if (locationDefType == LocationDefinitionType::City)
			{
				const LocationCityDefinition &cityDef = locationDef.getCityDefinition();
				const ArenaCityType cityType = cityDef.type;
				switch (cityType)
				{
				case ArenaCityType::CityState:
					return &state.cityStateTextures;
				case ArenaCityType::Town:
					return &state.townTextures;
				case ArenaCityType::Village:
					return &state.villageTextures;
				default:
					DebugLogErrorFormat("Unhandled city type \"%d\".", cityType);
					return nullptr;
				}
			}
			else if (locationDefType == LocationDefinitionType::Dungeon)
			{
				return &state.dungeonTextures;
			}
			else if (locationDefType == LocationDefinitionType::MainQuestDungeon)
			{
				const LocationMainQuestDungeonDefinition &mainQuestDungeonDef = locationDef.getMainQuestDungeonDefinition();
				const LocationMainQuestDungeonDefinitionType mainQuestDungeonType = mainQuestDungeonDef.type;
				switch (mainQuestDungeonType)
				{
				case LocationMainQuestDungeonDefinitionType::Start:
				case LocationMainQuestDungeonDefinitionType::Map:
					return &state.dungeonTextures;
				case LocationMainQuestDungeonDefinitionType::Staff:
					return &state.staffDungeonTextures;
				default:
					DebugLogErrorFormat("Unhandled main quest dungeon type \"%d\".", mainQuestDungeonType);
					return nullptr;
				}
			}
			else
			{
				DebugLogErrorFormat("Unhandled location definition type \"%d\".", locationDefType);
				return nullptr;
			}
		}();

		UiElementInitInfo locationIconImageElementInitInfo;
		locationIconImageElementInitInfo.name = GetLocationIconImageElementName(locationDef);
		locationIconImageElementInitInfo.position = Int2(locationDef.getScreenX(), locationDef.getScreenY());
		locationIconImageElementInitInfo.pivotType = UiPivotType::Middle;
		locationIconImageElementInitInfo.drawOrder = 1;
		uiManager.createImage(locationIconImageElementInitInfo, locationTexturesPtr->textureID, state.contextInstID, renderer);

		const ProvinceMapUiView::HighlightType locationIconHighlightType = [provinceID, &state, &gameState, &provinceDef, &playerProvinceDef, i, &locationDef]()
		{
			const LocationDefinition &playerLocationDef = gameState.getLocationDefinition();
			if (provinceDef.matches(playerProvinceDef) && locationDef.matches(playerLocationDef))
			{
				return ProvinceMapUiView::HighlightType::PlayerLocation;
			}
			else
			{
				// If there is a currently-selected destination in this province, draw its blinking highlight
				// if within the "blink on" interval.
				const ProvinceMapUiModel::TravelData *travelDataPtr = gameState.getTravelData();
				if (travelDataPtr != nullptr)
				{
					const ProvinceMapUiModel::TravelData &travelData = *travelDataPtr;
					if ((travelData.provinceID == provinceID) && (travelData.locationID == i))
					{
						return ProvinceMapUiView::HighlightType::TravelDestination;
					}
				}
			}

			return ProvinceMapUiView::HighlightType::None;
		}();

		const UiTextureID highlightLocationTextureID = [locationIconHighlightType, locationTexturesPtr]()
		{
			switch (locationIconHighlightType)
			{
			case ProvinceMapUiView::HighlightType::None:
			case ProvinceMapUiView::HighlightType::TravelDestination:
				return locationTexturesPtr->travelDestinationTextureID;
			case ProvinceMapUiView::HighlightType::PlayerLocation:
				return locationTexturesPtr->playerCurrentTextureID;
			default:
				DebugUnhandledReturnMsg(UiTextureID, std::to_string(static_cast<int>(locationIconHighlightType)));
			}
		}();

		UiElementInitInfo locationIconHighlightImageElementInitInfo;
		locationIconHighlightImageElementInitInfo.name = GetLocationIconHighlightImageElementName(locationDef);
		locationIconHighlightImageElementInitInfo.position = locationIconImageElementInitInfo.position;
		locationIconHighlightImageElementInitInfo.pivotType = locationIconImageElementInitInfo.pivotType;
		locationIconHighlightImageElementInitInfo.drawOrder = 2;
		uiManager.createImage(locationIconHighlightImageElementInitInfo, highlightLocationTextureID, state.contextInstID, renderer);
	}
}

void ProvinceMapUI::updateHoveredLocationID(Int2 originalPosition)
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();

	const WorldMapInstance &worldMapInst = gameState.getWorldMapInstance();
	const ProvinceInstance &provinceInst = worldMapInst.getProvinceInstance(state.provinceID);
	const int provinceDefIndex = provinceInst.getProvinceDefIndex();
	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceDefIndex);

	std::optional<Int2> closestPosition;
	auto closerThanCurrentClosest = [&originalPosition, &closestPosition](const Int2 &point)
	{
		if (!closestPosition.has_value())
		{
			return true;
		}

		const Int2 diff = point - originalPosition;
		const Int2 closestDiff = *closestPosition - originalPosition;
		const int distanceSqr = (diff.x * diff.x) + (diff.y * diff.y);
		const int closestDistanceSqr = (closestDiff.x * closestDiff.x) + (closestDiff.y * closestDiff.y);
		return distanceSqr < closestDistanceSqr;
	};

	// Look through all visible locations to find the one closest to the mouse.
	std::optional<int> closestIndex;
	for (int i = 0; i < provinceInst.getLocationCount(); i++)
	{
		const LocationInstance &locationInst = provinceInst.getLocationInstance(i);
		if (locationInst.isVisible())
		{
			const int locationDefIndex = locationInst.getLocationDefIndex();
			const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);
			const Int2 point(locationDef.getScreenX(), locationDef.getScreenY());

			if (closerThanCurrentClosest(point))
			{
				closestPosition = point;
				closestIndex = i;
			}
		}
	}

	if (!closestIndex.has_value())
	{
		DebugLogErrorFormat("No closest location ID found at UI position (%d, %d).", originalPosition.x, originalPosition.y);
		return;
	}

	if (state.hoveredLocationID != *closestIndex)
	{
		state.hoveredLocationID = *closestIndex;

		const std::string locationName = ProvinceMapUiModel::getLocationName(game, state.provinceID, state.hoveredLocationID);

		UiManager &uiManager = game.uiManager;
		const UiElementInstanceID hoveredLocationTextBoxElementInstID = uiManager.getElementByName("ProvinceMapLocationTextBox");
		uiManager.setTextBoxText(hoveredLocationTextBoxElementInstID, locationName.c_str());
	}
}

void ProvinceMapUI::updateLocationHighlights()
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	GameState &gameState = game.gameState;
	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(state.provinceID);
	const ProvinceDefinition &playerProvinceDef = gameState.getProvinceDefinition();
	const WorldMapInstance &worldMapInst = gameState.getWorldMapInstance();
	const ProvinceInstance &provinceInst = worldMapInst.getProvinceInstance(state.provinceID);
	for (int i = 0; i < provinceDef.getLocationCount(); i++)
	{
		const LocationDefinition &locationDef = provinceDef.getLocationDef(i);
		const LocationInstance &locationInst = provinceInst.getLocationInstance(i);
		if (!locationInst.isVisible())
		{
			continue;
		}

		bool isHighlightVisible = false;
		const LocationDefinition &playerLocationDef = gameState.getLocationDefinition();
		if (provinceDef.matches(playerProvinceDef) && locationDef.matches(playerLocationDef))
		{
			isHighlightVisible = true;
		}
		else
		{
			// If there is a currently-selected destination in this province, draw its blinking highlight
			// if within the "blink on" interval.
			const ProvinceMapUiModel::TravelData *travelDataPtr = gameState.getTravelData();
			if (travelDataPtr != nullptr)
			{
				const ProvinceMapUiModel::TravelData &travelData = *travelDataPtr;
				if ((travelData.provinceID == state.provinceID) && (travelData.locationID == i))
				{
					// See if the blink period percent lies within the "on" percent. Use less-than to compare them
					// so the on-state appears before the off-state.
					if (state.blinkState.getPercent() < ProvinceMapUiView::BlinkPeriodPercentOn)
					{
						isHighlightVisible = true;
					}
				}
			}
		}

		const std::string locationIconHighlightImageElementName = GetLocationIconHighlightImageElementName(locationDef);
		const UiElementInstanceID locationHighlightIconImageElementInstID = uiManager.getElementByName(locationIconHighlightImageElementName.c_str());
		uiManager.setElementActive(locationHighlightIconImageElementInstID, isHighlightVisible);
	}
}

void ProvinceMapUI::trySelectLocation(int selectedLocationID)
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();

	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
	const ProvinceDefinition &currentProvinceDef = gameState.getProvinceDefinition();
	const LocationDefinition &currentLocationDef = gameState.getLocationDefinition();
	const ProvinceDefinition &selectedProvinceDef = worldMapDef.getProvinceDef(state.provinceID);
	const LocationDefinition &selectedLocationDef = selectedProvinceDef.getLocationDef(selectedLocationID);

	const bool matchesPlayerLocation = selectedProvinceDef.matches(currentProvinceDef) && selectedLocationDef.matches(currentLocationDef);
	if (!matchesPlayerLocation)
	{
		auto makeGlobalPoint = [](const LocationDefinition &locationDef, const ProvinceDefinition &provinceDef)
		{
			const Int2 localPoint(locationDef.getScreenX(), locationDef.getScreenY());
			return ArenaLocationUtils::getGlobalPoint(localPoint, provinceDef.getGlobalRect());
		};

		const Int2 srcGlobalPoint = makeGlobalPoint(currentLocationDef, currentProvinceDef);
		const Int2 dstGlobalPoint = makeGlobalPoint(selectedLocationDef, selectedProvinceDef);
		const Date &currentDate = gameState.getDate();

		// Use a copy of the RNG so repeating the travel pop-up doesn't cause different day amounts.
		ArenaRandom tempRandom = game.arenaRandom;

		const int travelDays = ArenaLocationUtils::getTravelDays(srcGlobalPoint, dstGlobalPoint, currentDate.getMonth(), gameState.getWorldMapWeathers(), tempRandom, binaryAssetLibrary);

		gameState.setTravelData(ProvinceMapUiModel::TravelData(selectedLocationID, state.provinceID, travelDays));
		state.blinkState.reset();
		ProvinceMapUI::updateLocationHighlights();

		const std::string travelText = ProvinceMapUiModel::makeTravelText(game, state.provinceID, currentLocationDef, currentProvinceDef, selectedLocationID);
		ProvinceMapUI::showTextPopUp(travelText.c_str());
	}
	else
	{
		// Cannot travel to the player's current location. Create an error pop-up.
		const LocationInstance &currentLocationInst = gameState.getLocationInstance();
		const std::string &currentLocationName = currentLocationInst.getName(currentLocationDef);

		const std::string errorText = ProvinceMapUiModel::makeAlreadyAtLocationText(game, currentLocationName);
		ProvinceMapUI::showTextPopUp(errorText.c_str());
	}
}

void ProvinceMapUI::beginFastTravel()
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
	game.pushSubPanel<FastTravelSubPanel>(); // @todo this should set some isFastTraveling bool in WorldMapUI
	game.setPanel<WorldMapPanel>();
}

void ProvinceMapUI::showTextPopUp(const char *str)
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	uiManager.clearContextElements(state.textPopUpContextInstID, inputManager, renderer);

	UiElementInitInfo textPopUpTextBoxElementInitInfo;
	textPopUpTextBoxElementInitInfo.name = "ProvinceMapTextPopUpTextBox";
	textPopUpTextBoxElementInitInfo.position = ProvinceMapUiView::TextPopUpCenterPoint;
	textPopUpTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	textPopUpTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo textPopUpTextBoxInitInfo;
	textPopUpTextBoxInitInfo.text = str;
	textPopUpTextBoxInitInfo.fontName = ProvinceMapUiView::TextPopUpFontName;
	textPopUpTextBoxInitInfo.defaultColor = ProvinceMapUiView::TextPopUpTextColor;
	textPopUpTextBoxInitInfo.alignment = ProvinceMapUiView::TextPopUpTextAlignment;
	textPopUpTextBoxInitInfo.lineSpacing = ProvinceMapUiView::TextPopUpLineSpacing;
	const UiElementInstanceID textPopUpTextBoxElementInstID = uiManager.createTextBox(textPopUpTextBoxElementInitInfo, textPopUpTextBoxInitInfo, state.textPopUpContextInstID, renderer);
	const Rect textPopUpTextBoxRect = uiManager.getTransformGlobalRect(textPopUpTextBoxElementInstID);

	UiElementInitInfo textPopUpImageElementInitInfo;
	textPopUpImageElementInitInfo.name = "ProvinceMapTextPopUpImage";
	textPopUpImageElementInitInfo.position = ProvinceMapUiView::TextPopUpTextureCenterPoint;
	textPopUpImageElementInitInfo.pivotType = UiPivotType::Middle;
	textPopUpImageElementInitInfo.drawOrder = 0;

	const int textPopUpImageTextureWidth = ProvinceMapUiView::getTextPopUpTextureWidth(textPopUpTextBoxRect.width);
	const int textPopUpImageTextureHeight = ProvinceMapUiView::getTextPopUpTextureHeight(textPopUpTextBoxRect.height);
	const UiTextureID textPopUpImageTextureID = uiManager.getOrAddTexture(ProvinceMapUiView::TextPopUpTexturePatternType, textPopUpImageTextureWidth, textPopUpImageTextureHeight, textureManager, renderer);
	uiManager.createImage(textPopUpImageElementInitInfo, textPopUpImageTextureID, state.textPopUpContextInstID, renderer);

	UiElementInitInfo textPopUpBackButtonElementInitInfo;
	textPopUpBackButtonElementInitInfo.name = "ProvinceMapTextPopUpBackButton";
	textPopUpBackButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	textPopUpBackButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	textPopUpBackButtonElementInitInfo.drawOrder = 2;

	auto popUpButtonCallback = [](MouseButtonType)
	{
		ProvinceMapUiState &state = ProvinceMapUI::state;
		Game &game = *state.game;
		InputManager &inputManager = game.inputManager;
		UiManager &uiManager = game.uiManager;
		uiManager.setContextEnabled(state.textPopUpContextInstID, false);

		ProvinceMapUI::onPauseChanged(false);
	};

	UiButtonInitInfo textPopUpBackButtonInitInfo;
	textPopUpBackButtonInitInfo.mouseButtonFlags = PopUpMouseButtonTypeFlags;
	textPopUpBackButtonInitInfo.callback = popUpButtonCallback;
	uiManager.createButton(textPopUpBackButtonElementInitInfo, textPopUpBackButtonInitInfo, state.textPopUpContextInstID);

	auto inputActionCallback = [popUpButtonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			popUpButtonCallback(MouseButtonType::Left);
		}
	};

	uiManager.addInputActionListener(InputActionName::Back, inputActionCallback, ContextName_TextPopUp, inputManager);
	uiManager.setContextEnabled(state.textPopUpContextInstID, true);

	ProvinceMapUI::onPauseChanged(true);
}

void ProvinceMapUI::onPauseChanged(bool paused)
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	if (!paused)
	{
		// Make sure the hovered location matches where the pointer is now since mouse motion events
		// aren't processed while this panel is paused.
		const Window &window = game.window;
		const InputManager &inputManager = game.inputManager;
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPosition = window.nativeToOriginal(mousePosition);
		ProvinceMapUI::updateHoveredLocationID(originalPosition);
	}

	const UiElementInstanceID locationTextBoxElementInstID = uiManager.getElementByName(ElementName_LocationTextBox);
	uiManager.setElementActive(locationTextBoxElementInstID, !paused);
}

void ProvinceMapUI::onSearchResultsListLocationSelected(int locationID)
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.setContextEnabled(state.searchResultsPopUpContextInstID, false);
	ProvinceMapUI::trySelectLocation(locationID);
}

void ProvinceMapUI::onSearchResultsListUpButtonSelected(MouseButtonType mouseButtonType)
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID listBoxElementInstID = uiManager.getElementByName(ElementName_SearchResultsListBox);
	uiManager.scrollListBoxUp(listBoxElementInstID);
}

void ProvinceMapUI::onSearchResultsListDownButtonSelected(MouseButtonType mouseButtonType)
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID listBoxElementInstID = uiManager.getElementByName(ElementName_SearchResultsListBox);
	uiManager.scrollListBoxDown(listBoxElementInstID);
}

void ProvinceMapUI::onMouseMotion(Game &game, int dx, int dy)
{
	const Window &window = game.window;
	const InputManager &inputManager = game.inputManager;
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = window.nativeToOriginal(mousePosition);
	ProvinceMapUI::updateHoveredLocationID(originalPosition);
}

void ProvinceMapUI::onFullscreenButtonSelected(MouseButtonType mouseButtonType)
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 classicPosition = game.window.nativeToOriginal(mousePosition);

	constexpr Rect searchButtonRect = ProvinceMapUiView::SearchButtonRect;
	constexpr Rect travelButtonRect = ProvinceMapUiView::TravelButtonRect;
	constexpr Rect backButtonRect = ProvinceMapUiView::BackToWorldMapRect;

	if (searchButtonRect.contains(classicPosition))
	{
		uiManager.setContextEnabled(state.searchInputPopUpContextInstID, true);

		const UiElementInstanceID textEntryTextBoxElementInstID = uiManager.getElementByName(ElementName_SearchInputTextEntryTextBox);
		uiManager.setTextBoxText(textEntryTextBoxElementInstID, "");

		inputManager.setTextInputMode(true);
		ProvinceMapUI::onPauseChanged(true);
	}
	else if (travelButtonRect.contains(classicPosition))
	{
		const GameState &gameState = game.gameState;
		const bool hasTravelData = gameState.getTravelData() != nullptr;

		if (hasTravelData)
		{
			ProvinceMapUI::beginFastTravel();
		}
		else
		{
			const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
			std::string noDestinationText = exeData.travel.noDestination;

			// Remove carriage return at end.
			noDestinationText.pop_back();
			noDestinationText = String::replace(noDestinationText, '\r', '\n');

			ProvinceMapUI::showTextPopUp(noDestinationText.c_str());
		}
	}
	else if (backButtonRect.contains(classicPosition))
	{
		game.setPanel<WorldMapPanel>();
	}
	else
	{
		// The closest location to the cursor was clicked. See if it can be set as the
		// travel destination (depending on whether the player is already there).
		ProvinceMapUI::trySelectLocation(state.hoveredLocationID);
	}
}

void ProvinceMapUI::onBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ProvinceMapUiState &state = ProvinceMapUI::state;
		Game &game = *state.game;
		game.setPanel<WorldMapPanel>();
	}
}

void ProvinceMapUI::onSearchInputTextInput(const std::string_view text)
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID textEntryTextBoxElementInstID = uiManager.getElementByName(ElementName_SearchInputTextEntryTextBox);
	std::string searchInputText = uiManager.getTextBoxText(textEntryTextBoxElementInstID);
	const bool textChanged = TextEntry::append(searchInputText, text, ProvinceSearchUiModel::isCharAllowed, ProvinceSearchUiModel::MaxNameLength);

	if (textChanged)
	{
		uiManager.setTextBoxText(textEntryTextBoxElementInstID, searchInputText.c_str());
	}
}

void ProvinceMapUI::onSearchInputAcceptInputAction(const InputActionCallbackValues &values)
{	
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	uiManager.setContextEnabled(state.searchInputPopUpContextInstID, false);

	const UiElementInstanceID textEntryTextBoxElementInstID = uiManager.getElementByName(ElementName_SearchInputTextEntryTextBox);
	std::string searchInputText = uiManager.getTextBoxText(textEntryTextBoxElementInstID);

	const int *exactLocationIndex = nullptr;
	const std::vector<int> matchingLocationIndices = ProvinceSearchUiModel::getMatchingLocations(game, searchInputText, state.provinceID, &exactLocationIndex);
	const bool isExactLocationMatch = exactLocationIndex != nullptr;

	if (isExactLocationMatch)
	{
		ProvinceMapUI::trySelectLocation(*exactLocationIndex);
	}
	else
	{
		uiManager.setContextEnabled(state.searchResultsPopUpContextInstID, true);

		GameState &gameState = game.gameState;
		const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
		const WorldMapInstance &worldMapInst = gameState.getWorldMapInstance();
		const ProvinceInstance &provinceInst = worldMapInst.getProvinceInstance(state.provinceID);
		const int provinceDefIndex = provinceInst.getProvinceDefIndex();
		const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceDefIndex);

		const UiElementInstanceID listBoxElementInstID = uiManager.getElementByName(ElementName_SearchResultsListBox);
		uiManager.clearListBox(listBoxElementInstID);

		for (const int locationIndex : matchingLocationIndices)
		{
			const LocationInstance &locationInst = provinceInst.getLocationInstance(locationIndex);
			const int locationDefIndex = locationInst.getLocationDefIndex();
			const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);

			UiListBoxItem listBoxItem;
			listBoxItem.text = locationInst.getName(locationDef);
			listBoxItem.callback = [locationIndex]()
			{
				ProvinceMapUI::onSearchResultsListLocationSelected(locationIndex);
			};

			uiManager.insertBackListBoxItem(listBoxElementInstID, std::move(listBoxItem));
		}
	}

	inputManager.setTextInputMode(false);
}

void ProvinceMapUI::onSearchInputBackInputAction(const InputActionCallbackValues &values)
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	uiManager.setContextEnabled(state.searchInputPopUpContextInstID, false);
	inputManager.setTextInputMode(false);
	ProvinceMapUI::onPauseChanged(false);
}

void ProvinceMapUI::onSearchInputBackspaceInputAction(const InputActionCallbackValues &values)
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;

	if (values.performed)
	{
		UiManager &uiManager = game.uiManager;
		const UiElementInstanceID textEntryTextBoxElementInstID = uiManager.getElementByName(ElementName_SearchInputTextEntryTextBox);
		std::string searchInputText = uiManager.getTextBoxText(textEntryTextBoxElementInstID);
		const bool textChanged = TextEntry::backspace(searchInputText);

		if (textChanged)
		{
			uiManager.setTextBoxText(textEntryTextBoxElementInstID, searchInputText.c_str());
		}
	}
}

void ProvinceMapUI::onSearchResultsMouseScrollChanged(Game &game, MouseWheelScrollType type, const Int2 &position)
{
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID listBoxElementInstID = uiManager.getElementByName(ElementName_SearchResultsListBox);
	const Rect listBoxRect = uiManager.getTransformGlobalRect(listBoxElementInstID);

	const Int2 classicMousePosition = game.window.nativeToOriginal(position);
	if (listBoxRect.contains(classicMousePosition))
	{
		if (type == MouseWheelScrollType::Up)
		{
			ProvinceMapUI::onSearchResultsListUpButtonSelected(MouseButtonType::Left);
		}
		else if (type == MouseWheelScrollType::Down)
		{
			ProvinceMapUI::onSearchResultsListDownButtonSelected(MouseButtonType::Left);
		}
	}
}

void ProvinceMapUI::onSearchResultsBackInputAction(const InputActionCallbackValues &values)
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	uiManager.setContextEnabled(state.searchResultsPopUpContextInstID, false);
	ProvinceMapUI::onPauseChanged(false);
}
