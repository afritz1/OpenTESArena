#include <cstring>

#include "AutomapUiState.h"
#include "CharacterUiState.h"
#include "GameWorldUiMVC.h"
#include "GameWorldUiState.h"
#include "LogbookUiState.h"
#include "PauseMenuUiState.h"
#include "WorldMapUiState.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Items/ItemLibrary.h"
#include "../Player/PlayerLogic.h"
#include "../Player/WeaponAnimationLibrary.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../UI/FontLibrary.h"
#include "../UI/TextEntry.h"
#include "../UI/UiRenderSpace.h"
#include "../World/MapType.h"

namespace
{
	constexpr char ContextName_TextPopUp[] = "GameWorldTextPopUp";
	constexpr char ContextName_LootPopUp[] = "GameWorldLootPopUp";
	constexpr char ContextName_CampModal[] = "GameWorldCampModal";
	constexpr char ContextName_CampManualHoursModal[] = "GameWorldCampManualHoursModal";
	constexpr char ContextName_ConversationModal[] = "GameWorldConversationModal";
	constexpr char ContextName_ShopkeeperBackground[] = "GameWorldShopkeeperBackground";

	constexpr char InterfaceImageElementName[] = "GameWorldInterfaceImage";
	constexpr char NoMagicImageElementName[] = "GameWorldNoMagicImage";
	constexpr char StatusGradientImageElementName[] = "GameWorldStatusGradientImage";
	constexpr char PlayerPortraitImageElementName[] = "GameWorldPlayerPortraitImage";
	constexpr char StatusBarsImageElementName[] = "GameWorldPlayerStatusBarsImage";

	constexpr char PlayerNameTextBoxElementName[] = "GameWorldPlayerNameTextBox";

	constexpr char CompassSliderImageElementName[] = "GameWorldCompassSlider";
	constexpr char CompassFrameImageElementName[] = "GameWorldCompassFrame";

	constexpr char WeaponImageElementName[] = "GameWorldWeaponImage";
	constexpr char PlayerHurtImageElementName[] = "GameWorldPlayerHurtImage";
	constexpr char ModernModeReticleImageElementName[] = "GameWorldModernModeReticleImage";

	constexpr char TriggerTextBoxElementName[] = "GameWorldTriggerTextBox";
	constexpr char ActionTextBoxElementName[] = "GameWorldActionTextBox";
	constexpr char EffectTextBoxElementName[] = "GameWorldEffectTextBox";
	constexpr char CampingHoursTextBoxElementName[] = "GameWorldCampingHoursTextBox";

	constexpr const char *ButtonElementNames[] =
	{
		"GameWorldCharacterSheetButton",
		"GameWorldWeaponToggleButton",
		"GameWorldMapButton",
		"GameWorldStealButton",
		"GameWorldStatusButton",
		"GameWorldMagicButton",
		"GameWorldLogbookButton",
		"GameWorldUseItemButton",
		"GameWorldCampButton",
		"GameWorldScrollUpButton",
		"GameWorldScrollDownButton",
	};

	constexpr MouseButtonTypeFlags PopUpMouseButtonTypeFlags = MouseButtonType::Left | MouseButtonType::Right;

	std::string GetKeyImageElementName(int keyIndex)
	{
		char elementName[32];
		std::snprintf(elementName, sizeof(elementName), "GameWorldKey%dImage", keyIndex);
		return std::string(elementName);
	}

	bool IsPlayerWeaponVisible(const Player &player)
	{
		const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
		const WeaponAnimationDefinitionID weaponAnimDefID = player.getEquippedWeaponAnimationDefID();
		const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(weaponAnimDefID);
		const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
		DebugAssertIndex(weaponAnimDef.states, weaponAnimInst.currentStateIndex);
		const WeaponAnimationDefinitionState &weaponAnimDefState = weaponAnimDef.states[weaponAnimInst.currentStateIndex];
		return !WeaponAnimationUtils::isSheathed(weaponAnimDefState);
	}

	Int2 GetStatusBarsModernModePosition(const Window &window)
	{
		const Int2 windowDims = window.getPixelDimensions();
		const Int2 statusBarModernModeWindowPosition(GameWorldUiView::StatusBarModernModeXOffset, windowDims.y - GameWorldUiView::StatusBarModernModeYOffset);
		return window.nativeToOriginal(statusBarModernModeWindowPosition);
	}

	std::string GetLootItemDisplayNameWithQty(const ItemDefinition &itemDef, int stackAmount)
	{
		std::string displayName = itemDef.getDisplayName(stackAmount);
		if (itemDef.type == ItemType::Gold)
		{
			size_t goldCountIndex = displayName.find("%u");
			if (goldCountIndex != std::string::npos)
			{
				displayName.replace(goldCountIndex, 2, std::to_string(stackAmount));
			}
		}

		return displayName;
	}
}

GameWorldLootUiItemMapping::GameWorldLootUiItemMapping()
{
	this->inventoryItemIndex = -1;
	this->listBoxItemIndex = -1;
}

void GameWorldUiInitInfo::init(const std::string &textPopUpMessage)
{
	this->textPopUpMessage = textPopUpMessage;
}

GameWorldUiState::GameWorldUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->textPopUpContextInstID = -1;
	this->lootPopUpContextInstID = -1;
	this->campModalContextInstID = -1;
	this->campManualHoursModalContextInstID = -1;
	this->statusBarsTextureID = -1;
	this->playerHurtTextureID = -1;
	this->modernModeReticleTextureID = -1;
	this->currentHealth = 0.0;
	this->maxHealth = 0.0;
	this->currentStamina = 0.0;
	this->maxStamina = 0.0;
	this->currentSpellPoints = 0.0;
	this->maxSpellPoints = 0.0;
	this->interactionType = GameWorldInteractionType::Default;
	this->triggerTextRemainingSeconds = 0.0;
	this->actionTextRemainingSeconds = 0.0;
	this->effectTextRemainingSeconds = 0.0;
	this->playerHurtRemainingSeconds = 0.0;
}

void GameWorldUiState::init(Game &game)
{
	const Player &player = game.player;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	this->game = &game;

	this->statusBarsTextureID = GameWorldUiView::allocStatusBarsTexture(textureManager, renderer);

	const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
	const WeaponAnimationDefinitionID weaponAnimDefID = player.getEquippedWeaponAnimationDefID();
	const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(weaponAnimDefID);
	this->weaponAnimTextureIDs.init(weaponAnimDef.frameCount);
	for (int i = 0; i < weaponAnimDef.frameCount; i++)
	{
		const WeaponAnimationDefinitionFrame &weaponAnimDefFrame = weaponAnimDef.frames[i];
		const TextureAsset &weaponAnimDefFrameTextureAsset = weaponAnimDefFrame.textureAsset;
		const std::string &weaponAnimDefFrameTextureFilename = weaponAnimDefFrameTextureAsset.filename;
		DebugAssert(weaponAnimDefFrameTextureAsset.index >= 0);
		const int weaponAnimDefFrameTextureIndex = weaponAnimDefFrameTextureAsset.index;
		// @todo: some WeaponAnimationDefinitionFrames are duplicates, this can cause duplicate UiTextureID allocations, maybe map TextureAsset to UiTextureID to avoid it
		const UiTextureID weaponTextureID = GameWorldUiView::allocWeaponAnimTexture(weaponAnimDefFrameTextureFilename, weaponAnimDefFrameTextureIndex, textureManager, renderer);
		this->weaponAnimTextureIDs.set(i, weaponTextureID);
	}

	const int keyTextureCount = GameWorldUiView::getKeyTextureCount(textureManager);
	this->keyTextureIDs.init(keyTextureCount);
	for (int i = 0; i < keyTextureCount; i++)
	{
		const UiTextureID keyTextureID = GameWorldUiView::allocKeyTexture(i, textureManager, renderer);
		this->keyTextureIDs.set(i, keyTextureID);
	}

	this->arrowCursorTextureIDs.init(GameWorldUiView::ArrowCursorRegionCount);
	for (int i = 0; i < GameWorldUiView::ArrowCursorRegionCount; i++)
	{
		const UiTextureID arrowTextureID = GameWorldUiView::allocArrowCursorTexture(i, textureManager, renderer);
		this->arrowCursorTextureIDs.set(i, arrowTextureID);
	}

	const Window &window = game.window;
	this->playerHurtTextureID = GameWorldUiView::allocPlayerHurtTexture(window.getSceneViewAspectRatio(), window.fullGameWindow, renderer);

	this->modernModeReticleTextureID = GameWorldUiView::allocModernModeReticleTexture(textureManager, renderer);

	const Int2 windowDims = window.getPixelDimensions();
	this->updateNativeCursorRegions(windowDims.x, windowDims.y);

	this->currentHealth = player.currentHealth;
	this->maxHealth = player.maxHealth;
	this->currentStamina = player.currentStamina;
	this->maxStamina = player.maxStamina;
	this->currentSpellPoints = player.currentSpellPoints;
	this->maxSpellPoints = player.maxSpellPoints;
	this->interactionType = GameWorldInteractionType::Default;
}

void GameWorldUiState::freeTextures(Renderer &renderer)
{
	if (this->statusBarsTextureID >= 0)
	{
		renderer.freeUiTexture(this->statusBarsTextureID);
		this->statusBarsTextureID = -1;
	}

	if (this->weaponAnimTextureIDs.isValid())
	{
		for (const UiTextureID textureID : this->weaponAnimTextureIDs)
		{
			renderer.freeUiTexture(textureID);
		}

		this->weaponAnimTextureIDs.clear();
	}

	if (this->keyTextureIDs.isValid())
	{
		for (const UiTextureID textureID : this->keyTextureIDs)
		{
			renderer.freeUiTexture(textureID);
		}

		this->keyTextureIDs.clear();
	}

	if (this->arrowCursorTextureIDs.isValid())
	{
		for (const UiTextureID textureID : this->arrowCursorTextureIDs)
		{
			renderer.freeUiTexture(textureID);
		}

		this->arrowCursorTextureIDs.clear();
	}

	if (this->playerHurtTextureID >= 0)
	{
		renderer.freeUiTexture(this->playerHurtTextureID);
		this->playerHurtTextureID = -1;
	}

	if (this->modernModeReticleTextureID >= 0)
	{
		renderer.freeUiTexture(this->modernModeReticleTextureID);
		this->modernModeReticleTextureID = -1;
	}
}

void GameWorldUiState::updateNativeCursorRegions(int windowWidth, int windowHeight)
{
	// @todo: maybe the classic rects should be converted to vector space then scaled by the ratio of aspect ratios?
	const double xScale = static_cast<double>(windowWidth) / ArenaRenderUtils::SCREEN_WIDTH_REAL;
	const double yScale = static_cast<double>(windowHeight) / ArenaRenderUtils::SCREEN_HEIGHT_REAL;

	for (int i = 0; i < static_cast<int>(std::size(this->nativeCursorRegions)); i++)
	{
		this->nativeCursorRegions[i] = GameWorldUiView::scaleClassicCursorRectToNative(i, xScale, yScale);
	}
}

