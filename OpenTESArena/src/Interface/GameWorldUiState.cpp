#include "AutomapPanel.h"
#include "CharacterPanel.h"
#include "GameWorldUiController.h"
#include "GameWorldUiModel.h"
#include "GameWorldUiState.h"
#include "GameWorldUiView.h"
#include "LogbookPanel.h"
#include "PauseMenuPanel.h"
#include "WorldMapPanel.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Player/PlayerLogic.h"
#include "../Player/WeaponAnimationLibrary.h"

namespace
{
	constexpr char ContextName_TextPopUp[] = "GameWorldTextPopUp";

	constexpr char InterfaceImageElementName[] = "GameWorldInterfaceImage";
	constexpr char NoMagicImageElementName[] = "GameWorldNoMagicImage";
	constexpr char StatusGradientImageElementName[] = "GameWorldStatusGradientImage";
	constexpr char PlayerPortraitImageElementName[] = "GameWorldPlayerPortraitImage";

	constexpr char PlayerNameTextBoxElementName[] = "GameWorldPlayerNameTextBox";

	constexpr char WeaponImageElementName[] = "GameWorldWeaponImage";

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

	std::string GetKeyImageElementName(int keyIndex)
	{
		char elementName[32];
		std::snprintf(elementName, sizeof(elementName), "GameWorldKey%dImage", keyIndex);
		return std::string(elementName);
	}

	constexpr MouseButtonTypeFlags PopUpMouseButtonTypeFlags = MouseButtonType::Left | MouseButtonType::Right;
}

GameWorldUiState::GameWorldUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->textPopUpContextInstID = -1;
	this->healthTextureID = -1;
	this->staminaTextureID = -1;
	this->spellPointsTextureID = -1;
	this->statusGradientTextureID = -1;
	this->playerPortraitTextureID = -1;
	this->modernModeReticleTextureID = -1;
}

void GameWorldUiState::init(Game &game)
{
	const Player &player = game.player;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	this->game = &game;

	this->healthTextureID = GameWorldUiView::allocHealthBarTexture(textureManager, renderer);
	this->staminaTextureID = GameWorldUiView::allocStaminaBarTexture(textureManager, renderer);
	this->spellPointsTextureID = GameWorldUiView::allocSpellPointsBarTexture(textureManager, renderer);
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
}

