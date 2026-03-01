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
#include "../WorldMap/ArenaLocationUtils.h"

#include "components/utilities/String.h"

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
	this->searchPopUpContextInstID = -1;
	this->backgroundTextureID = -1;
	this->provinceID = -1;
	this->hoveredLocationID = -1;
}

void ProvinceMapUiState::init(Game &game)
{
	DebugAssert(this->provinceID >= 0); // Must be set by world map logic already.
	this->game = &game;
	
	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	this->backgroundTextureID = ProvinceMapUiView::allocBackgroundTexture(this->provinceID, binaryAssetLibrary, game.textureManager, game.renderer);

	this->blinkState.reset();
}

void ProvinceMapUiState::freeTextures(Renderer &renderer)
{
	if (this->backgroundTextureID >= 0)
	{
		renderer.freeUiTexture(this->backgroundTextureID);
		this->backgroundTextureID = -1;
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

	UiContextInitInfo textPopUpContextInitInfo;
	textPopUpContextInitInfo.name = "ProvinceMapTextPopUp";
	textPopUpContextInitInfo.drawOrder = 1;
	state.textPopUpContextInstID = uiManager.createContext(textPopUpContextInitInfo);

	UiContextInitInfo searchPopUpContextInitInfo;
	searchPopUpContextInitInfo.name = "ProvinceMapSearchPopUp";
	searchPopUpContextInitInfo.drawOrder = 1;
	state.searchPopUpContextInstID = uiManager.createContext(searchPopUpContextInitInfo);

	uiManager.setContextEnabled(state.textPopUpContextInstID, false);
	uiManager.setContextEnabled(state.searchPopUpContextInstID, false);

	ProvinceMapUI::initLocationIconUI(state.provinceID);

	/*

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), UiPivotType::TopLeft, hoveredLocationTextDrawCallInitInfo.activeFunc);

	this->blinkState.init(ProvinceMapUiView::BlinkPeriodSeconds, true);
	this->provinceID = provinceID;
	this->hoveredLocationID = -1;

	const Window &window = game.window;
	const InputManager &inputManager = game.inputManager;
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = window.nativeToOriginal(mousePosition);
	this->updateHoveredLocationID(originalPosition);

	*/

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

	if (state.searchPopUpContextInstID >= 0)
	{
		uiManager.freeContext(state.searchPopUpContextInstID, inputManager, renderer);
		state.searchPopUpContextInstID = -1;
	}

	state.freeTextures(renderer);

	inputManager.setInputActionMapActive(InputActionMapName::WorldMap, false);
}

void ProvinceMapUI::update(double dt)
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;

	// @todo hovered text box position update
	/*UiDrawCallInitInfo hoveredLocationTextDrawCallInitInfo;
	hoveredLocationTextDrawCallInitInfo.textureFunc = [this]() { return this->hoveredLocationTextBox.getTextureID(); };
	hoveredLocationTextDrawCallInitInfo.positionFunc = [this, &game]()
	{
		auto &gameState = game.gameState;
		const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
		const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(this->provinceID);
		const LocationDefinition &locationDef = provinceDef.getLocationDef(this->hoveredLocationID);

		const Int2 locationCenter = ProvinceMapUiView::getLocationCenterPoint(game, this->provinceID, this->hoveredLocationID);
		const Int2 textBoxCenter = locationCenter - Int2(0, 10);

		// Can't use the text box dimensions with clamping since it's allocated for worst-case location name now.
		const FontLibrary &fontLibrary = FontLibrary::getInstance();
		const std::string &fontName = ProvinceMapUiView::LocationFontName;
		int fontDefIndex;
		if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
		{
			DebugCrash("Couldn't get hovered location font name \"" + fontName + "\".");
		}

		const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);

		const std::string locationName = ProvinceMapUiModel::getLocationName(game, this->provinceID, this->hoveredLocationID);
		TextRenderShadowInfo shadowInfo;
		shadowInfo.init(ProvinceMapUiView::LocationTextShadowOffsetX, ProvinceMapUiView::LocationTextShadowOffsetY, ProvinceMapUiView::LocationTextShadowColor);
		const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(locationName, fontDef, shadowInfo);

		// Clamp to screen edges, with some extra space on the left and right (note this clamped position
		// is for the TopLeft pivot type).
		const Rect textBoxRect(textBoxCenter, textureGenInfo.width, textureGenInfo.height);
		const Int2 clampedCenter = ProvinceMapUiView::getLocationTextClampedCenter(textBoxRect);
		return clampedCenter;
	};

	hoveredLocationTextDrawCallInitInfo.size = this->hoveredLocationTextBox.getRect().getSize();
	hoveredLocationTextDrawCallInitInfo.pivotType = UiPivotType::Middle;
	hoveredLocationTextDrawCallInitInfo.activeFunc = [this]() { return !this->isPaused(); };*/

	const GameState &gameState = game.gameState;
	if (gameState.getTravelData() != nullptr)
	{
		state.blinkState.update(dt);
	}
}