void GameWorldUI::create(Game &game)
{
	GameWorldUiState &state = GameWorldUI::state;
	state.init(game);

	const GameState &gameState = game.gameState;
	const Options &options = game.options;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(GameWorldUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	UiContextInitInfo textPopUpContextInitInfo;
	textPopUpContextInitInfo.name = ContextName_TextPopUp;
	textPopUpContextInitInfo.drawOrder = 1;
	state.textPopUpContextInstID = uiManager.createContext(textPopUpContextInitInfo);
	uiManager.setContextEnabled(state.textPopUpContextInstID, false);

	UiContextInitInfo lootPopUpContextInitInfo;
	lootPopUpContextInitInfo.name = ContextName_LootPopUp;
	lootPopUpContextInitInfo.drawOrder = 1;
	state.lootPopUpContextInstID = uiManager.createContext(lootPopUpContextInitInfo);
	uiManager.setContextEnabled(state.lootPopUpContextInstID, false);

	UiContextInitInfo campModalContextInitInfo;
	campModalContextInitInfo.name = ContextName_CampModal;
	campModalContextInitInfo.drawOrder = 1;
	state.campModalContextInstID = uiManager.createContext(campModalContextInitInfo);
	uiManager.setContextEnabled(state.campModalContextInstID, false);

	UiContextInitInfo campManualHoursModalContextInitInfo;
	campManualHoursModalContextInitInfo.name = ContextName_CampManualHoursModal;
	campManualHoursModalContextInitInfo.drawOrder = 1;
	state.campManualHoursModalContextInstID = uiManager.createContext(campManualHoursModalContextInitInfo);
	uiManager.setContextEnabled(state.campManualHoursModalContextInstID, false);

	UiContextInitInfo conversationModalContextInitInfo;
	conversationModalContextInitInfo.name = ContextName_ConversationModal;
	conversationModalContextInitInfo.drawOrder = 2;
	state.conversationModalContextInstID = uiManager.createContext(conversationModalContextInitInfo);
	uiManager.setContextEnabled(state.conversationModalContextInstID, false);

	UiContextInitInfo shopkeeperBackgroundContextInitInfo;
	shopkeeperBackgroundContextInitInfo.name = ContextName_ShopkeeperBackground;
	shopkeeperBackgroundContextInitInfo.drawOrder = 1;
	state.shopkeeperBgContextInstID = uiManager.createContext(shopkeeperBackgroundContextInitInfo);
	uiManager.setContextEnabled(state.shopkeeperBgContextInstID, false);

	const bool isModernInterface = options.getGraphics_ModernInterface();

	UiElementInitInfo weaponAnimImageElementInitInfo;
	weaponAnimImageElementInitInfo.name = WeaponImageElementName;
	weaponAnimImageElementInitInfo.sizeType = isModernInterface ? UiTransformSizeType::Manual : UiTransformSizeType::Content;
	weaponAnimImageElementInitInfo.drawOrder = 0;
	weaponAnimImageElementInitInfo.renderSpace = isModernInterface ? UiRenderSpace::Native : UiRenderSpace::Classic;

	const UiTextureID dummyWeaponAnimImageTextureID = state.weaponAnimTextureIDs[0];
	uiManager.createImage(weaponAnimImageElementInitInfo, dummyWeaponAnimImageTextureID, state.contextInstID, renderer);

	for (int i = 0; i < state.keyTextureIDs.getCount(); i++)
	{
		UiElementInitInfo keyImageElementInitInfo;
		keyImageElementInitInfo.name = GetKeyImageElementName(i);
		keyImageElementInitInfo.position = GameWorldUiView::getKeyPosition(i);
		keyImageElementInitInfo.drawOrder = 0;

		const UiTextureID keyImageTextureID = state.keyTextureIDs[i];
		uiManager.createImage(keyImageElementInitInfo, keyImageTextureID, state.contextInstID, renderer);
	}

	GameWorldUI::updateDoorKeys();

	UiElementInitInfo statusBarsImageElementInitInfo;
	statusBarsImageElementInitInfo.name = StatusBarsImageElementName;
	statusBarsImageElementInitInfo.position = GameWorldUiView::HealthBarRect.getBottomLeft();

	if (isModernInterface)
	{
		statusBarsImageElementInitInfo.position = GetStatusBarsModernModePosition(game.window);
	}

	statusBarsImageElementInitInfo.pivotType = GameWorldUiView::StatusBarPivotType;
	statusBarsImageElementInitInfo.drawOrder = 5;
	uiManager.createImage(statusBarsImageElementInitInfo, state.statusBarsTextureID, state.contextInstID, renderer);

	UiElementInitInfo modernModeReticleImageElementInitInfo;
	modernModeReticleImageElementInitInfo.name = ModernModeReticleImageElementName;
	modernModeReticleImageElementInitInfo.position = GameWorldUiView::getInterfaceCenter(game);
	modernModeReticleImageElementInitInfo.pivotType = UiPivotType::Middle;
	modernModeReticleImageElementInitInfo.drawOrder = 1;
	const UiElementInstanceID modernModeReticleImageElementInstID = uiManager.createImage(modernModeReticleImageElementInitInfo, state.modernModeReticleTextureID, state.contextInstID, renderer);

	UiElementInitInfo playerHurtImageElementInitInfo;
	playerHurtImageElementInitInfo.name = PlayerHurtImageElementName;
	playerHurtImageElementInitInfo.sizeType = UiTransformSizeType::Manual;
	playerHurtImageElementInitInfo.size = game.window.getSceneViewDimensions();
	playerHurtImageElementInitInfo.renderSpace = UiRenderSpace::Native;
	playerHurtImageElementInitInfo.drawOrder = 3;
	const UiElementInstanceID playerHurtImageElementInstID = uiManager.createImage(playerHurtImageElementInitInfo, state.playerHurtTextureID, state.contextInstID, renderer);
	uiManager.setElementActive(playerHurtImageElementInstID, false);

	const Player &player = game.player;
	const TextureAsset playerPortraitTextureAsset = GameWorldUiView::getPlayerPortraitTextureAsset(player.male, player.raceID, player.portraitID);
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();
	const UiTextureID playerPortraitTextureID = uiManager.getOrAddTexture(playerPortraitTextureAsset, paletteTextureAsset, textureManager, renderer);
	const UiElementInstanceID playerPortraitImageElementInstID = uiManager.getElementByName(PlayerPortraitImageElementName);
	uiManager.setImageTexture(playerPortraitImageElementInstID, playerPortraitTextureID);

	const UiElementInstanceID playerNameTextBoxElementInstID = uiManager.getElementByName(PlayerNameTextBoxElementName);
	const std::string playerNameText = GameWorldUiModel::getPlayerNameText(game);
	uiManager.setTextBoxText(playerNameTextBoxElementInstID, playerNameText.c_str());

	const UiElementInstanceID triggerTextBoxElementInstID = uiManager.getElementByName(TriggerTextBoxElementName);
	const Int2 triggerTextBoxPosition = GameWorldUiView::getTriggerTextPosition(game, ArenaRenderUtils::SCENE_UI_HEIGHT);
	uiManager.setTransformPosition(triggerTextBoxElementInstID, triggerTextBoxPosition);

	const UiElementInstanceID actionTextBoxElementInstID = uiManager.getElementByName(ActionTextBoxElementName);
	const Int2 actionTextBoxPosition = GameWorldUiView::getActionTextPosition();
	uiManager.setTransformPosition(actionTextBoxElementInstID, actionTextBoxPosition);

	const UiElementInstanceID effectTextBoxElementInstID = uiManager.getElementByName(EffectTextBoxElementName);
	const Int2 effectTextBoxPosition = GameWorldUiView::getEffectTextPosition(game, ArenaRenderUtils::SCENE_UI_HEIGHT);
	uiManager.setTransformPosition(effectTextBoxElementInstID, effectTextBoxPosition);

	if (isModernInterface)
	{
		const UiElementInstanceID interfaceImageElementInstID = uiManager.getElementByName(InterfaceImageElementName);
		uiManager.setElementActive(interfaceImageElementInstID, false);

		const UiElementInstanceID noMagicImageElementInstID = uiManager.getElementByName(NoMagicImageElementName);
		uiManager.setElementActive(noMagicImageElementInstID, false);

		const UiElementInstanceID statusGradientImageElementInstID = uiManager.getElementByName(StatusGradientImageElementName);
		uiManager.setElementActive(statusGradientImageElementInstID, false);

		uiManager.setElementActive(playerPortraitImageElementInstID, false);
		uiManager.setElementActive(playerNameTextBoxElementInstID, false);

		for (const char *buttonElementName : ButtonElementNames)
		{
			const UiElementInstanceID buttonElementInstID = uiManager.getElementByName(buttonElementName);
			uiManager.setElementActive(buttonElementInstID, false);
		}

		uiManager.setElementActive(game.cursorImageElementInstID, false);

		GameWorldUiModel::setFreeLookActive(game, true);
	}
	else
	{
		uiManager.setElementActive(modernModeReticleImageElementInstID, false);
	}

	GameWorldUiView::updateStatusBarsTexture(state.statusBarsTextureID, game.player, renderer);

	const bool isCompassVisible = options.getMisc_ShowCompass() && !gameState.isCamping();
	GameWorldUI::setCompassVisible(isCompassVisible);

	uiManager.addMouseButtonChangedListener(GameWorldUI::onMouseButtonChanged, GameWorldUI::ContextName, inputManager);
	uiManager.addMouseButtonHeldListener(GameWorldUI::onMouseButtonHeld, GameWorldUI::ContextName, inputManager);
	uiManager.addWindowResizedListener(GameWorldUI::onWindowResized, GameWorldUI::ContextName, inputManager);

	game.shouldSimulateScene = true;
	game.shouldRenderScene = true;

	inputManager.setInputActionMapActive(InputActionMapName::GameWorld, true);

	if (!state.initInfo.textPopUpMessage.empty())
	{
		GameWorldUI::showTextPopUp(state.initInfo.textPopUpMessage.c_str(), GameWorldUiView::StatusPopUpFontName, GameWorldUiView::StatusPopUpTextAlignment);
	}
}

void GameWorldUI::destroy()
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
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

	if (state.lootPopUpContextInstID >= 0)
	{
		uiManager.freeContext(state.lootPopUpContextInstID, inputManager, renderer);
		state.lootPopUpContextInstID = -1;
	}

	if (state.campModalContextInstID >= 0)
	{
		uiManager.freeContext(state.campModalContextInstID, inputManager, renderer);
		state.campModalContextInstID = -1;
	}

	if (state.campManualHoursModalContextInstID >= 0)
	{
		uiManager.freeContext(state.campManualHoursModalContextInstID, inputManager, renderer);
		state.campManualHoursModalContextInstID = -1;
	}

	if (state.conversationModalContextInstID >= 0)
	{
		uiManager.freeContext(state.conversationModalContextInstID, inputManager, renderer);
		state.conversationModalContextInstID = -1;
	}

	if (state.shopkeeperBgContextInstID >= 0)
	{
		uiManager.freeContext(state.shopkeeperBgContextInstID, inputManager, renderer);
		state.shopkeeperBgContextInstID = -1;
	}

	state.freeTextures(renderer);
	state.initInfo.textPopUpMessage.clear();
	state.currentHealth = 0.0;
	state.maxHealth = 0.0;
	state.currentStamina = 0.0;
	state.maxStamina = 0.0;
	state.currentSpellPoints = 0.0;
	state.maxSpellPoints = 0.0;
	state.playerHurtRemainingSeconds = 0.0;
	state.lootPopUpItemMappings.clear();

	inputManager.setInputActionMapActive(InputActionMapName::GameWorld, false);

	const Options &options = game.options;
	if (options.getGraphics_ModernInterface())
	{
		uiManager.setElementActive(game.cursorImageElementInstID, true);
		GameWorldUiModel::setFreeLookActive(game, false);
	}

	game.shouldSimulateScene = false;
	game.shouldRenderScene = false;
}

