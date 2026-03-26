#include "AutomapUiState.h"
#include "CharacterUiState.h"
#include "GameWorldUiMVC.h"
#include "GameWorldUiState.h"
#include "LogbookUiState.h"
#include "PauseMenuUiState.h"
#include "WorldMapUiState.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Items/ItemLibrary.h"
#include "../Player/PlayerLogic.h"
#include "../Player/WeaponAnimationLibrary.h"
#include "../UI/FontLibrary.h"
#include "../UI/UiRenderSpace.h"

namespace
{
	constexpr char ContextName_TextPopUp[] = "GameWorldTextPopUp";
	constexpr char ContextName_LootPopUp[] = "GameWorldLootPopUp";

	constexpr char InterfaceImageElementName[] = "GameWorldInterfaceImage";
	constexpr char NoMagicImageElementName[] = "GameWorldNoMagicImage";
	constexpr char StatusGradientImageElementName[] = "GameWorldStatusGradientImage";
	constexpr char PlayerPortraitImageElementName[] = "GameWorldPlayerPortraitImage";
	constexpr char StatusBarsImageElementName[] = "GameWorldPlayerStatusBarsImage";

	constexpr char PlayerNameTextBoxElementName[] = "GameWorldPlayerNameTextBox";

	constexpr char CompassSliderImageElementName[] = "GameWorldCompassSlider";
	constexpr char CompassFrameImageElementName[] = "GameWorldCompassFrame";

	constexpr char WeaponImageElementName[] = "GameWorldWeaponImage";
	constexpr char ModernModeReticleImageElementName[] = "GameWorldModernModeReticleImage";

	constexpr char TriggerTextBoxElementName[] = "GameWorldTriggerTextBox";
	constexpr char ActionTextBoxElementName[] = "GameWorldActionTextBox";

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
		const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
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
	this->statusBarsTextureID = -1;
	this->statusGradientTextureID = -1;
	this->playerPortraitTextureID = -1;
	this->modernModeReticleTextureID = -1;
	this->currentHealth = 0.0;
	this->maxHealth = 0.0;
	this->currentStamina = 0.0;
	this->maxStamina = 0.0;
	this->currentSpellPoints = 0.0;
	this->maxSpellPoints = 0.0;
}