void GameWorldUiState::freeTextures(Renderer &renderer)
{
	if (this->healthTextureID >= 0)
	{
		renderer.freeUiTexture(this->healthTextureID);
		this->healthTextureID = -1;
	}

	if (this->staminaTextureID >= 0)
	{
		renderer.freeUiTexture(this->staminaTextureID);
		this->staminaTextureID = -1;
	}

	if (this->spellPointsTextureID >= 0)
	{
		renderer.freeUiTexture(this->spellPointsTextureID);
		this->spellPointsTextureID = -1;
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
	DebugLogError("@todo: put health/stam/sp into one texture. fix Panel::onPauseChanged() usage for here. travel places, enter interiors, etc.");

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
		UiElementInitInfo modernModeReticleImageElementInitInfo;
		modernModeReticleImageElementInitInfo.name = "GameWorldModernModeReticleImage";
		modernModeReticleImageElementInitInfo.position = GameWorldUiView::getInterfaceCenter(game);
		modernModeReticleImageElementInitInfo.pivotType = UiPivotType::Middle;
		modernModeReticleImageElementInitInfo.drawOrder = 1;
		uiManager.createImage(modernModeReticleImageElementInitInfo, state.modernModeReticleTextureID, state.contextInstID, renderer);

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

	uiManager.addMouseButtonChangedListener(GameWorldUI::onMouseButtonChanged, GameWorldUI::ContextName, inputManager);
	uiManager.addMouseButtonHeldListener(GameWorldUI::onMouseButtonHeld, GameWorldUI::ContextName, inputManager);

	game.shouldSimulateScene = true;
	game.shouldRenderScene = true;

	inputManager.setInputActionMapActive(InputActionMapName::GameWorld, true);
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

	state.freeTextures(renderer);

	inputManager.setInputActionMapActive(InputActionMapName::GameWorld, false);

	const Options &options = game.options;
	if (options.getGraphics_ModernInterface())
	{
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
	const Renderer &renderer = game.renderer;

	const bool isModernInterface = options.getGraphics_ModernInterface();
	const bool isGameWorldSimulating = game.shouldSimulateScene; // @todo all the setElementActive(false) needs to happen before a popup, right? previously it was Panel::paused

	// Compass
	const Player &player = game.player;
	const Double2 playerDirection = player.getGroundDirectionXZ();
	const Int2 compassSliderPosition = GameWorldUiView::getCompassSliderPosition(game, playerDirection);
	const UiElementInstanceID compassSliderImageElementInstID = uiManager.getElementByName("GameWorldCompassSlider");
	const UiElementInstanceID compassFrameImageElementInstID = uiManager.getElementByName("GameWorldCompassFrame");
	uiManager.setTransformPosition(compassSliderImageElementInstID, compassSliderPosition);

	const bool isCompassVisible = isGameWorldSimulating && options.getMisc_ShowCompass();
	uiManager.setElementActive(compassSliderImageElementInstID, isCompassVisible);
	uiManager.setElementActive(compassFrameImageElementInstID, isCompassVisible);

	// Keys
	const Span<const int> keyInventory = player.keyInventory;
	for (int i = 0; i < state.keyTextureIDs.getCount(); i++)
	{
		const std::string keyImageElementName = GetKeyImageElementName(i);
		const UiElementInstanceID keyImageElementInstID = uiManager.getElementByName(keyImageElementName.c_str());

		const int keyID = keyInventory[i];
		const bool isKeyVisible = isGameWorldSimulating && (keyID != ArenaItemUtils::InvalidDoorKeyID);
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

	// Weapon
	const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
	const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
	const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
	DebugAssertIndex(weaponAnimDef.states, weaponAnimInst.currentStateIndex);
	const WeaponAnimationDefinitionState &weaponAnimDefState = weaponAnimDef.states[weaponAnimInst.currentStateIndex];

	const UiElementInstanceID weaponImageElementInstID = uiManager.getElementByName(WeaponImageElementName);
	const bool isWeaponVisible = isGameWorldSimulating && !WeaponAnimationUtils::isSheathed(weaponAnimDefState);
	uiManager.setElementActive(weaponImageElementInstID, isWeaponVisible);
	if (isWeaponVisible)
	{
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

	// @todo health/stamina/spell points


	const GameState &gameState = game.gameState;
	const bool isTriggerTextVisible = isGameWorldSimulating && gameState.triggerTextIsVisible();
	const UiElementInstanceID triggerTextBoxElementInstID = uiManager.getElementByName(TriggerTextBoxElementName);
	uiManager.setElementActive(triggerTextBoxElementInstID, isTriggerTextVisible);

	const bool isActionTextVisible = isGameWorldSimulating && gameState.actionTextIsVisible();
	const UiElementInstanceID actionTextBoxElementInstID = uiManager.getElementByName(ActionTextBoxElementName);
	uiManager.setElementActive(actionTextBoxElementInstID, isActionTextVisible);

	// @todo modern mode reticle


	/*
	if (modernInterface)
	{
		auto getStatusBarsModernModeOrigin = [&window]()
		{
			const Int2 windowDims = window.getPixelDimensions();
			return Int2(GameWorldUiView::StatusBarModernModeXOffset, windowDims.y - GameWorldUiView::StatusBarModernModeYOffset);
		};

		auto getStatusBarScaledXDelta = [&window](const Rect &statusBarRect)
		{
			const int originalXDelta = statusBarRect.getLeft() - GameWorldUiView::HealthBarRect.getLeft();
			const double originalXPercentDelta = static_cast<double>(originalXDelta) / ArenaRenderUtils::SCREEN_WIDTH_REAL;

			const Int2 windowDims = window.getPixelDimensions();
			const double scaleXRatio = static_cast<double>(windowDims.x) / ArenaRenderUtils::SCREEN_WIDTH_REAL;
			const double aspectRatioMultiplier = ArenaRenderUtils::ASPECT_RATIO / window.getAspectRatio();
			return static_cast<int>(std::round(static_cast<double>(originalXDelta) * (scaleXRatio * aspectRatioMultiplier)));
		};

		constexpr UiPivotType statusBarPivotType = GameWorldUiView::StatusBarPivotType;
		const UiDrawCallActiveFunc statusBarActiveFunc = [this]() { return !this->isPaused(); };

		UiDrawCallInitInfo healthBarDrawCallInitInfo;
		healthBarDrawCallInitInfo.textureID = this->healthBarTextureRef.get();
		healthBarDrawCallInitInfo.positionFunc = [&window, getStatusBarsModernModeOrigin]()
		{
			const Int2 windowDims = window.getPixelDimensions();
			const Int2 nativePoint = getStatusBarsModernModeOrigin();
			return window.nativeToOriginal(nativePoint);
		};

		healthBarDrawCallInitInfo.sizeFunc = [&game]()
		{
			const Player &player = game.player;
			const Rect &barRect = GameWorldUiView::HealthBarRect;
			return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentHealth, player.maxHealth));
		};

		healthBarDrawCallInitInfo.pivotType = statusBarPivotType;
		healthBarDrawCallInitInfo.activeFunc = statusBarActiveFunc;
		this->addDrawCall(healthBarDrawCallInitInfo);

		UiDrawCallInitInfo staminaBarDrawCallInitInfo;
		staminaBarDrawCallInitInfo.textureID = this->staminaBarTextureRef.get();
		staminaBarDrawCallInitInfo.positionFunc = [&window, getStatusBarsModernModeOrigin, getStatusBarScaledXDelta]()
		{
			const int scaledXDelta = getStatusBarScaledXDelta(GameWorldUiView::StaminaBarRect);
			const Int2 nativePoint = getStatusBarsModernModeOrigin() + Int2(scaledXDelta, 0);
			return window.nativeToOriginal(nativePoint);
		};

		staminaBarDrawCallInitInfo.sizeFunc = [&game]()
		{
			const Player &player = game.player;
			const Rect &barRect = GameWorldUiView::StaminaBarRect;
			return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentStamina, player.maxStamina));
		};

		staminaBarDrawCallInitInfo.pivotType = statusBarPivotType;
		staminaBarDrawCallInitInfo.activeFunc = statusBarActiveFunc;
		this->addDrawCall(staminaBarDrawCallInitInfo);

		UiDrawCallInitInfo spellPointsBarDrawCallInitInfo;
		spellPointsBarDrawCallInitInfo.textureID = this->spellPointsBarTextureRef.get();
		spellPointsBarDrawCallInitInfo.positionFunc = [&window, getStatusBarsModernModeOrigin, getStatusBarScaledXDelta]()
		{
			const int scaledXDelta = getStatusBarScaledXDelta(GameWorldUiView::SpellPointsBarRect);
			const Int2 nativePoint = getStatusBarsModernModeOrigin() + Int2(scaledXDelta, 0);
			return window.nativeToOriginal(nativePoint);
		};

		spellPointsBarDrawCallInitInfo.sizeFunc = [&game]()
		{
			const Player &player = game.player;
			const Rect &barRect = GameWorldUiView::SpellPointsBarRect;
			return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentSpellPoints, player.maxSpellPoints));
		};

		spellPointsBarDrawCallInitInfo.pivotType = statusBarPivotType;
		spellPointsBarDrawCallInitInfo.activeFunc = statusBarActiveFunc;
		this->addDrawCall(spellPointsBarDrawCallInitInfo);
	}
	else
	{
		const Rect portraitRect = GameWorldUiView::getPlayerPortraitRect();

		UiDrawCallInitInfo statusGradientDrawCallInitInfo;
		statusGradientDrawCallInitInfo.textureID = this->statusGradientTextureRef.get();
		statusGradientDrawCallInitInfo.position = portraitRect.getTopLeft();
		statusGradientDrawCallInitInfo.size = this->statusGradientTextureRef.getDimensions();
		this->addDrawCall(statusGradientDrawCallInitInfo);

		UiDrawCallInitInfo playerPortraitDrawCallInitInfo;
		playerPortraitDrawCallInitInfo.textureID = this->playerPortraitTextureRef.get();
		playerPortraitDrawCallInitInfo.position = portraitRect.getTopLeft();
		playerPortraitDrawCallInitInfo.size = this->playerPortraitTextureRef.getDimensions();
		this->addDrawCall(playerPortraitDrawCallInitInfo);

		constexpr UiPivotType statusBarPivotType = GameWorldUiView::StatusBarPivotType;

		UiDrawCallInitInfo healthBarDrawCallInitInfo;
		healthBarDrawCallInitInfo.textureID = this->healthBarTextureRef.get();
		healthBarDrawCallInitInfo.position = GameWorldUiView::HealthBarRect.getBottomLeft();
		healthBarDrawCallInitInfo.sizeFunc = [&game]()
		{
			const Player &player = game.player;
			const Rect barRect = GameWorldUiView::HealthBarRect;
			return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentHealth, player.maxHealth));
		};

		healthBarDrawCallInitInfo.pivotType = statusBarPivotType;
		this->addDrawCall(healthBarDrawCallInitInfo);

		UiDrawCallInitInfo staminaBarDrawCallInitInfo;
		staminaBarDrawCallInitInfo.textureID = this->staminaBarTextureRef.get();
		staminaBarDrawCallInitInfo.position = GameWorldUiView::StaminaBarRect.getBottomLeft();
		staminaBarDrawCallInitInfo.sizeFunc = [&game]()
		{
			const Player &player = game.player;
			const Rect barRect = GameWorldUiView::StaminaBarRect;
			return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentStamina, player.maxStamina));
		};

		staminaBarDrawCallInitInfo.pivotType = statusBarPivotType;
		this->addDrawCall(staminaBarDrawCallInitInfo);

		UiDrawCallInitInfo spellPointsBarDrawCallInitInfo;
		spellPointsBarDrawCallInitInfo.textureID = this->spellPointsBarTextureRef.get();
		spellPointsBarDrawCallInitInfo.position = GameWorldUiView::SpellPointsBarRect.getBottomLeft();
		spellPointsBarDrawCallInitInfo.sizeFunc = [&game]()
		{
			const Player &player = game.player;
			const Rect barRect = GameWorldUiView::SpellPointsBarRect;
			return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentSpellPoints, player.maxSpellPoints));
		};

		spellPointsBarDrawCallInitInfo.pivotType = statusBarPivotType;
		this->addDrawCall(spellPointsBarDrawCallInitInfo);

		const FontLibrary &fontLibrary = FontLibrary::getInstance();
		this->tooltipTextureRefs.init(GameWorldUiModel::BUTTON_COUNT);
		for (int i = 0; i < GameWorldUiModel::BUTTON_COUNT; i++)
		{
			const GameWorldUiModel::ButtonType buttonType = static_cast<GameWorldUiModel::ButtonType>(i);
			const UiTextureID tooltipTextureID = GameWorldUiView::allocTooltipTexture(buttonType, fontLibrary, renderer);
			this->tooltipTextureRefs.set(i, ScopedUiTextureRef(tooltipTextureID, renderer));
		}

		UiDrawCallInitInfo tooltipDrawCallInitInfo;
		tooltipDrawCallInitInfo.textureFunc = [this, &game]()
		{
			std::optional<GameWorldUiModel::ButtonType> buttonType = GameWorldUiModel::getHoveredButtonType(game);
			if (!buttonType.has_value())
			{
				DebugCrash("Expected tooltip texture func to only be called when actually drawing a tooltip.");
			}

			const int index = static_cast<int>(*buttonType);
			const ScopedUiTextureRef &tooltipTextureRef = this->tooltipTextureRefs.get(index);
			return tooltipTextureRef.get();
		};

		tooltipDrawCallInitInfo.position = GameWorldUiView::getTooltipPosition(game);
		tooltipDrawCallInitInfo.sizeFunc = [this, &game]()
		{
			std::optional<GameWorldUiModel::ButtonType> buttonType = GameWorldUiModel::getHoveredButtonType(game);
			if (!buttonType.has_value())
			{
				DebugCrash("Expected tooltip size func to only be called when actually drawing a tooltip.");
			}

			const int index = static_cast<int>(*buttonType);
			const ScopedUiTextureRef &tooltipTextureRef = this->tooltipTextureRefs.get(index);
			return tooltipTextureRef.getDimensions();
		};

		tooltipDrawCallInitInfo.pivotType = UiPivotType::BottomLeft;
		tooltipDrawCallInitInfo.activeFunc = [this, &game]()
		{
			if (this->isPaused())
			{
				return false;
			}

			const std::optional<GameWorldUiModel::ButtonType> buttonType = GameWorldUiModel::getHoveredButtonType(game);
			return buttonType.has_value() && GameWorldUiModel::isButtonTooltipAllowed(*buttonType, game);
		};

		this->addDrawCall(tooltipDrawCallInitInfo);

		UiDrawCallInitInfo cursorDrawCallInitInfo;
		cursorDrawCallInitInfo.textureFunc = [this, &game, getCursorRegionIndex]()
		{
			const std::optional<int> index = getCursorRegionIndex();
			if (!index.has_value())
			{
				return this->defaultCursorTextureRef.get();
			}

			const ScopedUiTextureRef &arrowCursorTextureRef = this->arrowCursorTextureRefs.get(*index);
			return arrowCursorTextureRef.get();
		};

		cursorDrawCallInitInfo.positionFunc = cursorPositionFunc;
		cursorDrawCallInitInfo.sizeFunc = [this, &game, getCursorRegionIndex]()
		{
			const std::optional<int> index = getCursorRegionIndex();
			const Int2 textureDims = [this, &index]()
			{
				if (!index.has_value())
				{
					return this->defaultCursorTextureRef.getDimensions();
				}

				const ScopedUiTextureRef &arrowCursorTextureRef = this->arrowCursorTextureRefs.get(*index);
				return arrowCursorTextureRef.getDimensions();
			}();

			const auto &options = game.options;
			const double cursorScale = options.getGraphics_CursorScale();
			return Int2(
				static_cast<int>(static_cast<double>(textureDims.x) * cursorScale),
				static_cast<int>(static_cast<double>(textureDims.y) * cursorScale));
		};

		cursorDrawCallInitInfo.pivotFunc = [this, getCursorRegionIndex]()
		{
			const std::optional<int> index = getCursorRegionIndex();
			if (!index.has_value())
			{
				return UiPivotType::TopLeft;
			}

			Span<const UiPivotType> arrowCursorPivotTypes = GameWorldUiView::ArrowCursorPivotTypes;
			return arrowCursorPivotTypes[*index];
		};

		cursorDrawCallInitInfo.activeFunc = [this]() { return !this->isPaused(); };
		cursorDrawCallInitInfo.renderSpace = UiRenderSpace::Native;
		this->addDrawCall(cursorDrawCallInitInfo);
	}
	*/

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

		UiTextureID cursorTextureID = game.defaultCursorTextureID;
		Int2 cursorSize = *renderer.tryGetUiTextureDims(game.defaultCursorTextureID);
		UiPivotType cursorPivotType = UiPivotType::TopLeft;
		if (arrowCursorRegionIndex >= 0)
		{
			const UiTextureID arrowCursorTextureID = state.arrowCursorTextureIDs[arrowCursorRegionIndex];
			cursorTextureID = arrowCursorTextureID;
			cursorSize = *renderer.tryGetUiTextureDims(arrowCursorTextureID);

			Span<const UiPivotType> arrowCursorPivotTypes = GameWorldUiView::ArrowCursorPivotTypes;
			cursorPivotType = arrowCursorPivotTypes[arrowCursorRegionIndex];
		}

		const double cursorScale = options.getGraphics_CursorScale();
		const Int2 cursorSizeScaled(
			static_cast<int>(static_cast<double>(cursorSize.x) * cursorScale),
			static_cast<int>(static_cast<double>(cursorSize.y) * cursorScale));

		const UiElementInstanceID cursorImageElementInstID = game.cursorImageElementInstID;
		uiManager.setTransformPosition(cursorImageElementInstID, cursorPosition);
		uiManager.setTransformSize(cursorImageElementInstID, cursorSizeScaled);
		uiManager.setTransformPivot(cursorImageElementInstID, cursorPivotType);
		uiManager.setImageTexture(cursorImageElementInstID, cursorTextureID);
	}
}

void GameWorldUI::onScreenToWorldInteraction(Int2 windowPoint, bool isPrimaryInteraction)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;

	if (isPrimaryInteraction)
	{
		const InputManager &inputManager = game.inputManager;
		const bool debugFadeVoxel = inputManager.keyIsDown(SDL_SCANCODE_G);
		PlayerLogic::handleScreenToWorldInteraction(game, windowPoint, isPrimaryInteraction, debugFadeVoxel);
	}
	else
	{
		constexpr bool debugFadeVoxel = false;
		PlayerLogic::handleScreenToWorldInteraction(game, windowPoint, isPrimaryInteraction, debugFadeVoxel);
	}
}

void GameWorldUI::showTextPopUp(const char *str, const std::function<void()> &callback)
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

	auto buttonCallback = [callback](MouseButtonType)
	{
		callback();

		DebugLogError("@todo: set everything in GameWorldUI paused = false?");
		//game.shouldSimulateScene = true; ??
	};

	UiButtonInitInfo textPopUpBackButtonInitInfo;
	textPopUpBackButtonInitInfo.mouseButtonFlags = PopUpMouseButtonTypeFlags;
	textPopUpBackButtonInitInfo.callback = buttonCallback;
	uiManager.createButton(textPopUpBackButtonElementInitInfo, textPopUpBackButtonInitInfo, state.textPopUpContextInstID);

	auto inputActionCallback = [buttonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			buttonCallback(MouseButtonType::Left);
		}
	};

	uiManager.addInputActionListener(InputActionName::Back, inputActionCallback, ContextName_TextPopUp, inputManager);

	DebugLogError("@todo: set everything in GameWorldUI paused = true?");
	//game.shouldSimulateScene = false; ??

	uiManager.setContextEnabled(state.textPopUpContextInstID, true);
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
	game.setPanel<CharacterPanel>();
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
		game.setPanel<AutomapPanel>();
	}
	else if (mouseButtonType == MouseButtonType::Right)
	{
		game.setPanel<WorldMapPanel>();
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
	game.setPanel<LogbookPanel>();
}

void GameWorldUI::onUseItemButtonSelected(MouseButtonType mouseButtonType)
{
	DebugLog("Use item.");
}

void GameWorldUI::onCampButtonSelected(MouseButtonType mouseButtonType)
{
	DebugLog("Camp.");

	// @todo: make this click the button eventually when not needed for testing.
	//gameState.setIsCamping(values.performed);
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
		game.setPanel<PauseMenuPanel>();
	}
}