void GameWorldUI::update(double dt)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	const Options &options = game.options;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	const Window &window = game.window;
	Renderer &renderer = game.renderer;

	const bool isModernInterface = options.getGraphics_ModernInterface();

	// Compass
	const Player &player = game.player;
	const Double2 playerDirection = player.getGroundDirectionXZ();
	const Int2 compassSliderPosition = GameWorldUiView::getCompassSliderPosition(game, playerDirection);
	const UiElementInstanceID compassSliderImageElementInstID = uiManager.getElementByName(CompassSliderImageElementName);
	const UiElementInstanceID compassFrameImageElementInstID = uiManager.getElementByName(CompassFrameImageElementName);
	uiManager.setTransformPosition(compassSliderImageElementInstID, compassSliderPosition);

	const bool isCompassVisible = options.getMisc_ShowCompass() && !gameState.isCamping();
	GameWorldUI::setCompassVisible(isCompassVisible);

	// Weapon
	const UiElementInstanceID weaponImageElementInstID = uiManager.getElementByName(WeaponImageElementName);
	const bool isWeaponVisible = IsPlayerWeaponVisible(player);
	uiManager.setElementActive(weaponImageElementInstID, isWeaponVisible);
	if (isWeaponVisible)
	{
		const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
		const WeaponAnimationDefinitionID weaponAnimDefID = player.getEquippedWeaponAnimationDefID();
		const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(weaponAnimDefID);
		const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
		const int weaponAnimFrameIndex = WeaponAnimationUtils::getFrameIndex(weaponAnimInst, weaponAnimDef);
		DebugAssertIndex(weaponAnimDef.frames, weaponAnimFrameIndex);
		const WeaponAnimationDefinitionFrame &weaponAnimFrame = weaponAnimDef.frames[weaponAnimFrameIndex];
		const UiTextureID weaponAnimTextureID = state.weaponAnimTextureIDs[weaponAnimFrameIndex];
		const std::optional<Int2> weaponTextureDims = renderer.tryGetUiTextureDims(weaponAnimTextureID);
		DebugAssert(weaponTextureDims.has_value());

		Int2 weaponPosition;
		Int2 weaponSize;
		if (isModernInterface)
		{
			constexpr int classicViewHeight = ArenaRenderUtils::SCENE_VIEW_HEIGHT;
			const Int2 windowDims = window.getPixelDimensions();
			const Double2 windowDimsReal(
				static_cast<double>(windowDims.x),
				static_cast<double>(windowDims.y));
			const Double2 weaponOffsetPercents(
				static_cast<double>(weaponAnimFrame.xOffset) / ArenaRenderUtils::SCREEN_WIDTH_REAL,
				static_cast<double>(weaponAnimFrame.yOffset) / static_cast<double>(classicViewHeight));
			const Double2 weaponTextureScreenSizePercents(
				static_cast<double>(weaponTextureDims->x) / ArenaRenderUtils::SCREEN_WIDTH_REAL,
				static_cast<double>(weaponTextureDims->y) / static_cast<double>(classicViewHeight));

			weaponPosition = Int2(
				static_cast<int>(std::round(weaponOffsetPercents.x * windowDimsReal.x)),
				static_cast<int>(std::round(weaponOffsetPercents.y * windowDimsReal.y)));
			weaponSize = Int2(
				static_cast<int>(std::round(weaponTextureScreenSizePercents.x * windowDimsReal.x)),
				static_cast<int>(std::round(weaponTextureScreenSizePercents.y * windowDimsReal.y)));
		}
		else
		{
			weaponPosition = Int2(weaponAnimFrame.xOffset, weaponAnimFrame.yOffset);
			weaponSize = *weaponTextureDims;
		}

		uiManager.setTransformPosition(weaponImageElementInstID, weaponPosition);
		uiManager.setTransformSize(weaponImageElementInstID, weaponSize);
		uiManager.setImageTexture(weaponImageElementInstID, weaponAnimTextureID);
	}

	// Status bars
	if (isModernInterface)
	{
		const UiElementInstanceID statusBarsImageElementInstID = uiManager.getElementByName(StatusBarsImageElementName);
		const Int2 statusBarsModernModePosition = GetStatusBarsModernModePosition(game.window);
		uiManager.setTransformPosition(statusBarsImageElementInstID, statusBarsModernModePosition);
	}

	const int previousHealthBarHeight = GameWorldUiView::getStatusBarCurrentPixelHeight(state.currentHealth, state.maxHealth);
	const int previousStaminaBarHeight = GameWorldUiView::getStatusBarCurrentPixelHeight(state.currentStamina, state.maxStamina);
	const int previousSpellPointsBarHeight = GameWorldUiView::getStatusBarCurrentPixelHeight(state.currentSpellPoints, state.maxSpellPoints);
	const int currentHealthBarHeight = GameWorldUiView::getStatusBarCurrentPixelHeight(player.currentHealth, player.maxHealth);
	const int currentStaminaBarHeight = GameWorldUiView::getStatusBarCurrentPixelHeight(player.currentStamina, player.maxStamina);
	const int currentSpellPointsBarHeight = GameWorldUiView::getStatusBarCurrentPixelHeight(player.currentSpellPoints, player.maxSpellPoints);

	const bool isStatusBarsTextureDirty = (previousHealthBarHeight != currentHealthBarHeight) ||
		(previousStaminaBarHeight != currentStaminaBarHeight) ||
		(previousSpellPointsBarHeight != currentSpellPointsBarHeight);

	if (isStatusBarsTextureDirty)
	{
		state.currentHealth = player.currentHealth;
		state.maxHealth = player.maxHealth;
		state.currentStamina = player.currentStamina;
		state.maxStamina = player.maxStamina;
		state.currentSpellPoints = player.currentSpellPoints;
		state.maxSpellPoints = player.maxSpellPoints;
		GameWorldUiView::updateStatusBarsTexture(state.statusBarsTextureID, player, renderer);
	}

	// Trigger/action/effect text
	const bool isTriggerTextVisible = GameWorldUI::isTriggerTextVisible();
	const UiElementInstanceID triggerTextBoxElementInstID = uiManager.getElementByName(TriggerTextBoxElementName);
	uiManager.setElementActive(triggerTextBoxElementInstID, isTriggerTextVisible);

	const bool isActionTextVisible = GameWorldUI::isActionTextVisible();
	const UiElementInstanceID actionTextBoxElementInstID = uiManager.getElementByName(ActionTextBoxElementName);
	uiManager.setElementActive(actionTextBoxElementInstID, isActionTextVisible);

	const bool isEffectTextVisible = GameWorldUI::isEffectTextVisible();
	const UiElementInstanceID effectTextBoxElementInstID = uiManager.getElementByName(EffectTextBoxElementName);
	uiManager.setElementActive(effectTextBoxElementInstID, isEffectTextVisible);

	const bool isCampingHoursTextVisible = GameWorldUI::isCampingHoursTextVisible();
	const UiElementInstanceID campingHoursTextBoxElementInstID = uiManager.getElementByName(CampingHoursTextBoxElementName);
	uiManager.setElementActive(campingHoursTextBoxElementInstID, isCampingHoursTextVisible);

	if (!isModernInterface)
	{
		const PlayerStatusGradientType statusGradientType = GameWorldUiModel::getCurrentPlayerStatusGradientType(player);
		const TextureAsset statusGradientTextureAsset = GameWorldUiView::getStatusGradientTextureAsset(statusGradientType);
		const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();
		const UiTextureID statusGradientTextureID = uiManager.getOrAddTexture(statusGradientTextureAsset, paletteTextureAsset, textureManager, renderer);
		const UiElementInstanceID statusGradientElementInstID = uiManager.getElementByName(StatusGradientImageElementName);
		uiManager.setImageTexture(statusGradientElementInstID, statusGradientTextureID);

		const InputManager &inputManager = game.inputManager;
		const Int2 cursorPosition = inputManager.getMousePosition();

		int arrowCursorRegionIndex = -1;
		for (int i = 0; i < GameWorldUiView::ArrowCursorRegionCount; i++)
		{
			const Rect &nativeCursorRegion = state.nativeCursorRegions[i];
			if (nativeCursorRegion.contains(cursorPosition))
			{
				arrowCursorRegionIndex = i;
				break;
			}
		}

		if (arrowCursorRegionIndex >= 0)
		{
			const UiTextureID cursorTextureID = state.arrowCursorTextureIDs[arrowCursorRegionIndex];

			Span<const UiPivotType> arrowCursorPivotTypes = GameWorldUiView::ArrowCursorPivotTypes;
			const UiPivotType cursorPivotType = arrowCursorPivotTypes[arrowCursorRegionIndex];
			game.setCursorOverride(UiCursorOverrideState(cursorTextureID, cursorPivotType));
		}
		else
		{
			game.setCursorOverride(std::nullopt);
		}
	}

	// Player hurt.
	const bool isPlayerHurtVisible = state.playerHurtRemainingSeconds > 0.0;
	const UiElementInstanceID playerHurtImageElementInstID = uiManager.getElementByName(PlayerHurtImageElementName);
	uiManager.setElementActive(playerHurtImageElementInstID, isPlayerHurtVisible);

	if (isPlayerHurtVisible)
	{
		state.playerHurtRemainingSeconds = std::max(state.playerHurtRemainingSeconds - dt, 0.0);
	}
}

void GameWorldUI::onScreenToWorldInteraction(Int2 windowPoint, bool isPrimaryInteraction)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	const InputManager &inputManager = game.inputManager;
	const bool debugFadeVoxel = isPrimaryInteraction && inputManager.keyIsDown(SDL_SCANCODE_G);
	PlayerLogic::handleScreenToWorldInteraction(game, windowPoint, isPrimaryInteraction, debugFadeVoxel);
}

void GameWorldUI::updateDoorKeys()
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	const Player &player = game.player;
	UiManager &uiManager = game.uiManager;
	const Renderer &renderer = game.renderer;

	const Span<const int> keyInventory = player.keyInventory;
	for (int i = 0; i < state.keyTextureIDs.getCount(); i++)
	{
		const std::string keyImageElementName = GetKeyImageElementName(i);
		const UiElementInstanceID keyImageElementInstID = uiManager.getElementByName(keyImageElementName.c_str());

		const int keyID = keyInventory[i];
		const bool isKeyVisible = game.shouldSimulateScene && (keyID != ArenaItemUtils::InvalidDoorKeyID);
		uiManager.setElementActive(keyImageElementInstID, isKeyVisible);

		if (isKeyVisible)
		{
			const UiTextureID keyTextureID = state.keyTextureIDs[keyID];
			const std::optional<Int2> keyDimensions = renderer.tryGetUiTextureDims(keyTextureID);
			DebugAssert(keyDimensions.has_value());

			uiManager.setTransformSize(keyImageElementInstID, *keyDimensions);
			uiManager.setImageTexture(keyImageElementInstID, keyTextureID);
		}
	}
}

void GameWorldUI::setCompassVisible(bool visible)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID compassSliderImageElementInstID = uiManager.getElementByName(CompassSliderImageElementName);
	const UiElementInstanceID compassFrameImageElementInstID = uiManager.getElementByName(CompassFrameImageElementName);
	uiManager.setElementActive(compassSliderImageElementInstID, visible);
	uiManager.setElementActive(compassFrameImageElementInstID, visible);
}

void GameWorldUI::setInteractionType(GameWorldInteractionType type)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

	state.interactionType = type;

	switch (type)
	{
	case GameWorldInteractionType::Default:
	{
		GameWorldUI::setActionText("");
		state.actionTextRemainingSeconds = 0.0;
		break;
	}
	case GameWorldInteractionType::Thieving:
	{
		GameWorldUI::setActionText(exeData.thieving.thievingInteractionType.c_str());
		state.actionTextRemainingSeconds = Constants::Infinity;
		break;
	}
	default:
		DebugNotImplementedMsg(std::to_string(static_cast<int>(type)));
		break;
	}
}

void GameWorldUI::onPauseChanged(bool paused)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	const GameState &gameState = game.gameState;
	const Player &player = game.player;
	UiManager &uiManager = game.uiManager;
	const Options &options = game.options;
	const bool isModernInterface = options.getGraphics_ModernInterface();

	game.shouldSimulateScene = !paused;

	const bool isWeaponVisible = !paused && IsPlayerWeaponVisible(player);
	const UiElementInstanceID weaponImageElementInstID = uiManager.getElementByName(WeaponImageElementName);
	uiManager.setElementActive(weaponImageElementInstID, isWeaponVisible);

	const bool isCompassVisible = !paused && options.getMisc_ShowCompass() && !gameState.isCamping();
	GameWorldUI::setCompassVisible(isCompassVisible);

	GameWorldUI::updateDoorKeys();

	const bool isModernModeReticleVisible = !paused && isModernInterface;
	const UiElementInstanceID modernModeReticleImageElementInstID = uiManager.getElementByName(ModernModeReticleImageElementName);
	uiManager.setElementActive(modernModeReticleImageElementInstID, isModernModeReticleVisible);

	if (paused)
	{
		const UiElementInstanceID triggerTextBoxElementInstID = uiManager.getElementByName(TriggerTextBoxElementName);
		uiManager.setElementActive(triggerTextBoxElementInstID, false);

		const UiElementInstanceID actionTextBoxElementInstID = uiManager.getElementByName(ActionTextBoxElementName);
		uiManager.setElementActive(actionTextBoxElementInstID, false);

		const UiElementInstanceID effectTextBoxElementInstID = uiManager.getElementByName(EffectTextBoxElementName);
		uiManager.setElementActive(effectTextBoxElementInstID, false);

		game.setCursorOverride(std::nullopt);
	}

	if (isModernInterface)
	{
		uiManager.setElementActive(game.cursorImageElementInstID, paused);
		GameWorldUiModel::setFreeLookActive(game, !paused);
	}
}

void GameWorldUI::showTextPopUp(const char *str, const std::string &fontName, TextAlignment alignment, const GameWorldPopUpClosedCallback &callback)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	uiManager.clearContextElements(state.textPopUpContextInstID, inputManager, renderer);

	UiElementInitInfo textPopUpTextBoxElementInitInfo;
	textPopUpTextBoxElementInitInfo.name = "GameWorldTextPopUpTextBox";
	textPopUpTextBoxElementInitInfo.position = GameWorldUiView::getStatusPopUpTextCenterPoint(game);
	textPopUpTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	textPopUpTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo textPopUpTextBoxInitInfo;
	textPopUpTextBoxInitInfo.text = str;
	textPopUpTextBoxInitInfo.fontName = fontName;
	textPopUpTextBoxInitInfo.defaultColor = GameWorldUiView::StatusPopUpTextColor;
	textPopUpTextBoxInitInfo.alignment = alignment;
	textPopUpTextBoxInitInfo.lineSpacing = GameWorldUiView::StatusPopUpTextLineSpacing;
	const UiElementInstanceID textPopUpTextBoxElementInstID = uiManager.createTextBox(textPopUpTextBoxElementInitInfo, textPopUpTextBoxInitInfo, state.textPopUpContextInstID, renderer);
	const Rect textPopUpTextBoxRect = uiManager.getTransformGlobalRect(textPopUpTextBoxElementInstID);

	UiElementInitInfo textPopUpImageElementInitInfo;
	textPopUpImageElementInitInfo.name = "GameWorldTextPopUpImage";
	textPopUpImageElementInitInfo.position = GameWorldUiView::getStatusPopUpTextCenterPoint(game);
	textPopUpImageElementInitInfo.pivotType = UiPivotType::Middle;
	textPopUpImageElementInitInfo.drawOrder = 0;

	const int textPopUpImageTextureWidth = GameWorldUiView::getStatusPopUpTextureWidth(textPopUpTextBoxRect.width);
	const int textPopUpImageTextureHeight = GameWorldUiView::getStatusPopUpTextureHeight(textPopUpTextBoxRect.height);
	const UiTextureID textPopUpImageTextureID = uiManager.getOrAddTexture(GameWorldUiView::StatusPopUpTexturePatternType, textPopUpImageTextureWidth, textPopUpImageTextureHeight, textureManager, renderer);
	uiManager.createImage(textPopUpImageElementInitInfo, textPopUpImageTextureID, state.textPopUpContextInstID, renderer);

	UiElementInitInfo textPopUpBackButtonElementInitInfo;
	textPopUpBackButtonElementInitInfo.name = "GameWorldTextPopUpBackButton";
	textPopUpBackButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	textPopUpBackButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	textPopUpBackButtonElementInitInfo.drawOrder = 2;

	auto popUpButtonCallback = [callback](MouseButtonType)
	{
		callback();
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

	GameWorldUI::onPauseChanged(true);
}