void GameWorldUiState::init(Game &game)
{
	const Player &player = game.player;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	this->game = &game;

	this->statusBarsTextureID = GameWorldUiView::allocStatusBarsTexture(textureManager, renderer);
	this->statusGradientTextureID = GameWorldUiView::allocStatusGradientTexture(GameWorldUiView::StatusGradientType::Default, textureManager, renderer);
	this->playerPortraitTextureID = GameWorldUiView::allocPlayerPortraitTexture(player.male, player.raceID, player.portraitID, textureManager, renderer);

	const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
	const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
	this->weaponAnimTextureIDs.init(weaponAnimDef.frameCount);
	for (int i = 0; i < weaponAnimDef.frameCount; i++)
	{
		const WeaponAnimationDefinitionFrame &weaponAnimDefFrame = weaponAnimDef.frames[i];
		const TextureAsset &weaponAnimDefFrameTextureAsset = weaponAnimDefFrame.textureAsset;
		const std::string &weaponAnimDefFrameTextureFilename = weaponAnimDefFrameTextureAsset.filename;
		DebugAssert(weaponAnimDefFrameTextureAsset.index.has_value());
		const int weaponAnimDefFrameTextureIndex = *weaponAnimDefFrameTextureAsset.index;
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

	this->modernModeReticleTextureID = GameWorldUiView::allocModernModeReticleTexture(textureManager, renderer);

	this->currentHealth = player.currentHealth;
	this->maxHealth = player.maxHealth;
	this->currentStamina = player.currentStamina;
	this->maxStamina = player.maxStamina;
	this->currentSpellPoints = player.currentSpellPoints;
	this->maxSpellPoints = player.maxSpellPoints;
}

void GameWorldUiState::freeTextures(Renderer &renderer)
{
	if (this->statusBarsTextureID >= 0)
	{
		renderer.freeUiTexture(this->statusBarsTextureID);
		this->statusBarsTextureID = -1;
	}

	if (this->statusGradientTextureID >= 0)
	{
		renderer.freeUiTexture(this->statusGradientTextureID);
		this->statusGradientTextureID = -1;
	}

	if (this->playerPortraitTextureID >= 0)
	{
		renderer.freeUiTexture(this->playerPortraitTextureID);
		this->playerPortraitTextureID = -1;
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

	if (this->modernModeReticleTextureID >= 0)
	{
		renderer.freeUiTexture(this->modernModeReticleTextureID);
		this->modernModeReticleTextureID = -1;
	}
}

void GameWorldUI::create(Game &game)
{
	GameWorldUiState &state = GameWorldUI::state;
	state.init(game);

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

	const bool isModernInterface = options.getGraphics_ModernInterface();

	UiElementInitInfo weaponAnimImageElementInitInfo;
	weaponAnimImageElementInitInfo.name = WeaponImageElementName;
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

	const UiElementInstanceID playerPortraitImageElementInstID = uiManager.getElementByName(PlayerPortraitImageElementName);
	uiManager.setImageTexture(playerPortraitImageElementInstID, state.playerPortraitTextureID);

	const UiElementInstanceID playerNameTextBoxElementInstID = uiManager.getElementByName(PlayerNameTextBoxElementName);
	const std::string playerNameText = GameWorldUiModel::getPlayerNameText(game);
	uiManager.setTextBoxText(playerNameTextBoxElementInstID, playerNameText.c_str());

	const UiElementInstanceID triggerTextBoxElementInstID = uiManager.getElementByName(TriggerTextBoxElementName);
	const Int2 triggerTextBoxPosition = GameWorldUiView::getTriggerTextPosition(game, ArenaRenderUtils::SCENE_UI_HEIGHT);
	uiManager.setTransformPosition(triggerTextBoxElementInstID, triggerTextBoxPosition);

	const UiElementInstanceID actionTextBoxElementInstID = uiManager.getElementByName(ActionTextBoxElementName);
	const Int2 actionTextBoxPosition = GameWorldUiView::getActionTextPosition();
	uiManager.setTransformPosition(actionTextBoxElementInstID, actionTextBoxPosition);

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

	uiManager.addMouseButtonChangedListener(GameWorldUI::onMouseButtonChanged, GameWorldUI::ContextName, inputManager);
	uiManager.addMouseButtonHeldListener(GameWorldUI::onMouseButtonHeld, GameWorldUI::ContextName, inputManager);

	game.shouldSimulateScene = true;
	game.shouldRenderScene = true;

	inputManager.setInputActionMapActive(InputActionMapName::GameWorld, true);

	if (!state.initInfo.textPopUpMessage.empty())
	{
		GameWorldUI::showTextPopUp(state.initInfo.textPopUpMessage.c_str());
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

	state.freeTextures(renderer);
	state.initInfo.textPopUpMessage.clear();
	state.currentHealth = 0.0;
	state.maxHealth = 0.0;
	state.currentStamina = 0.0;
	state.maxStamina = 0.0;
	state.currentSpellPoints = 0.0;
	state.maxSpellPoints = 0.0;
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

	// Weapon
	const UiElementInstanceID weaponImageElementInstID = uiManager.getElementByName(WeaponImageElementName);
	const bool isWeaponVisible = IsPlayerWeaponVisible(player);
	uiManager.setElementActive(weaponImageElementInstID, isWeaponVisible);
	if (isWeaponVisible)
	{
		const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
		const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
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
	const GameState &gameState = game.gameState;
	const bool isTriggerTextVisible = gameState.triggerTextIsVisible();
	const UiElementInstanceID triggerTextBoxElementInstID = uiManager.getElementByName(TriggerTextBoxElementName);
	uiManager.setElementActive(triggerTextBoxElementInstID, isTriggerTextVisible);

	const bool isActionTextVisible = gameState.actionTextIsVisible();
	const UiElementInstanceID actionTextBoxElementInstID = uiManager.getElementByName(ActionTextBoxElementName);
	uiManager.setElementActive(actionTextBoxElementInstID, isActionTextVisible);

	// @todo effect text

	if (!isModernInterface)
	{
		const InputManager &inputManager = game.inputManager;
		const Int2 cursorPosition = inputManager.getMousePosition();

		int arrowCursorRegionIndex = -1;
		for (int i = 0; i < GameWorldUiView::ArrowCursorRegionCount; i++)
		{
			const Rect &nativeCursorRegion = game.getNativeCursorRegion(i);
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
}

void GameWorldUI::onScreenToWorldInteraction(Int2 windowPoint, bool isPrimaryInteraction)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
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

void GameWorldUI::onPauseChanged(bool paused)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	const Player &player = game.player;
	UiManager &uiManager = game.uiManager;
	const Options &options = game.options;
	const bool isModernInterface = options.getGraphics_ModernInterface();

	game.shouldSimulateScene = !paused;

	const bool isWeaponVisible = !paused && IsPlayerWeaponVisible(player);
	const UiElementInstanceID weaponImageElementInstID = uiManager.getElementByName(WeaponImageElementName);
	uiManager.setElementActive(weaponImageElementInstID, isWeaponVisible);

	const bool isCompassVisible = !paused && options.getMisc_ShowCompass();
	const UiElementInstanceID compassSliderImageElementInstID = uiManager.getElementByName(CompassSliderImageElementName);
	const UiElementInstanceID compassFrameImageElementInstID = uiManager.getElementByName(CompassFrameImageElementName);
	uiManager.setElementActive(compassSliderImageElementInstID, isCompassVisible);
	uiManager.setElementActive(compassFrameImageElementInstID, isCompassVisible);

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

		// @todo effect text box

		game.setCursorOverride(std::nullopt);
	}

	if (isModernInterface)
	{
		uiManager.setElementActive(game.cursorImageElementInstID, paused);
		GameWorldUiModel::setFreeLookActive(game, !paused);
	}
}

void GameWorldUI::showTextPopUp(const char *str, const GameWorldPopUpClosedCallback &callback)
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
	textPopUpTextBoxInitInfo.fontName = GameWorldUiView::StatusPopUpFontName;
	textPopUpTextBoxInitInfo.defaultColor = GameWorldUiView::StatusPopUpTextColor;
	textPopUpTextBoxInitInfo.alignment = GameWorldUiView::StatusPopUpTextAlignment;
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
		GameWorldUI::onPauseChanged(false);
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

void GameWorldUI::showTextPopUp(const char *str)
{
	auto callback = []()
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		GameWorldUiController::onStatusPopUpSelected(game);
	};

	GameWorldUI::showTextPopUp(str, callback);
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
		std::string itemDisplayName = GetLootItemDisplayNameWithQty(itemDef, 1); // @todo implement stacking

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
				player.gold += 1; // @todo implement stacking
			}
			else
			{
				ItemInventory &playerInventory = player.inventory;
				playerInventory.insert(selectedItemDefID);
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
					newListBoxItemText = GetLootItemDisplayNameWithQty(curItemDef, 1); // @todo implement stacking
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

void GameWorldUI::setTriggerText(const char *str)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID textBoxElementInstID = uiManager.getElementByName(TriggerTextBoxElementName);
	uiManager.setTextBoxText(textBoxElementInstID, str);

	GameState &gameState = game.gameState;
	gameState.setTriggerTextDuration(str);
}

void GameWorldUI::setActionText(const char *str)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID textBoxElementInstID = uiManager.getElementByName(ActionTextBoxElementName);
	uiManager.setTextBoxText(textBoxElementInstID, str);

	GameState &gameState = game.gameState;
	gameState.setActionTextDuration(str);
}

void GameWorldUI::onMouseButtonChanged(Game &game, MouseButtonType type, const Int2 &position, bool pressed)
{
	const Rect &centerCursorRegion = game.getNativeCursorRegion(GameWorldUiView::CursorMiddleIndex);

	if (pressed)
	{
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
				const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
				DebugAssertIndex(weaponAnimDef.states, weaponAnimInst.currentStateIndex);
				const WeaponAnimationDefinitionState &weaponAnimDefState = weaponAnimDef.states[weaponAnimInst.currentStateIndex];

				if (WeaponAnimationUtils::isIdle(weaponAnimDefState) && !ArenaItemUtils::isRangedWeapon(player.weaponAnimDefID))
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
	const Options &options = game.options;
	const Rect &centerCursorRegion = game.getNativeCursorRegion(GameWorldUiView::CursorMiddleIndex);
	if (!options.getGraphics_ModernInterface() && !centerCursorRegion.contains(position))
	{
		if (type == MouseButtonType::Left)
		{
			// @todo: move out of PlayerLogicController::handlePlayerTurning() and handlePlayerAttack()
		}
	}
}

void GameWorldUI::onCharacterSheetButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	game.setNextContext(CharacterUI::ContextName);
}

void GameWorldUI::onWeaponToggleButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	Player &player = game.player;
	WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
	const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
	const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
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

	if (mouseButtonType == MouseButtonType::Left)
	{
		game.setNextContext(AutomapUI::ContextName);
	}
	else if (mouseButtonType == MouseButtonType::Right)
	{
		game.setNextContext(WorldMapUI::ContextName);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(mouseButtonType)));
	}
}