void ProvinceMapUI::initLocationIconUI(int provinceID)
{
	ProvinceMapUiState &state = ProvinceMapUI::state;
	Game &game = *state.game;
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
		if (locationInst.isVisible())
		{
			const int locationDefIndex = locationInst.getLocationDefIndex();
			const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);

			// @todo create UiElement for this location
			/*UiDrawCallInitInfo locationIconDrawCallInitInfo;
			locationIconDrawCallInitInfo.textureFunc = [&provinceDef, locationDefIndex]()
			{
				const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);
				const ProvinceMapLocationTextures *textureRefGroupPtr = [&locationDef]() -> const ProvinceMapLocationTextures*
				{
					const LocationDefinitionType locationDefType = locationDef.getType();
					if (locationDefType == LocationDefinitionType::City)
					{
						const LocationCityDefinition &cityDef = locationDef.getCityDefinition();
						const ArenaCityType cityType = cityDef.type;
						switch (cityType)
						{
						case ArenaCityType::CityState:
							return &this->cityStateTextureRefs;
						case ArenaCityType::Town:
							return &this->townTextureRefs;
						case ArenaCityType::Village:
							return &this->villageTextureRefs;
						default:
							DebugCrash("Unhandled city type \"" + std::to_string(static_cast<int>(cityType)) + "\".");
							return nullptr;
						}
					}
					else if (locationDefType == LocationDefinitionType::Dungeon)
					{
						return &this->dungeonTextureRefs;
					}
					else if (locationDefType == LocationDefinitionType::MainQuestDungeon)
					{
						const LocationMainQuestDungeonDefinition &mainQuestDungeonDef = locationDef.getMainQuestDungeonDefinition();
						const LocationMainQuestDungeonDefinitionType mainQuestDungeonType = mainQuestDungeonDef.type;
						switch (mainQuestDungeonType)
						{
						case LocationMainQuestDungeonDefinitionType::Start:
						case LocationMainQuestDungeonDefinitionType::Map:
							return &this->dungeonTextureRefs;
						case LocationMainQuestDungeonDefinitionType::Staff:
							return &this->staffDungeonTextureRefs;
						default:
							DebugCrash("Unhandled main quest dungeon type \"" + std::to_string(static_cast<int>(mainQuestDungeonType)) + "\".");
							return nullptr;
						}
					}
					else
					{
						DebugCrash("Unhandled location definition type \"" + std::to_string(static_cast<int>(locationDefType)) + "\".");
						return nullptr;
					}
				}();

				DebugAssert(textureRefGroupPtr != nullptr);
				return textureRefGroupPtr->textureRef.get();
			};

			locationIconDrawCallInitInfo.position = Int2(locationDef.getScreenX(), locationDef.getScreenY());
			locationIconDrawCallInitInfo.size = *renderer.tryGetUiTextureDims(locationIconDrawCallInitInfo.textureFunc());
			locationIconDrawCallInitInfo.pivotType = UiPivotType::Middle;
			this->addDrawCall(locationIconDrawCallInitInfo);*/

			UiDrawCallTextureFunc highlightIconTextureFunc = [provinceID, &state, &gameState, &provinceInst, &provinceDef, &playerProvinceDef, i, locationDefIndex]()
			{
				const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);
				const ProvinceMapUiView::HighlightType highlightType = [provinceID, &state, &gameState, &provinceDef, &playerProvinceDef, i, &locationDef]()
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
								// See if the blink period percent lies within the "on" percent. Use less-than to compare them
								// so the on-state appears before the off-state.
								if (state.blinkState.getPercent() < ProvinceMapUiView::BlinkPeriodPercentOn)
								{
									return ProvinceMapUiView::HighlightType::TravelDestination;
								}
							}
						}
					}

					return ProvinceMapUiView::HighlightType::None;
				}();

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

				DebugAssert(locationTexturesPtr != nullptr);

				switch (highlightType)
				{
				case ProvinceMapUiView::HighlightType::None:
					return locationTexturesPtr->textureID;
				case ProvinceMapUiView::HighlightType::PlayerLocation:
					return locationTexturesPtr->playerCurrentTextureID;
				case ProvinceMapUiView::HighlightType::TravelDestination:
					return locationTexturesPtr->travelDestinationTextureID;
				default:
					DebugUnhandledReturnMsg(UiTextureID, std::to_string(static_cast<int>(highlightType)));
				}
			};

			// @todo create UiElement for this location highlight
			/*UiDrawCallInitInfo highlightIconDrawCallInitInfo;
			highlightIconDrawCallInitInfo.textureFunc = highlightIconTextureFunc;
			highlightIconDrawCallInitInfo.position = locationIconDrawCallInitInfo.position;
			highlightIconDrawCallInitInfo.size = *renderer.tryGetUiTextureDims(highlightIconTextureFunc());
			highlightIconDrawCallInitInfo.pivotType = UiPivotType::Middle;
			highlightIconDrawCallInitInfo.activeFunc = [provinceID, &gameState, &provinceInst, &provinceDef, &playerProvinceDef, i, locationDefIndex]()
			{
				const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);
				const LocationDefinition &playerLocationDef = gameState.getLocationDefinition();
				if (provinceDef.matches(playerProvinceDef) && locationDef.matches(playerLocationDef))
				{
					return true;
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
							// See if the blink period percent lies within the "on" percent. Use less-than to compare them
							// so the on-state appears before the off-state.
							if (state.blinkState.getPercent() < ProvinceMapUiView::BlinkPeriodPercentOn)
							{
								return true;
							}
						}
					}
				}

				return false;
			};

			this->addDrawCall(highlightIconDrawCallInitInfo);*/
		}
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
		const UiElementInstanceID hoveredLocationTextBoxElementInstID = uiManager.getElementByName("ProvinceMapHoveredLocationTextBox");
		uiManager.setTextBoxText(hoveredLocationTextBoxElementInstID, locationName.c_str());
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

		const std::string travelText = ProvinceMapUiModel::makeTravelText(game, state.provinceID, currentLocationDef, currentProvinceDef, selectedLocationID);
		std::unique_ptr<Panel> textPopUp = ProvinceMapUiModel::makeTextPopUp(game, travelText);
		game.pushSubPanel(std::move(textPopUp));
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
	game.pushSubPanel<FastTravelSubPanel>();
	game.setPanel<WorldMapPanel>();
}