void GameWorldUI::showTextPopUp(const char *str, const std::string &fontName, TextAlignment alignment)
{
	auto callback = []()
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		GameWorldUiController::onPopUpSelected(game);
	};

	GameWorldUI::showTextPopUp(str, fontName, alignment, callback);
}

void GameWorldUI::showLootPopUp(ItemInventory &itemInventory, const GameWorldPopUpClosedCallback &callback)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	uiManager.clearContextElements(state.lootPopUpContextInstID, inputManager, renderer);

	UiElementInitInfo lootPopUpImageElementInitInfo;
	lootPopUpImageElementInitInfo.name = "GameWorldLootPopUpImage";
	lootPopUpImageElementInitInfo.position = Int2(56, 10);

	const TextureAsset lootPopUpTextureAsset = GameWorldUiView::getContainerInventoryTextureAsset();
	const TextureAsset lootPopUpPaletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();
	const UiTextureID lootPopUpTextureID = uiManager.getOrAddTexture(lootPopUpTextureAsset, lootPopUpPaletteTextureAsset, textureManager, renderer);
	uiManager.createImage(lootPopUpImageElementInitInfo, lootPopUpTextureID, state.lootPopUpContextInstID, renderer);

	UiElementInitInfo lootPopUpListBoxElementInitInfo;
	lootPopUpListBoxElementInitInfo.name = "GameWorldLootPopUpListBox";
	lootPopUpListBoxElementInitInfo.position = Int2(85, 34);
	lootPopUpListBoxElementInitInfo.drawOrder = 1;

	const UiListBoxInitInfo lootPopUpListBoxInitInfo = GameWorldUiView::getLootListBoxProperties();
	const UiElementInstanceID listBoxElementInstID = uiManager.createListBox(lootPopUpListBoxElementInitInfo, lootPopUpListBoxInitInfo, state.lootPopUpContextInstID, renderer);

	UiElementInitInfo lootPopUpListBoxUpButtonElementInitInfo;
	lootPopUpListBoxUpButtonElementInitInfo.name = "GameWorldLootPopUpListBoxUpButton";
	lootPopUpListBoxUpButtonElementInitInfo.position = Int2(65, 19);
	lootPopUpListBoxUpButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	lootPopUpListBoxUpButtonElementInitInfo.size = Int2(9, 9);

	UiButtonInitInfo lootPopUpListBoxUpButtonInitInfo;
	lootPopUpListBoxUpButtonInitInfo.callback = [&uiManager, listBoxElementInstID](MouseButtonType) { uiManager.scrollListBoxUp(listBoxElementInstID); };
	uiManager.createButton(lootPopUpListBoxUpButtonElementInitInfo, lootPopUpListBoxUpButtonInitInfo, state.lootPopUpContextInstID);

	UiElementInitInfo lootPopUpListBoxDownButtonElementInitInfo;
	lootPopUpListBoxDownButtonElementInitInfo.name = "GameWorldLootPopUpListBoxDownButton";
	lootPopUpListBoxDownButtonElementInitInfo.position = Int2(65, 92);
	lootPopUpListBoxDownButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	lootPopUpListBoxDownButtonElementInitInfo.size = Int2(9, 9);

	UiButtonInitInfo lootPopUpListBoxDownButtonInitInfo;
	lootPopUpListBoxDownButtonInitInfo.callback = [&uiManager, listBoxElementInstID](MouseButtonType) { uiManager.scrollListBoxDown(listBoxElementInstID); };
	uiManager.createButton(lootPopUpListBoxDownButtonElementInitInfo, lootPopUpListBoxDownButtonInitInfo, state.lootPopUpContextInstID);

	UiElementInitInfo lootPopUpBackButtonElementInitInfo;
	lootPopUpBackButtonElementInitInfo.name = "GameWorldLootPopUpBackButton";
	lootPopUpBackButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	lootPopUpBackButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);

	auto lootPopUpBackButtonCallback = [&state, callback](MouseButtonType)
	{
		callback();
		GameWorldUI::onPauseChanged(false);
	};

	UiButtonInitInfo lootPopUpBackButtonInitInfo;
	lootPopUpBackButtonInitInfo.mouseButtonFlags = MouseButtonTypeFlags(MouseButtonType::Right);
	lootPopUpBackButtonInitInfo.callback = lootPopUpBackButtonCallback;
	uiManager.createButton(lootPopUpBackButtonElementInitInfo, lootPopUpBackButtonInitInfo, state.lootPopUpContextInstID);

	auto lootPopUpBackInputActionCallback = [lootPopUpBackButtonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			lootPopUpBackButtonCallback(MouseButtonType::Right);
		}
	};

	uiManager.addInputActionListener(InputActionName::Back, lootPopUpBackInputActionCallback, ContextName_LootPopUp, inputManager);

	auto lootPopUpMouseWheelScrollChangedCallback = [&uiManager, listBoxElementInstID](Game &game, MouseWheelScrollType type, const Int2 &position)
	{
		if (type == MouseWheelScrollType::Down)
		{
			uiManager.scrollListBoxDown(listBoxElementInstID);
		}
		else if (type == MouseWheelScrollType::Up)
		{
			uiManager.scrollListBoxUp(listBoxElementInstID);
		}
	};

	uiManager.addMouseScrollChangedListener(lootPopUpMouseWheelScrollChangedCallback, ContextName_LootPopUp, inputManager);

	state.lootPopUpItemMappings.clear();
	for (int i = 0; i < itemInventory.getTotalSlotCount(); i++)
	{
		const ItemInstance &itemInst = itemInventory.getSlot(i);
		if (!itemInst.isValid())
		{
			continue;
		}

		const int listBoxItemIndex = uiManager.getListBoxItemCount(listBoxElementInstID);

		std::vector<GameWorldLootUiItemMapping> &itemMappings = state.lootPopUpItemMappings;
		GameWorldLootUiItemMapping itemMapping;
		itemMapping.inventoryItemIndex = i;
		itemMapping.listBoxItemIndex = listBoxItemIndex;
		itemMappings.emplace_back(itemMapping);

		const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
		const ItemDefinition &itemDef = itemLibrary.getDefinition(itemInst.defID);
		std::string itemDisplayName = GetLootItemDisplayNameWithQty(itemDef, itemInst.stackAmount);

		auto listBoxItemCallback = [&game, &itemInventory, &uiManager, &itemLibrary, listBoxElementInstID, lootPopUpBackButtonCallback, listBoxItemIndex, &itemMappings](MouseButtonType)
		{
			// Find which inventory item slot this list box item points to.
			int itemMappingsIndex = -1;
			for (int curItemMappingsIndex = 0; curItemMappingsIndex < static_cast<int>(itemMappings.size()); curItemMappingsIndex++)
			{
				if (itemMappings[curItemMappingsIndex].listBoxItemIndex == listBoxItemIndex)
				{
					itemMappingsIndex = curItemMappingsIndex;
					break;
				}
			}

			DebugAssert(itemMappingsIndex >= 0);
			const int mappedInventoryItemIndex = itemMappings[itemMappingsIndex].inventoryItemIndex;
			if (mappedInventoryItemIndex < 0)
			{
				// This list box item was emptied previously.
				return;
			}

			ItemInstance &selectedItemInst = itemInventory.getSlot(mappedInventoryItemIndex);
			const ItemDefinitionID selectedItemDefID = selectedItemInst.defID;
			DebugAssert(selectedItemDefID >= 0);
			const ItemDefinition &selectedItemDef = itemLibrary.getDefinition(selectedItemDefID);

			Player &player = game.player;
			if (selectedItemDef.type == ItemType::Gold)
			{
				player.gold += selectedItemInst.stackAmount;
			}
			else
			{
				ItemInventory &playerInventory = player.inventory;
				playerInventory.insert(selectedItemDefID, selectedItemInst.stackAmount);
			}

			selectedItemInst.defID = -1;

			if (itemInventory.getOccupiedSlotCount() == 0)
			{
				lootPopUpBackButtonCallback(MouseButtonType::Right);
			}

			// Shift mappings forward by one.
			for (int curItemMappingsIndex = itemMappingsIndex; curItemMappingsIndex < static_cast<int>(itemMappings.size()); curItemMappingsIndex++)
			{
				GameWorldLootUiItemMapping &curItemMapping = itemMappings[curItemMappingsIndex];

				const int nextItemMappingsIndex = curItemMappingsIndex + 1;
				if (nextItemMappingsIndex < static_cast<int>(itemMappings.size()))
				{
					GameWorldLootUiItemMapping &nextItemMapping = itemMappings[nextItemMappingsIndex];
					curItemMapping.inventoryItemIndex = nextItemMapping.inventoryItemIndex;
				}
				else
				{
					curItemMapping.inventoryItemIndex = -1;
				}

				std::string newListBoxItemText;
				if (curItemMapping.inventoryItemIndex >= 0)
				{
					const ItemInstance &curItemInst = itemInventory.getSlot(curItemMapping.inventoryItemIndex);
					const ItemDefinition &curItemDef = itemLibrary.getDefinition(curItemInst.defID);
					newListBoxItemText = GetLootItemDisplayNameWithQty(curItemDef, curItemInst.stackAmount);
				}

				uiManager.setListBoxItemText(listBoxElementInstID, curItemMapping.listBoxItemIndex, newListBoxItemText.c_str());
			}
		};

		UiListBoxItem listBoxItem;
		listBoxItem.text = std::move(itemDisplayName);
		listBoxItem.callback = listBoxItemCallback;
		uiManager.insertBackListBoxItem(listBoxElementInstID, std::move(listBoxItem));
	}

	// @todo sword cursor

	uiManager.setContextEnabled(state.lootPopUpContextInstID, true);

	GameWorldUI::onPauseChanged(true);
}