void GameWorldUI::onStealButtonSelected(MouseButtonType mouseButtonType)
{
	DebugLog("Steal.");
}

void GameWorldUI::onStatusButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	const std::string text = GameWorldUiModel::getStatusButtonText(game);
	GameWorldUI::showTextPopUp(text.c_str());
}

void GameWorldUI::onMagicButtonSelected(MouseButtonType mouseButtonType)
{
	DebugLog("Magic.");
}

void GameWorldUI::onLogbookButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	game.setNextContext(LogbookUI::ContextName);
}

void GameWorldUI::onUseItemButtonSelected(MouseButtonType mouseButtonType)
{
	DebugLog("Use item.");
}

void GameWorldUI::onCampButtonSelected(MouseButtonType mouseButtonType)
{
	DebugLog("Camp.");
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
	// @todo: uncomment this eventually when it actually works
	/*if (values.performed)
	{
		GameWorldUI::onCampButtonSelected(MouseButtonType::Left);
	}*/

	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	gameState.setIsCamping(values.performed);
}

void GameWorldUI::onToggleCompassInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Options &options = values.game.options;
		options.setMisc_ShowCompass(!options.getMisc_ShowCompass());
	}
}

void GameWorldUI::onPlayerPositionInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		const std::string text = GameWorldUiModel::getPlayerPositionText(game);
		GameWorldUI::setActionText(text.c_str());
	}
}

void GameWorldUI::onPauseMenuInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		game.setNextContext(PauseMenuUI::ContextName);
	}
}