void ProvinceMapUI::showTextPopUp(const char *str)
{
	// @todo
	//std::unique_ptr<Panel> textPopUp = ProvinceMapUiModel::makeTextPopUp(game, str);
	//game.pushSubPanel(std::move(textPopUp));
}

void ProvinceMapUI::onPauseChanged(bool paused)
{
	if (!paused)
	{
		// Make sure the hovered location matches where the pointer is now since mouse motion events
		// aren't processed while this panel is paused.
		ProvinceMapUiState &state = ProvinceMapUI::state;
		Game &game = *state.game;
		const Window &window = game.window;
		const InputManager &inputManager = game.inputManager;
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPosition = window.nativeToOriginal(mousePosition);
		ProvinceMapUI::updateHoveredLocationID(originalPosition);
	}
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
	const InputManager &inputManager = game.inputManager;
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 classicPosition = game.window.nativeToOriginal(mousePosition);

	constexpr Rect searchButtonRect = ProvinceMapUiView::SearchButtonRect;
	constexpr Rect travelButtonRect = ProvinceMapUiView::TravelButtonRect;
	constexpr Rect backButtonRect = ProvinceMapUiView::BackToWorldMapRect;

	if (searchButtonRect.contains(classicPosition))
	{
		// @todo enable pop up context
		//game.pushSubPanel<ProvinceSearchSubPanel>(panel, provinceID);
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
			const std::string errorText = [&exeData]()
			{
				std::string text = exeData.travel.noDestination;

				// Remove carriage return at end.
				text.pop_back();

				text = String::replace(text, '\r', '\n');

				return text;
			}();

			ProvinceMapUI::showTextPopUp(errorText.c_str());
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