void GameWorldUI::showCampModal()
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	uiManager.clearContextElements(state.campModalContextInstID, inputManager, renderer);

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

	constexpr int campModalImageTextureWidth = 156;
	constexpr int campModalImageTextureHeight = 24;
	const UiTextureID campModalImageTextureID = uiManager.getOrAddTexture(UiTexturePatternType::Dark, campModalImageTextureWidth, campModalImageTextureHeight, textureManager, renderer);

	constexpr Color campModalTextColor(215, 158, 4);

	UiElementInitInfo campModalTitleImageElementInitInfo;
	campModalTitleImageElementInitInfo.name = "GameWorldCampModalTitleImage";
	campModalTitleImageElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, 80);
	campModalTitleImageElementInitInfo.pivotType = UiPivotType::Middle;
	uiManager.createImage(campModalTitleImageElementInitInfo, campModalImageTextureID, state.campModalContextInstID, renderer);

	UiElementInitInfo campModalTitleTextBoxElementInitInfo;
	campModalTitleTextBoxElementInitInfo.name = "GameWorldCampModalTitleTextBox";
	campModalTitleTextBoxElementInitInfo.position = campModalTitleImageElementInitInfo.position;
	campModalTitleTextBoxElementInitInfo.pivotType = UiPivotType::Middle;

	UiTextBoxInitInfo campModalTitleTextBoxInitInfo;
	campModalTitleTextBoxInitInfo.text = GameWorldUiModel::getCampModalTitleText(exeData);
	campModalTitleTextBoxInitInfo.fontName = ArenaFontName::A;
	campModalTitleTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
	campModalTitleTextBoxInitInfo.defaultColor = campModalTextColor;
	campModalTitleTextBoxInitInfo.tabColorPaletteID = GameWorldUiView::getCampModalTextBoxPaletteID(textureManager);
	uiManager.createTextBox(campModalTitleTextBoxElementInitInfo, campModalTitleTextBoxInitInfo, state.campModalContextInstID, renderer);

	UiElementInitInfo campModalManualHoursImageElementInitInfo;
	campModalManualHoursImageElementInitInfo.name = "GameWorldCampModalManualHoursImage";
	campModalManualHoursImageElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, campModalTitleImageElementInitInfo.position.y + campModalImageTextureHeight);
	campModalManualHoursImageElementInitInfo.pivotType = UiPivotType::Middle;
	uiManager.createImage(campModalManualHoursImageElementInitInfo, campModalImageTextureID, state.campModalContextInstID, renderer);

	UiElementInitInfo campModalManualHoursTextBoxElementInitInfo;
	campModalManualHoursTextBoxElementInitInfo.name = "GameWorldCampModalManualHoursTextBox";
	campModalManualHoursTextBoxElementInitInfo.position = campModalManualHoursImageElementInitInfo.position;
	campModalManualHoursTextBoxElementInitInfo.pivotType = UiPivotType::Middle;

	UiTextBoxInitInfo campModalManualHoursTextBoxInitInfo;
	campModalManualHoursTextBoxInitInfo.text = GameWorldUiModel::getCampModalManualHoursText(exeData);
	campModalManualHoursTextBoxInitInfo.fontName = ArenaFontName::A;
	campModalManualHoursTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
	campModalManualHoursTextBoxInitInfo.defaultColor = campModalTextColor;
	campModalManualHoursTextBoxInitInfo.tabColorPaletteID = campModalTitleTextBoxInitInfo.tabColorPaletteID;
	uiManager.createTextBox(campModalManualHoursTextBoxElementInitInfo, campModalManualHoursTextBoxInitInfo, state.campModalContextInstID, renderer); 

	UiElementInitInfo campModalManualHoursButtonElementInitInfo;
	campModalManualHoursButtonElementInitInfo.name = "GameWorldCampModalManualHoursButton";
	campModalManualHoursButtonElementInitInfo.position = campModalManualHoursImageElementInitInfo.position;
	campModalManualHoursButtonElementInitInfo.pivotType = UiPivotType::Middle;

	auto campModalManualHoursButtonCallback = [](MouseButtonType)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		game.uiManager.disableTopMostContext();
		game.inputManager.setInputActionMapActive(InputActionMapName::Camping, false);
		GameWorldUI::showCampManualHoursModal();
	};

	UiButtonInitInfo campModalManualHoursButtonInitInfo;
	campModalManualHoursButtonInitInfo.callback = campModalManualHoursButtonCallback;
	campModalManualHoursButtonInitInfo.contentElementName = campModalManualHoursImageElementInitInfo.name;
	uiManager.createButton(campModalManualHoursButtonElementInitInfo, campModalManualHoursButtonInitInfo, state.campModalContextInstID);

	UiElementInitInfo campModalUntilHealedImageElementInitInfo;
	campModalUntilHealedImageElementInitInfo.name = "GameWorldCampModalUntilHealedImage";
	campModalUntilHealedImageElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, campModalManualHoursImageElementInitInfo.position.y + campModalImageTextureHeight);
	campModalUntilHealedImageElementInitInfo.pivotType = UiPivotType::Middle;
	uiManager.createImage(campModalUntilHealedImageElementInitInfo, campModalImageTextureID, state.campModalContextInstID, renderer);

	UiElementInitInfo campModalUntilHealedTextBoxElementInitInfo;
	campModalUntilHealedTextBoxElementInitInfo.name = "GameWorldCampModalUntilHealedTextBox";
	campModalUntilHealedTextBoxElementInitInfo.position = campModalUntilHealedImageElementInitInfo.position;
	campModalUntilHealedTextBoxElementInitInfo.pivotType = UiPivotType::Middle;

	UiTextBoxInitInfo campModalUntilHealedTextBoxInitInfo;
	campModalUntilHealedTextBoxInitInfo.text = GameWorldUiModel::getCampModalUntilHealedText(exeData);
	campModalUntilHealedTextBoxInitInfo.fontName = ArenaFontName::A;
	campModalUntilHealedTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
	campModalUntilHealedTextBoxInitInfo.defaultColor = campModalTextColor;
	campModalUntilHealedTextBoxInitInfo.tabColorPaletteID = campModalManualHoursTextBoxInitInfo.tabColorPaletteID;
	uiManager.createTextBox(campModalUntilHealedTextBoxElementInitInfo, campModalUntilHealedTextBoxInitInfo, state.campModalContextInstID, renderer);

	UiElementInitInfo campModalUntilHealedButtonElementInitInfo;
	campModalUntilHealedButtonElementInitInfo.name = "GameWorldCampModalUntilHealedButton";
	campModalUntilHealedButtonElementInitInfo.position = campModalUntilHealedImageElementInitInfo.position;
	campModalUntilHealedButtonElementInitInfo.pivotType = UiPivotType::Middle;

	auto campModalUntilHealedButtonCallback = [&exeData](MouseButtonType)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		game.uiManager.disableTopMostContext();
		game.inputManager.setInputActionMapActive(InputActionMapName::Camping, false);
		
		Player &player = game.player;
		if (!player.canRestUntilHealed())
		{
			GameWorldUI::showTextPopUp(exeData.camping.alreadyFullyRested.c_str(), ArenaFontName::A, GameWorldUiView::StatusPopUpTextAlignment);
			return;
		}

		GameState &gameState = game.gameState;
		gameState.setCampingUntilHealed();

		GameWorldUI::onPauseChanged(false);
	};

	UiButtonInitInfo campModalUntilHealedButtonInitInfo;
	campModalUntilHealedButtonInitInfo.callback = campModalUntilHealedButtonCallback;
	campModalUntilHealedButtonInitInfo.contentElementName = campModalUntilHealedImageElementInitInfo.name;
	uiManager.createButton(campModalUntilHealedButtonElementInitInfo, campModalUntilHealedButtonInitInfo, state.campModalContextInstID);

	UiElementInitInfo campModalBackButtonElementInitInfo;
	campModalBackButtonElementInitInfo.name = "GameWorldCampModalBackButton";
	campModalBackButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	campModalBackButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);

	auto campModalBackButtonCallback = [&state](MouseButtonType)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		game.uiManager.disableTopMostContext();
		game.inputManager.setInputActionMapActive(InputActionMapName::Camping, false);
		GameWorldUI::onPauseChanged(false);
	};

	UiButtonInitInfo campModalBackButtonInitInfo;
	campModalBackButtonInitInfo.mouseButtonFlags = MouseButtonTypeFlags(MouseButtonType::Right);
	campModalBackButtonInitInfo.callback = campModalBackButtonCallback;
	uiManager.createButton(campModalBackButtonElementInitInfo, campModalBackButtonInitInfo, state.campModalContextInstID);

	auto campModalBackInputActionCallback = [campModalBackButtonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			campModalBackButtonCallback(MouseButtonType::Right);
		}
	};

	auto campModalManualHoursInputActionCallback = [campModalManualHoursButtonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			campModalManualHoursButtonCallback(MouseButtonType::Right);
		}
	};

	auto campModalUntilHealedInputActionCallback = [campModalUntilHealedButtonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			campModalUntilHealedButtonCallback(MouseButtonType::Right);
		}
	};

	uiManager.addInputActionListener(InputActionName::Back, campModalBackInputActionCallback, ContextName_CampModal, inputManager);
	uiManager.addInputActionListener(InputActionName::CampManualHours, campModalManualHoursInputActionCallback, ContextName_CampModal, inputManager);
	uiManager.addInputActionListener(InputActionName::CampUntilHealed, campModalUntilHealedInputActionCallback, ContextName_CampModal, inputManager);

	uiManager.setContextEnabled(state.campModalContextInstID, true);

	inputManager.setInputActionMapActive(InputActionMapName::Camping, true);

	GameWorldUI::onPauseChanged(true);
}

void GameWorldUI::showCampManualHoursModal()
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	uiManager.clearContextElements(state.campManualHoursModalContextInstID, inputManager, renderer);
	state.campManualHoursInputText.clear();

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

	constexpr int imageTextureWidth = 231;
	constexpr int imageTextureHeight = 30;
	const UiTextureID imageTextureID = uiManager.getOrAddTexture(UiTexturePatternType::Dark, imageTextureWidth, imageTextureHeight, textureManager, renderer);

	const int imageY = game.options.getGraphics_ModernInterface() ? (ArenaRenderUtils::SCREEN_HEIGHT / 2) : (ArenaRenderUtils::SCENE_VIEW_HEIGHT / 2);

	UiElementInitInfo imageElementInitInfo;
	imageElementInitInfo.name = "GameWorldCampManualHoursModalImage";
	imageElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, imageY);
	imageElementInitInfo.pivotType = UiPivotType::Middle;
	const UiElementInstanceID imageElementInstID = uiManager.createImage(imageElementInitInfo, imageTextureID, state.campManualHoursModalContextInstID, renderer);
	const Rect imageGlobalRect = uiManager.getTransformGlobalRect(imageElementInstID);

	UiElementInitInfo textBoxElementInitInfo;
	textBoxElementInitInfo.name = "GameWorldCampManualHoursModalTextBox";
	textBoxElementInitInfo.position = imageGlobalRect.getTopLeft() + Int2(13, 11);
	textBoxElementInitInfo.pivotType = UiPivotType::TopLeft;

	UiTextBoxInitInfo textBoxInitInfo;
	textBoxInitInfo.text = GameWorldUiModel::getCampManualHoursModalText(exeData);
	textBoxInitInfo.fontName = ArenaFontName::Arena;
	textBoxInitInfo.defaultColor = Color(231, 215, 0);
	const UiElementInstanceID textBoxElementInstID = uiManager.createTextBox(textBoxElementInitInfo, textBoxInitInfo, state.campManualHoursModalContextInstID, renderer);
	const Rect textBoxGlobalRect = uiManager.getTransformGlobalRect(textBoxElementInstID);

	UiElementInitInfo inputTextBoxElementInitInfo;
	inputTextBoxElementInitInfo.name = "GameWorldCampManualHoursModalInputTextBox";
	inputTextBoxElementInitInfo.position = textBoxGlobalRect.getTopRight();
	inputTextBoxElementInitInfo.pivotType = UiPivotType::TopLeft;

	UiTextBoxInitInfo inputTextBoxInitInfo;
	inputTextBoxInitInfo.worstCaseText = TextRenderUtils::makeWorstCaseText(3);
	inputTextBoxInitInfo.fontName = ArenaFontName::Arena;
	inputTextBoxInitInfo.defaultColor = textBoxInitInfo.defaultColor;
	const UiElementInstanceID inputTextBoxElementInstID = uiManager.createTextBox(inputTextBoxElementInitInfo, inputTextBoxInitInfo, state.campManualHoursModalContextInstID, renderer);

	UiElementInitInfo backButtonElementInitInfo;
	backButtonElementInitInfo.name = "GameWorldCampManualHoursModalBackButton";
	backButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	backButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);

	auto backButtonCallback = [&state](MouseButtonType)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		game.uiManager.disableTopMostContext();
		game.inputManager.setTextInputMode(false);
		GameWorldUI::onPauseChanged(false);
	};

	UiButtonInitInfo backButtonInitInfo;
	backButtonInitInfo.mouseButtonFlags = MouseButtonTypeFlags(MouseButtonType::Right);
	backButtonInitInfo.callback = backButtonCallback;
	uiManager.createButton(backButtonElementInitInfo, backButtonInitInfo, state.campManualHoursModalContextInstID);

	auto backInputActionCallback = [backButtonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			backButtonCallback(MouseButtonType::Right);
		}
	};

	auto acceptInputActionCallback = [backButtonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			backButtonCallback(MouseButtonType::Right);

			GameWorldUiState &state = GameWorldUI::state;
			Game &game = *state.game;
			const std::string &inputText = state.campManualHoursInputText;
			if (inputText.empty())
			{
				return;
			}

			int hoursCount = 0;
			try
			{
				size_t index = 0;
				hoursCount = std::stoi(inputText, &index);
				if (index != inputText.size())
				{
					return;
				}
			}
			catch (std::exception)
			{
				return;
			}

			if (hoursCount > 0)
			{
				GameState &gameState = game.gameState;
				gameState.setCampingManualHours(hoursCount);
			}
		}
	};

	auto textInputCallback = [inputTextBoxElementInstID](const std::string_view text)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		UiManager &uiManager = game.uiManager;

		std::string &inputText = state.campManualHoursInputText;
		auto isCharAllowed = [](char c)
		{
			return (c >= '0') && (c <= '9'); // Numbers only
		};

		constexpr int maxCampHourDigits = 3;
		if (TextEntry::append(inputText, text, isCharAllowed, maxCampHourDigits))
		{
			uiManager.setTextBoxText(inputTextBoxElementInstID, inputText.c_str());
		}
	};

	auto textBackspaceCallback = [inputTextBoxElementInstID](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			GameWorldUiState &state = GameWorldUI::state;
			Game &game = *state.game;
			UiManager &uiManager = game.uiManager;

			std::string &inputText = state.campManualHoursInputText;
			if (TextEntry::backspace(inputText))
			{
				uiManager.setTextBoxText(inputTextBoxElementInstID, inputText.c_str());
			}
		}
	};

	uiManager.addInputActionListener(InputActionName::Back, backInputActionCallback, ContextName_CampManualHoursModal, inputManager);
	uiManager.addInputActionListener(InputActionName::Accept, acceptInputActionCallback, ContextName_CampManualHoursModal, inputManager);
	uiManager.addTextInputListener(textInputCallback, ContextName_CampManualHoursModal, inputManager);
	uiManager.addInputActionListener(InputActionName::Backspace, textBackspaceCallback, ContextName_CampManualHoursModal, inputManager);

	uiManager.setContextEnabled(state.campManualHoursModalContextInstID, true);

	inputManager.setTextInputMode(true);
}

void GameWorldUI::showPlayerHurt()
{
	GameWorldUiState &state = GameWorldUI::state;
	state.playerHurtRemainingSeconds = 1.0 / ArenaRenderUtils::FRAMES_PER_SECOND;
}

void GameWorldUI::showConversationMessageBox(ConversationMessageBoxType messageBoxType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	uiManager.clearContextElements(state.conversationModalContextInstID, inputManager, renderer);
	
	const GameState &gameState = game.gameState;
	const ArenaBuildingType buildingType = gameState.getBuildingType();
	const Player &player = game.player;
	const CharacterClassDefinition &playerCharClassDef = CharacterClassLibrary::getInstance().getDefinition(player.charClassDefID);
	const bool canCastMagic = playerCharClassDef.castsMagic;
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

	auto closeConversationCallback = [](MouseButtonType)
	{
		GameWorldUI::closeConversation();
	};

	std::string messageBoxTitleText;
	int messageBoxButtonCount = 0;
	constexpr int messageBoxMaxButtonCount = 5;
	UiButtonCallback messageBoxButtonCallbacks[messageBoxMaxButtonCount];
	std::string messageBoxButtonTexts[messageBoxMaxButtonCount];

	switch (messageBoxType)
	{
	case ConversationMessageBoxType::Citizen:
		messageBoxTitleText = exeData.services.citizenModalTitle;
		messageBoxButtonCount = 4;
		messageBoxButtonCallbacks[0] = GameWorldUI::onNpcWhoAreYouButtonSelected;
		messageBoxButtonCallbacks[1] = GameWorldUI::onNpcWhereIsButtonSelected;
		messageBoxButtonCallbacks[2] = GameWorldUI::onNpcRumorsButtonSelected;
		messageBoxButtonCallbacks[3] = closeConversationCallback;
		messageBoxButtonTexts[0] = exeData.services.citizenModalWhoAreYou;
		messageBoxButtonTexts[1] = exeData.services.citizenModalWhereIs;
		messageBoxButtonTexts[2] = exeData.services.citizenModalRumors;
		messageBoxButtonTexts[3] = exeData.services.citizenModalExit;
		break;
	case ConversationMessageBoxType::CitizenRumors:
		messageBoxTitleText = exeData.services.citizenRumorsModalTitle;
		messageBoxButtonCount = 2;
		messageBoxButtonCallbacks[0] = GameWorldUI::onNpcRumorsGeneralButtonSelected;
		messageBoxButtonCallbacks[1] = GameWorldUI::onNpcRumorsWorkButtonSelected;
		messageBoxButtonTexts[0] = exeData.services.citizenRumorsModalGeneral;
		messageBoxButtonTexts[1] = exeData.services.citizenRumorsModalWork;
		break;
	case ConversationMessageBoxType::Equipment:
		messageBoxTitleText = exeData.services.equipmentModalTitle;
		messageBoxButtonCount = 5;
		messageBoxButtonCallbacks[0] = GameWorldUI::onNpcEquipmentBuyButtonSelected;
		messageBoxButtonCallbacks[1] = GameWorldUI::onNpcEquipmentSellButtonSelected;
		messageBoxButtonCallbacks[2] = GameWorldUI::onNpcEquipmentRepairButtonSelected;
		messageBoxButtonCallbacks[3] = GameWorldUI::onNpcEquipmentStealButtonSelected;
		messageBoxButtonCallbacks[4] = closeConversationCallback;
		messageBoxButtonTexts[0] = exeData.services.equipmentModalBuy;
		messageBoxButtonTexts[1] = exeData.services.equipmentModalSell;
		messageBoxButtonTexts[2] = exeData.services.equipmentModalRepair;
		messageBoxButtonTexts[3] = exeData.services.equipmentModalSteal;
		messageBoxButtonTexts[4] = exeData.services.equipmentModalExit;
		break;
	case ConversationMessageBoxType::EquipmentBuyItem:
		messageBoxTitleText = exeData.services.equipmentBuyModalTitle;
		messageBoxButtonCount = 2;
		messageBoxButtonCallbacks[0] = GameWorldUI::onNpcEquipmentBuyWeaponsButtonSelected;
		messageBoxButtonCallbacks[1] = GameWorldUI::onNpcEquipmentBuyArmorButtonSelected;
		messageBoxButtonTexts[0] = exeData.services.equipmentBuyModalWeapons;
		messageBoxButtonTexts[1] = exeData.services.equipmentBuyModalArmor;
		break;
	case ConversationMessageBoxType::MagesGuild:
		messageBoxTitleText = exeData.services.magesGuildModalTitle;

		if (canCastMagic)
		{
			messageBoxButtonCount = 5;
			messageBoxButtonCallbacks[0] = GameWorldUI::onNpcMagesGuildBuyButtonSelected;
			messageBoxButtonCallbacks[1] = GameWorldUI::onNpcMagesGuildDetectMagicButtonSelected;
			messageBoxButtonCallbacks[2] = GameWorldUI::onNpcMagesGuildSpellmakerButtonSelected;
			messageBoxButtonCallbacks[3] = GameWorldUI::onNpcMagesGuildStealButtonSelected;
			messageBoxButtonCallbacks[4] = closeConversationCallback;
			messageBoxButtonTexts[0] = exeData.services.magesGuildModalBuy;
			messageBoxButtonTexts[1] = exeData.services.magesGuildModalDetectMagic;
			messageBoxButtonTexts[2] = exeData.services.magesGuildModalSpellmaker;
			messageBoxButtonTexts[3] = exeData.services.magesGuildModalSteal;
			messageBoxButtonTexts[4] = exeData.services.magesGuildModalExit;
		}
		else
		{
			messageBoxButtonCount = 4;
			messageBoxButtonCallbacks[0] = GameWorldUI::onNpcMagesGuildBuyButtonSelected;
			messageBoxButtonCallbacks[1] = GameWorldUI::onNpcMagesGuildDetectMagicButtonSelected;
			messageBoxButtonCallbacks[2] = GameWorldUI::onNpcMagesGuildStealButtonSelected;
			messageBoxButtonCallbacks[3] = closeConversationCallback;
			messageBoxButtonTexts[0] = exeData.services.magesGuildModalBuy;
			messageBoxButtonTexts[1] = exeData.services.magesGuildModalDetectMagic;
			messageBoxButtonTexts[2] = exeData.services.magesGuildModalSteal;
			messageBoxButtonTexts[3] = exeData.services.magesGuildModalExit;
		}

		break;
	case ConversationMessageBoxType::MagesGuildBuyItem:
		messageBoxTitleText = exeData.services.magesGuildPickItemModalTitle;
		messageBoxButtonCount = 3;
		messageBoxButtonCallbacks[0] = GameWorldUI::onNpcMagesGuildBuyPotionsButtonSelected;
		messageBoxButtonCallbacks[1] = GameWorldUI::onNpcMagesGuildBuyMagicItemsButtonSelected;
		messageBoxButtonCallbacks[2] = GameWorldUI::onNpcMagesGuildBuySpellsButtonSelected;
		messageBoxButtonTexts[0] = exeData.services.magesGuildPickItemModalPotions;
		messageBoxButtonTexts[1] = exeData.services.magesGuildPickItemModalMagicItems;
		messageBoxButtonTexts[2] = exeData.services.magesGuildPickItemModalSpells;
		break;
	case ConversationMessageBoxType::MagesGuildSteal:
		messageBoxTitleText = exeData.services.magesGuildPickItemModalTitle;
		messageBoxButtonCount = 2;
		messageBoxButtonCallbacks[0] = GameWorldUI::onNpcMagesGuildStealPotionsButtonSelected;
		messageBoxButtonCallbacks[1] = GameWorldUI::onNpcMagesGuildStealMagicItemsButtonSelected;
		messageBoxButtonTexts[0] = exeData.services.magesGuildPickItemModalPotions;
		messageBoxButtonTexts[1] = exeData.services.magesGuildPickItemModalMagicItems;
		break;
	case ConversationMessageBoxType::Tavern:
	{
		messageBoxTitleText = exeData.services.tavernModalTitle;

		const bool isRoomRented = false; // @todo query some TavernRoomState in GameState eventually
		if (isRoomRented)
		{
			messageBoxButtonCount = 3;
			messageBoxButtonCallbacks[0] = GameWorldUI::onNpcTavernBuyDrinksButtonSelected;
			messageBoxButtonCallbacks[1] = GameWorldUI::onNpcTavernRumorsButtonSelected;
			messageBoxButtonCallbacks[2] = closeConversationCallback;
			messageBoxButtonTexts[0] = exeData.services.tavernModalBuyDrinks;
			messageBoxButtonTexts[1] = exeData.services.tavernModalRumors;
			messageBoxButtonTexts[2] = exeData.services.tavernModalExit;
		}
		else
		{
			messageBoxButtonCount = 5;
			messageBoxButtonCallbacks[0] = GameWorldUI::onNpcTavernBuyDrinksButtonSelected;
			messageBoxButtonCallbacks[1] = GameWorldUI::onNpcTavernGetARoomButtonSelected;
			messageBoxButtonCallbacks[2] = GameWorldUI::onNpcTavernSneakIntoARoomButtonSelected;
			messageBoxButtonCallbacks[3] = GameWorldUI::onNpcTavernRumorsButtonSelected;
			messageBoxButtonCallbacks[4] = closeConversationCallback;
			messageBoxButtonTexts[0] = exeData.services.tavernModalBuyDrinks;
			messageBoxButtonTexts[1] = exeData.services.tavernModalGetARoom;
			messageBoxButtonTexts[2] = exeData.services.tavernModalSneakIntoARoom;
			messageBoxButtonTexts[3] = exeData.services.tavernModalRumors;
			messageBoxButtonTexts[4] = exeData.services.tavernModalExit;
		}

		break;
	}
	case ConversationMessageBoxType::TavernRumors:
		messageBoxTitleText = exeData.services.citizenRumorsModalTitle;
		messageBoxButtonCount = 2;
		messageBoxButtonCallbacks[0] = GameWorldUI::onNpcTavernRumorsGeneralButtonSelected;
		messageBoxButtonCallbacks[1] = GameWorldUI::onNpcTavernRumorsWorkButtonSelected;
		messageBoxButtonTexts[0] = exeData.services.citizenRumorsModalGeneral;
		messageBoxButtonTexts[1] = exeData.services.citizenRumorsModalWork;
		break;
	case ConversationMessageBoxType::Temple:
		messageBoxTitleText = exeData.services.templeModalTitle;
		messageBoxButtonCount = 4;
		messageBoxButtonCallbacks[0] = GameWorldUI::onNpcTempleBlessButtonSelected;
		messageBoxButtonCallbacks[1] = GameWorldUI::onNpcTempleCureButtonSelected;
		messageBoxButtonCallbacks[2] = GameWorldUI::onNpcTempleHealButtonSelected;
		messageBoxButtonCallbacks[3] = closeConversationCallback;
		messageBoxButtonTexts[0] = exeData.services.templeModalBless;
		messageBoxButtonTexts[1] = exeData.services.templeModalCure;
		messageBoxButtonTexts[2] = exeData.services.templeModalHeal;
		messageBoxButtonTexts[3] = exeData.services.templeModalExit;
		break;
	default:
		DebugNotImplemented();
		break;
	}

	const int titleImageTextureHeight = 24;
	const int totalMessageBoxHeight = titleImageTextureHeight * (messageBoxButtonCount + 1);
	const int titleTextBoxPositionY = (ArenaRenderUtils::SCREEN_HEIGHT / 2) - (totalMessageBoxHeight / 2) + (titleImageTextureHeight / 2);
	const PaletteID paletteID = *textureManager.tryGetPaletteID(GameWorldUiView::getPaletteTextureAsset());

	UiElementInitInfo titleTextBoxElementInitInfo;
	titleTextBoxElementInitInfo.name = "GameWorldConversationModalTitleTextBox";
	titleTextBoxElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, titleTextBoxPositionY);
	titleTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	titleTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo titleTextBoxInitInfo;
	titleTextBoxInitInfo.text = messageBoxTitleText;
	titleTextBoxInitInfo.fontName = ArenaFontName::A;
	titleTextBoxInitInfo.tabColorPaletteID = paletteID;
	titleTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
	const UiElementInstanceID titleTextBoxElementInstID = uiManager.createTextBox(titleTextBoxElementInitInfo, titleTextBoxInitInfo, state.conversationModalContextInstID, renderer);
	const Rect titleTextBoxGlobalRect = uiManager.getTransformGlobalRect(titleTextBoxElementInstID);

	int widestButtonTextBoxWidth = titleTextBoxGlobalRect.width;
	for (int i = 0; i < messageBoxButtonCount; i++)
	{
		UiElementInitInfo messageBoxButtonTextBoxElementInitInfo;
		messageBoxButtonTextBoxElementInitInfo.name = "GameWorldConversationModalButton" + std::to_string(i) + "TextBox";
		messageBoxButtonTextBoxElementInitInfo.position = titleTextBoxElementInitInfo.position + Int2(0, titleImageTextureHeight * (i + 1));
		messageBoxButtonTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
		messageBoxButtonTextBoxElementInitInfo.drawOrder = 1;

		UiTextBoxInitInfo messageBoxButtonTextBoxInitInfo;
		messageBoxButtonTextBoxInitInfo.text = messageBoxButtonTexts[i];
		messageBoxButtonTextBoxInitInfo.fontName = ArenaFontName::A;
		messageBoxButtonTextBoxInitInfo.tabColorPaletteID = paletteID;
		messageBoxButtonTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
		const UiElementInstanceID messageBoxButtonTextBoxElementInstID = uiManager.createTextBox(messageBoxButtonTextBoxElementInitInfo, messageBoxButtonTextBoxInitInfo, state.conversationModalContextInstID, renderer);
		const Rect messageBoxButtonTextBoxGlobalRect = uiManager.getTransformGlobalRect(messageBoxButtonTextBoxElementInstID);
		widestButtonTextBoxWidth = std::max(widestButtonTextBoxWidth, messageBoxButtonTextBoxGlobalRect.width);
	}

	UiElementInitInfo titleImageElementInitInfo;
	titleImageElementInitInfo.name = "GameWorldConversationModalTitleImage";
	titleImageElementInitInfo.position = titleTextBoxElementInitInfo.position;
	titleImageElementInitInfo.sizeType = UiTransformSizeType::Manual;
	titleImageElementInitInfo.size = Int2(widestButtonTextBoxWidth + 14, titleImageTextureHeight);
	titleImageElementInitInfo.pivotType = UiPivotType::Middle;

	const UiTextureID titleImageTextureID = uiManager.getOrAddTexture(UiTexturePatternType::Dark, titleImageElementInitInfo.size.x, titleImageElementInitInfo.size.y, textureManager, renderer);
	uiManager.createImage(titleImageElementInitInfo, titleImageTextureID, state.conversationModalContextInstID, renderer);

	for (int i = 0; i < messageBoxButtonCount; i++)
	{
		UiElementInitInfo messageBoxButtonImageElementInitInfo;
		messageBoxButtonImageElementInitInfo.name = "GameWorldConversationModalButton" + std::to_string(i) + "Image";
		messageBoxButtonImageElementInitInfo.position = titleTextBoxElementInitInfo.position + Int2(0, titleImageTextureHeight * (i + 1));
		messageBoxButtonImageElementInitInfo.sizeType = UiTransformSizeType::Manual;
		messageBoxButtonImageElementInitInfo.size = titleImageElementInitInfo.size;
		messageBoxButtonImageElementInitInfo.pivotType = UiPivotType::Middle;
		uiManager.createImage(messageBoxButtonImageElementInitInfo, titleImageTextureID, state.conversationModalContextInstID, renderer);

		UiElementInitInfo messageBoxButtonElementInitInfo;
		messageBoxButtonElementInitInfo.name = "GameWorldConversationModalButton" + std::to_string(i);
		messageBoxButtonElementInitInfo.position = messageBoxButtonImageElementInitInfo.position;
		messageBoxButtonElementInitInfo.pivotType = UiPivotType::Middle;

		UiButtonInitInfo messageBoxButtonInitInfo;
		messageBoxButtonInitInfo.callback = messageBoxButtonCallbacks[i];
		messageBoxButtonInitInfo.contentElementName = messageBoxButtonImageElementInitInfo.name;
		uiManager.createButton(messageBoxButtonElementInitInfo, messageBoxButtonInitInfo, state.conversationModalContextInstID);
	}

	UiElementInitInfo backButtonElementInitInfo;
	backButtonElementInitInfo.name = "GameWorldConversationModalBackButton";
	backButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	backButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);

	UiButtonInitInfo backButtonInitInfo;
	backButtonInitInfo.mouseButtonFlags = MouseButtonTypeFlags(MouseButtonType::Right);
	backButtonInitInfo.callback = closeConversationCallback;
	uiManager.createButton(backButtonElementInitInfo, backButtonInitInfo, state.conversationModalContextInstID);

	auto backInputActionCallback = [closeConversationCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			closeConversationCallback(MouseButtonType::Right);
		}
	};

	uiManager.addInputActionListener(InputActionName::Back, backInputActionCallback, ContextName_ConversationModal, inputManager);

	uiManager.setContextEnabled(state.conversationModalContextInstID, true);

	const bool isPaused = !game.shouldSimulateScene;
	if (!isPaused)
	{
		GameWorldUI::onPauseChanged(true);
	}
}

void GameWorldUI::showConversationListBox(ConversationListBoxType listBoxType)
{
	// @todo
}

void GameWorldUI::closeConversation()
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.setContextEnabled(state.conversationModalContextInstID, false);
	uiManager.setContextEnabled(state.shopkeeperBgContextInstID, false);
	GameWorldUI::onPauseChanged(false);
}

void GameWorldUI::onNpcWhoAreYouButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO Who am I";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Citizen);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcWhereIsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO Where is... (list box)";

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft);
}

void GameWorldUI::onNpcRumorsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::CitizenRumors);
}

void GameWorldUI::onNpcRumorsGeneralButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO Rumors general";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Citizen);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcRumorsWorkButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO Rumors work";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Citizen);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcEquipmentBuyButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::EquipmentBuyItem);
}

void GameWorldUI::onNpcEquipmentSellButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO Sell items (listbox)";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Equipment);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcEquipmentRepairButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO Repair (list box)";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Equipment);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcEquipmentStealButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	Random &random = game.random;
	const bool isStealSuccessful = random.nextBool();

	std::string text;
	GameWorldPopUpClosedCallback callback;
	if (isStealSuccessful)
	{
		text = "TODO steal success";
		callback = [&uiManager]()
		{
			uiManager.disableTopMostContext();
			GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Equipment);
		};
	}
	else
	{
		text = "TODO steal failure";
		callback = [&game, &uiManager]()
		{
			uiManager.disableTopMostContext();
			GameWorldUI::closeConversation();

			GameState &gameState = game.gameState;
			gameState.queueCityGuardEncounter(game);
		};
	}

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcEquipmentBuyWeaponsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO buy weapons (listbox)";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Equipment);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcEquipmentBuyArmorButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO buy armor (listbox)";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Equipment);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcMagesGuildBuyButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::MagesGuildBuyItem);
}

void GameWorldUI::onNpcMagesGuildDetectMagicButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO detect magic";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::MagesGuild);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcMagesGuildSpellmakerButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO spellmaker";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::MagesGuild);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcMagesGuildStealButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::MagesGuildSteal);
}

void GameWorldUI::onNpcMagesGuildBuyPotionsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO buy potions (listbox)";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::MagesGuild);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcMagesGuildBuyMagicItemsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO buy magic items (listbox)";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::MagesGuild);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcMagesGuildBuySpellsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO buy spells (listbox)";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::MagesGuild);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcMagesGuildStealPotionsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	Random &random = game.random;
	const bool isStealSuccessful = random.nextBool();

	std::string text;
	GameWorldPopUpClosedCallback callback;
	if (isStealSuccessful)
	{
		text = "TODO steal potions success";
		callback = [&uiManager]()
		{
			uiManager.disableTopMostContext();
			GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::MagesGuild);
		};
	}
	else
	{
		text = "TODO steal potions failure";
		callback = [&game, &uiManager]()
		{
			uiManager.disableTopMostContext();
			GameWorldUI::closeConversation();

			GameState &gameState = game.gameState;
			gameState.queueCityGuardEncounter(game);
		};
	}

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcMagesGuildStealMagicItemsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	Random &random = game.random;
	const bool isStealSuccessful = random.nextBool();

	std::string text;
	GameWorldPopUpClosedCallback callback;
	if (isStealSuccessful)
	{
		text = "TODO steal magic item success";
		callback = [&uiManager]()
		{
			uiManager.disableTopMostContext();
			GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::MagesGuild);
		};
	}
	else
	{
		text = "TODO steal magic item failure";
		callback = [&game, &uiManager]()
		{
			uiManager.disableTopMostContext();
			GameWorldUI::closeConversation();

			GameState &gameState = game.gameState;
			gameState.queueCityGuardEncounter(game);
		};
	}

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcTavernBuyDrinksButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO buy drink (listbox)";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Tavern);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcTavernGetARoomButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO rent room (listbox)";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Tavern);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcTavernSneakIntoARoomButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO sneak into room";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Tavern);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcTavernRumorsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::TavernRumors);
}

void GameWorldUI::onNpcTavernRumorsGeneralButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO tavern rumors general";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Tavern);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcTavernRumorsWorkButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO tavern rumors work";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Tavern);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcTempleBlessButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO bless";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Temple);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcTempleCureButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO cure";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Temple);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcTempleHealButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "TODO heal";

	// @todo check if player is full health

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Temple);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::showShopkeeperBackground(const char *titleText)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	uiManager.clearContextElements(state.shopkeeperBgContextInstID, inputManager, renderer);

	const bool shouldUseSmallerFont = std::strlen(titleText) >= 32;
	std::string fontName = ArenaFontName::C;
	if (shouldUseSmallerFont)
	{
		fontName = ArenaFontName::Arena;
	}

	UiElementInitInfo titleTextBoxElementInitInfo;
	titleTextBoxElementInitInfo.name = "GameWorldShopkeeperBackgroundTitleTextBox";
	titleTextBoxElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, 10);
	titleTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	titleTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo titleTextBoxInitInfo;
	titleTextBoxInitInfo.text = titleText;
	titleTextBoxInitInfo.fontName = fontName;
	titleTextBoxInitInfo.defaultColor = Color(12, 12, 24);
	titleTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
	const UiElementInstanceID titleTextBoxElementInstID = uiManager.createTextBox(titleTextBoxElementInitInfo, titleTextBoxInitInfo, state.shopkeeperBgContextInstID, renderer);
	const Rect titleTextBoxGlobalRect = uiManager.getTransformGlobalRect(titleTextBoxElementInstID);

	const int titleImageTextureWidth = titleTextBoxGlobalRect.width + 24;
	constexpr int titleImageTextureHeight = 20;
	const UiTextureID titleImageTextureID = uiManager.getOrAddTexture(UiTexturePatternType::ShopkeeperTitle, titleImageTextureWidth, titleImageTextureHeight, textureManager, renderer);

	UiElementInitInfo titleImageElementInitInfo;
	titleImageElementInitInfo.name = "GameWorldShopkeeperBackgroundTitleImage";
	titleImageElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, 0);
	titleImageElementInitInfo.pivotType = UiPivotType::Top;
	titleImageElementInitInfo.drawOrder = 0;
	uiManager.createImage(titleImageElementInitInfo, titleImageTextureID, state.shopkeeperBgContextInstID, renderer);

	const TextureAsset barterBgTextureAsset(ArenaTextureName::BarterBackground);
	const TextureAsset barterBgPaletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();
	const UiTextureID barterBgImageTextureID = uiManager.getOrAddTexture(barterBgTextureAsset, barterBgPaletteTextureAsset, textureManager, renderer);

	UiElementInitInfo barterBgImageElementInitInfo;
	barterBgImageElementInitInfo.name = "GameWorldShopkeeperBackgroundBarterImage";
	barterBgImageElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, ArenaRenderUtils::SCREEN_HEIGHT);
	barterBgImageElementInitInfo.pivotType = UiPivotType::Bottom;
	uiManager.createImage(barterBgImageElementInitInfo, barterBgImageTextureID, state.shopkeeperBgContextInstID, renderer);

	uiManager.setContextEnabled(state.shopkeeperBgContextInstID, true);
}

bool GameWorldUI::isTriggerTextVisible()
{
	const GameWorldUiState &state = GameWorldUI::state;
	return state.triggerTextRemainingSeconds > 0.0;
}

bool GameWorldUI::isActionTextVisible()
{
	const GameWorldUiState &state = GameWorldUI::state;
	return state.actionTextRemainingSeconds > 0.0;
}

bool GameWorldUI::isEffectTextVisible()
{
	const GameWorldUiState &state = GameWorldUI::state;
	return state.effectTextRemainingSeconds > 0.0;
}

bool GameWorldUI::isCampingHoursTextVisible()
{
	const GameWorldUiState &state = GameWorldUI::state;
	const Game &game = *state.game;
	return game.gameState.isCamping();
}

void GameWorldUI::setTriggerText(const char *str)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID textBoxElementInstID = uiManager.getElementByName(TriggerTextBoxElementName);
	uiManager.setTextBoxText(textBoxElementInstID, str);

	GameWorldUI::setTriggerTextDuration(str);
}

void GameWorldUI::setActionText(const char *str)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID textBoxElementInstID = uiManager.getElementByName(ActionTextBoxElementName);
	uiManager.setTextBoxText(textBoxElementInstID, str);

	GameWorldUI::setActionTextDuration(str);
}

void GameWorldUI::setEffectText(const char *str)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID textBoxElementInstID = uiManager.getElementByName(EffectTextBoxElementName);
	uiManager.setTextBoxText(textBoxElementInstID, str);

	GameWorldUI::setEffectTextDuration(str);
}

void GameWorldUI::setCampingHoursText(const char *str)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID textBoxElementInstID = uiManager.getElementByName(CampingHoursTextBoxElementName);
	uiManager.setTextBoxText(textBoxElementInstID, str);
}

void GameWorldUI::setTriggerTextDuration(const std::string_view text)
{
	GameWorldUiState &state = GameWorldUI::state;
	state.triggerTextRemainingSeconds = GameWorldUiView::getTriggerTextSeconds(text);
}

void GameWorldUI::setActionTextDuration(const std::string_view text)
{
	GameWorldUiState &state = GameWorldUI::state;
	state.actionTextRemainingSeconds = GameWorldUiView::getActionTextSeconds(text);
}

void GameWorldUI::setEffectTextDuration(const std::string_view text)
{
	GameWorldUiState &state = GameWorldUI::state;
	state.effectTextRemainingSeconds = GameWorldUiView::getEffectTextSeconds(text);
}

void GameWorldUI::resetTriggerTextDuration()
{
	GameWorldUiState &state = GameWorldUI::state;
	state.triggerTextRemainingSeconds = 0.0;
}

void GameWorldUI::resetActionTextDuration()
{
	GameWorldUiState &state = GameWorldUI::state;
	state.actionTextRemainingSeconds = 0.0;
}

void GameWorldUI::resetEffectTextDuration()
{
	GameWorldUiState &state = GameWorldUI::state;
	state.effectTextRemainingSeconds = 0.0;
}

void GameWorldUI::onMouseButtonChanged(Game &game, MouseButtonType type, const Int2 &position, bool pressed)
{
	const GameWorldUiState &state = GameWorldUI::state;
	const Rect &centerCursorRegion = state.nativeCursorRegions[GameWorldUiView::CursorMiddleIndex];

	if (pressed)
	{
		GameState &gameState = game.gameState;
		if (gameState.isCamping())
		{
			gameState.clearCampingState();
		}

		const bool isLeftClick = type == MouseButtonType::Left;
		const bool isRightClick = type == MouseButtonType::Right;

		const Options &options = game.options;
		if (options.getGraphics_ModernInterface())
		{
			if (isRightClick)
			{
				Player &player = game.player;
				const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
				const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
				const WeaponAnimationDefinitionID weaponAnimDefID = player.getEquippedWeaponAnimationDefID();
				const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(weaponAnimDefID);
				DebugAssertIndex(weaponAnimDef.states, weaponAnimInst.currentStateIndex);
				const WeaponAnimationDefinitionState &weaponAnimDefState = weaponAnimDef.states[weaponAnimInst.currentStateIndex];

				const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
				ItemDefinitionID weaponItemDefID = player.getEquippedWeaponItemDefID();
				bool isRangedWeapon = false;
				if (weaponItemDefID >= 0)
				{
					const ItemDefinition &weaponItemDef = itemLibrary.getDefinition(weaponItemDefID);
					isRangedWeapon = weaponItemDef.weapon.isRanged;
				}

				if (WeaponAnimationUtils::isIdle(weaponAnimDefState) && !isRangedWeapon)
				{
					CardinalDirectionName randomMeleeSwingDirection = PlayerLogic::getRandomMeleeSwingDirection(game.random);
					player.queuedMeleeSwingDirection = static_cast<int>(randomMeleeSwingDirection);
				}
			}
		}
		else
		{
			if (centerCursorRegion.contains(position))
			{
				if (isLeftClick)
				{
					GameWorldUI::onScreenToWorldInteraction(position, true);
				}
				else if (isRightClick)
				{
					GameWorldUI::onScreenToWorldInteraction(position, false);
				}
			}
		}
	}
}

void GameWorldUI::onMouseButtonHeld(Game &game, MouseButtonType type, const Int2 &position, double dt)
{
	const GameWorldUiState &state = GameWorldUI::state;
	const Options &options = game.options;
	const Rect &centerCursorRegion = state.nativeCursorRegions[GameWorldUiView::CursorMiddleIndex];
	if (!options.getGraphics_ModernInterface() && !centerCursorRegion.contains(position))
	{
		if (type == MouseButtonType::Left)
		{
			// @todo: move out of PlayerLogicController::handlePlayerTurning() and handlePlayerAttack()
		}
	}
}

void GameWorldUI::onWindowResized(int width, int height)
{
	GameWorldUiState &state = GameWorldUI::state;
	state.updateNativeCursorRegions(width, height);

	Game &game = *state.game;
	Renderer &renderer = game.renderer;
	DebugAssert(state.playerHurtTextureID >= 0);
	renderer.freeUiTexture(state.playerHurtTextureID);

	UiManager &uiManager = game.uiManager;
	const Window &window = game.window;
	const UiElementInstanceID playerHurtImageElementInstID = uiManager.getElementByName(PlayerHurtImageElementName);
	uiManager.setTransformSize(playerHurtImageElementInstID, window.getSceneViewDimensions());

	state.playerHurtTextureID = GameWorldUiView::allocPlayerHurtTexture(window.getSceneViewAspectRatio(), window.fullGameWindow, renderer);
	uiManager.setImageTexture(playerHurtImageElementInstID, state.playerHurtTextureID);
}

void GameWorldUI::onCharacterSheetButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	game.setNextContext(CharacterUI::ContextName);
}

void GameWorldUI::onWeaponToggleButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	if (!game.canPlayerMoveAndTurn())
	{
		return;
	}

	Player &player = game.player;
	WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
	const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
	const WeaponAnimationDefinitionID weaponAnimDefID = player.getEquippedWeaponAnimationDefID();
	const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(weaponAnimDefID);
	const WeaponAnimationDefinitionState &weaponAnimDefState = weaponAnimDef.states[weaponAnimInst.currentStateIndex];

	int newStateIndex = -1;
	int nextStateIndex = -1;
	if (WeaponAnimationUtils::isSheathed(weaponAnimDefState))
	{
		weaponAnimDef.tryGetStateIndex(WeaponAnimationUtils::STATE_UNSHEATHING.c_str(), &newStateIndex);
		weaponAnimDef.tryGetStateIndex(WeaponAnimationUtils::STATE_IDLE.c_str(), &nextStateIndex);
	}
	else if (WeaponAnimationUtils::isIdle(weaponAnimDefState))
	{
		weaponAnimDef.tryGetStateIndex(WeaponAnimationUtils::STATE_SHEATHING.c_str(), &newStateIndex);
		weaponAnimDef.tryGetStateIndex(WeaponAnimationUtils::STATE_SHEATHED.c_str(), &nextStateIndex);
	}

	if (newStateIndex >= 0)
	{
		weaponAnimInst.setStateIndex(newStateIndex);
		weaponAnimInst.setNextStateIndex(nextStateIndex);
	}
}

void GameWorldUI::onMapButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	if (mouseButtonType == MouseButtonType::Left)
	{
		game.setNextContext(AutomapUI::ContextName);
	}
	else if (mouseButtonType == MouseButtonType::Right)
	{
		const MapType mapType = gameState.getActiveMapType();
		const EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
		const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

		const Player &player = game.player;
		const WorldDouble3 playerTravelPosition = player.getEyePosition();
		const bool isPlayerSafeToTravel = !entityChunkManager.anyEnemiesNearby(playerTravelPosition);
		const bool isPlayerInBoat = false; // @todo vehicle support
		const bool isPlayerAllowedToTravel = mapType != MapType::Interior;

		std::string text;
		if (!isPlayerSafeToTravel)
		{
			text = exeData.travel.notSafeToTravel;
		}
		else if (isPlayerInBoat)
		{
			text = exeData.travel.notAllowedToTravelInBoat;
		}
		else if (!isPlayerAllowedToTravel)
		{
			text = exeData.travel.notAllowedToTravel;
		}

		if (!text.empty())
		{
			GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, GameWorldUiView::StatusPopUpTextAlignment);
		}
		else
		{
			game.setNextContext(WorldMapUI::ContextName);
		}
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(mouseButtonType)));
	}
}

void GameWorldUI::onStealButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	GameWorldUI::setInteractionType(GameWorldInteractionType::Thieving);
}

void GameWorldUI::onStatusButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	const std::string text = GameWorldUiModel::getStatusButtonText(game);
	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, GameWorldUiView::StatusPopUpTextAlignment);
}

void GameWorldUI::onMagicButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	DebugLog("Magic.");
}

void GameWorldUI::onLogbookButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	game.setNextContext(LogbookUI::ContextName);
}

void GameWorldUI::onUseItemButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	DebugLog("Use item.");
}

void GameWorldUI::onCampButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		gameState.clearCampingState();
		return;
	}

	const MapType mapType = gameState.getActiveMapType();
	const MapSubDefinition &mapSubDef = gameState.getActiveMapDef().getSubDefinition();
	const EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

	const Player &player = game.player;
	const WorldDouble3 playerRestPosition = player.getEyePosition();
	const bool isPlayerSafeForResting = !entityChunkManager.anyEnemiesNearby(playerRestPosition);
	const bool isPlayerAllowedToRest = (mapType != MapType::City) && player.groundState.onGround && !player.groundState.isSwimming;
	const bool isPlayerAttemptingRestInTavern = (mapType == MapType::Interior) && (mapSubDef.interior.interiorType == ArenaInteriorType::Tavern);
	
	std::string text;
	if (!isPlayerSafeForResting)
	{
		text = exeData.camping.enemiesNearbyBeforeResting;
	}
	else if (!isPlayerAllowedToRest)
	{
		text = exeData.camping.campingNotAllowed;
	}
	else if (isPlayerAttemptingRestInTavern)
	{
		// @todo actually check if a bed has been rented
		text = exeData.camping.tavernBedNotRented;
	}

	if (!text.empty())
	{
		GameWorldUI::showTextPopUp(text.c_str(), ArenaFontName::A, GameWorldUiView::StatusPopUpTextAlignment);
	}
	else
	{
		GameWorldUI::showCampModal();
	}
}

void GameWorldUI::onScrollUpButtonSelected(MouseButtonType mouseButtonType)
{
	// Nothing yet.
}

void GameWorldUI::onScrollDownButtonSelected(MouseButtonType mouseButtonType)
{
	// Nothing yet.
}

void GameWorldUI::onActivateInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		const Options &options = game.options;
		if (options.getGraphics_ModernInterface())
		{
			const Int2 screenPoint = GameWorldUiView::getNativeWindowCenter(game.window);
			GameWorldUI::onScreenToWorldInteraction(screenPoint, true);
		}
	}
}

void GameWorldUI::onInspectInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		const Options &options = game.options;
		if (options.getGraphics_ModernInterface())
		{
			const Int2 screenPoint = GameWorldUiView::getNativeWindowCenter(game.window);
			GameWorldUI::onScreenToWorldInteraction(screenPoint, false);
		}
	}
}

void GameWorldUI::onCharacterSheetInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onCharacterSheetButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onToggleWeaponInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onWeaponToggleButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onAutomapInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onMapButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onWorldMapInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onMapButtonSelected(MouseButtonType::Right);
	}
}

void GameWorldUI::onStealInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onStealButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onStatusInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onStatusButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onCastMagicInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onMagicButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onLogbookInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onLogbookButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onUseItemInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onUseItemButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onCampInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onCampButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onToggleCompassInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		Options &options = game.options;
		const bool isCompassVisible = !options.getMisc_ShowCompass();
		options.setMisc_ShowCompass(isCompassVisible);
	}
}

void GameWorldUI::onPlayerPositionInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		const GameState &gameState = game.gameState;
		if (gameState.isCamping())
		{
			return;
		}

		const std::string text = GameWorldUiModel::getPlayerPositionText(game);
		GameWorldUI::setActionText(text.c_str());
	}
}

void GameWorldUI::onPauseMenuInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		GameState &gameState = game.gameState;
		if (gameState.isCamping())
		{
			gameState.clearCampingState();
			return;
		}

		game.setNextContext(PauseMenuUI::ContextName);
	}
}
