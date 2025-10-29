#include <algorithm>
#include <cmath>

#include "CommonUiView.h"
#include "GameWorldPanel.h"
#include "GameWorldUiController.h"
#include "GameWorldUiModel.h"
#include "GameWorldUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Items/ArenaItemUtils.h"
#include "../Player/WeaponAnimationLibrary.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../UI/FontLibrary.h"

#include "components/debug/Debug.h"

GameWorldPanel::GameWorldPanel(Game &game)
	: Panel(game) { }

GameWorldPanel::~GameWorldPanel()
{
	auto &game = this->getGame();
	auto &inputManager = game.inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::GameWorld, false);

	// If in modern mode, disable free-look.
	const auto &options = game.options;
	const bool modernInterface = options.getGraphics_ModernInterface();

	if (modernInterface)
	{
		GameWorldUiModel::setFreeLookActive(game, false);
	}

	game.shouldSimulateScene = false;
	game.shouldRenderScene = false;
}

bool GameWorldPanel::init()
{
	auto &game = this->getGame();

	Renderer &renderer = game.renderer;
	const auto &fontLibrary = FontLibrary::getInstance();
	const std::string playerNameText = GameWorldUiModel::getPlayerNameText(game);
	const TextBoxInitInfo playerNameTextBoxInitInfo = GameWorldUiView::getPlayerNameTextBoxInitInfo(playerNameText, fontLibrary);
	if (!this->playerNameTextBox.init(playerNameTextBoxInitInfo, playerNameText, renderer))
	{
		DebugLogError("Couldn't init player name text box.");
		return false;
	}

	const TextBoxInitInfo triggerTextBoxInitInfo = GameWorldUiView::getTriggerTextBoxInitInfo(fontLibrary);
	if (!this->triggerText.init(triggerTextBoxInitInfo, renderer))
	{
		DebugLogError("Couldn't init trigger text box.");
		return false;
	}

	const TextBoxInitInfo actionTextBoxInitInfo = GameWorldUiView::getActionTextBoxInitInfo(fontLibrary);
	if (!this->actionText.init(actionTextBoxInitInfo, renderer))
	{
		DebugLogError("Couldn't init action text box.");
		return false;
	}

	// @todo: effect text box initialization

	this->characterSheetButton = Button<Game&>(GameWorldUiView::getCharacterSheetButtonRect(), GameWorldUiController::onCharacterSheetButtonSelected);
	this->drawWeaponButton = Button<Player&>(GameWorldUiView::getWeaponSheathButtonRect(), GameWorldUiController::onWeaponButtonSelected);
	this->stealButton = Button<>(GameWorldUiView::getStealButtonRect(), GameWorldUiController::onStealButtonSelected);
	this->statusButton = Button<Game&>(GameWorldUiView::getStatusButtonRect(), GameWorldUiController::onStatusButtonSelected);
	this->magicButton = Button<>(GameWorldUiView::getMagicButtonRect(), GameWorldUiController::onMagicButtonSelected);
	this->logbookButton = Button<Game&>(GameWorldUiView::getLogbookButtonRect(), GameWorldUiController::onLogbookButtonSelected);
	this->useItemButton = Button<>(GameWorldUiView::getUseItemButtonRect(), GameWorldUiController::onUseItemButtonSelected);
	this->campButton = Button<>(GameWorldUiView::getCampButtonRect(), GameWorldUiController::onCampButtonSelected);
	this->scrollUpButton = Button<GameWorldPanel&>(GameWorldUiView::getScrollUpButtonRect(), GameWorldUiController::onScrollUpButtonSelected);
	this->scrollDownButton = Button<GameWorldPanel&>(GameWorldUiView::getScrollDownButtonRect(), GameWorldUiController::onScrollDownButtonSelected);
	this->mapButton = Button<Game&, bool>(GameWorldUiView::getMapButtonRect(), GameWorldUiController::onMapButtonSelected);

	auto &player = game.player;
	const auto &options = game.options;
	const bool modernInterface = options.getGraphics_ModernInterface();
	if (!modernInterface)
	{
		// Classic interface buttons.
		this->addButtonProxy(MouseButtonType::Left, this->characterSheetButton.getRect(),
			[this, &game]() { this->characterSheetButton.click(game); });
		this->addButtonProxy(MouseButtonType::Left, this->drawWeaponButton.getRect(),
			[this, &player]() { this->drawWeaponButton.click(player); });
		this->addButtonProxy(MouseButtonType::Left, this->stealButton.getRect(),
			[this]() { this->stealButton.click(); });
		this->addButtonProxy(MouseButtonType::Left, this->statusButton.getRect(),
			[this, &game]() { this->statusButton.click(game); });
		this->addButtonProxy(MouseButtonType::Left, this->magicButton.getRect(),
			[this]() { this->magicButton.click(); });
		this->addButtonProxy(MouseButtonType::Left, this->logbookButton.getRect(),
			[this, &game]() { this->logbookButton.click(game); });
		this->addButtonProxy(MouseButtonType::Left, this->useItemButton.getRect(),
			[this]() { this->useItemButton.click(); });
		this->addButtonProxy(MouseButtonType::Left, this->campButton.getRect(),
			[this]() { this->campButton.click(); });
		this->addButtonProxy(MouseButtonType::Left, this->scrollUpButton.getRect(),
			[this]() { this->scrollUpButton.click(*this); });
		this->addButtonProxy(MouseButtonType::Left, this->scrollDownButton.getRect(),
			[this]() { this->scrollDownButton.click(*this); });
		this->addButtonProxy(MouseButtonType::Left, this->mapButton.getRect(),
			[this, &game]() { this->mapButton.click(game, true); });
		this->addButtonProxy(MouseButtonType::Right, this->mapButton.getRect(),
			[this, &game]() { this->mapButton.click(game, false); });
	}

	auto &inputManager = game.inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::GameWorld, true);

	this->addInputActionListener(InputActionName::Activate,
		[this](const InputActionCallbackValues &values)
	{
		GameWorldUiController::onActivateInputAction(values, this->actionText);
	});

	this->addInputActionListener(InputActionName::Inspect,
		[this](const InputActionCallbackValues &values)
	{
		GameWorldUiController::onInspectInputAction(values, this->actionText);
	});

	this->addInputActionListener(InputActionName::CharacterSheet,
		[this, &game](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->characterSheetButton.click(game);
		}
	});

	this->addInputActionListener(InputActionName::ToggleWeapon,
		[this, &player](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->drawWeaponButton.click(player);
		}
	});

	this->addInputActionListener(InputActionName::Steal,
		[this](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->stealButton.click();
		}
	});

	this->addInputActionListener(InputActionName::Status,
		[this, &game](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->statusButton.click(game);
		}
	});

	this->addInputActionListener(InputActionName::CastMagic,
		[this](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->magicButton.click();
		}
	});

	this->addInputActionListener(InputActionName::Logbook,
		[this, &game](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->logbookButton.click(game);
		}
	});

	this->addInputActionListener(InputActionName::UseItem,
		[this](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->useItemButton.click();
		}
	});

	this->addInputActionListener(InputActionName::Camp,
		[this](const InputActionCallbackValues &values)
	{
		Game &game = values.game;
		GameState &gameState = game.gameState;

		// @todo: make this click the button eventually when not needed for testing.
		gameState.setIsCamping(values.performed);
	});

	this->addInputActionListener(InputActionName::Automap,
		[this, &game](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->mapButton.click(game, true);
		}
	});

	this->addInputActionListener(InputActionName::WorldMap,
		[this, &game](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->mapButton.click(game, false);
		}
	});

	this->addInputActionListener(InputActionName::ToggleCompass, GameWorldUiController::onToggleCompassInputAction);
	this->addInputActionListener(InputActionName::PlayerPosition,
		[this](const InputActionCallbackValues &values)
	{
		GameWorldUiController::onPlayerPositionInputAction(values, this->actionText);
	});

	this->addInputActionListener(InputActionName::PauseMenu, GameWorldUiController::onPauseInputAction);

	this->addMouseButtonChangedListener([this](Game &game, MouseButtonType type, const Int2 &position, bool pressed)
	{
		const Rect &centerCursorRegion = game.getNativeCursorRegion(GameWorldUiView::CursorMiddleIndex);
		GameWorldUiController::onMouseButtonChanged(game, type, position, pressed, centerCursorRegion, this->actionText);
	});

	this->addMouseButtonHeldListener([this](Game &game, MouseButtonType type, const Int2 &position, double dt)
	{
		const Rect &centerCursorRegion = game.getNativeCursorRegion(GameWorldUiView::CursorMiddleIndex);
		GameWorldUiController::onMouseButtonHeld(game, type, position, dt, centerCursorRegion);
	});

	// Moved into a method for better organization due to extra complexity from classic/modern mode.
	this->initUiDrawCalls();

	// If in modern mode, lock mouse to center of screen for free-look.
	if (modernInterface)
	{
		GameWorldUiModel::setFreeLookActive(game, true);
	}

	game.shouldSimulateScene = true;
	game.shouldRenderScene = true;

	return true;
}

void GameWorldPanel::initUiDrawCalls()
{
	Game &game = this->getGame();
	TextureManager &textureManager = game.textureManager;
	const Window &window = game.window;
	Renderer &renderer = game.renderer;

	const Options &options = game.options;
	const bool modernInterface = options.getGraphics_ModernInterface();

	const UiTextureID gameWorldInterfaceTextureID = GameWorldUiView::allocGameWorldInterfaceTexture(textureManager, renderer);
	this->gameWorldInterfaceTextureRef.init(gameWorldInterfaceTextureID, renderer);

	const UiTextureID healthTextureID = GameWorldUiView::allocHealthBarTexture(textureManager, renderer);
	this->healthBarTextureRef.init(healthTextureID, renderer);
	const UiTextureID staminaTextureID = GameWorldUiView::allocStaminaBarTexture(textureManager, renderer);
	this->staminaBarTextureRef.init(staminaTextureID, renderer);	
	const UiTextureID spellPointsTextureID = GameWorldUiView::allocSpellPointsBarTexture(textureManager, renderer);
	this->spellPointsBarTextureRef.init(spellPointsTextureID, renderer);

	constexpr GameWorldUiView::StatusGradientType gradientType = GameWorldUiView::StatusGradientType::Default;
	const UiTextureID statusGradientTextureID = GameWorldUiView::allocStatusGradientTexture(gradientType, textureManager, renderer);
	this->statusGradientTextureRef.init(statusGradientTextureID, renderer);

	const Player &player = game.player;
	const UiTextureID playerPortraitTextureID = GameWorldUiView::allocPlayerPortraitTexture(player.male, player.raceID, player.portraitID, textureManager, renderer);
	this->playerPortraitTextureRef.init(playerPortraitTextureID, renderer);

	const UiTextureID noMagicTextureID = GameWorldUiView::allocNoMagicTexture(textureManager, renderer);
	this->noMagicTextureRef.init(noMagicTextureID, renderer);

	const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
	const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
	this->weaponAnimTextureRefs.init(weaponAnimDef.frameCount);
	for (int i = 0; i < weaponAnimDef.frameCount; i++)
	{
		const WeaponAnimationDefinitionFrame &weaponAnimDefFrame = weaponAnimDef.frames[i];
		const TextureAsset &weaponAnimDefFrameTextureAsset = weaponAnimDefFrame.textureAsset;
		const std::string &weaponAnimDefFrameTextureFilename = weaponAnimDefFrameTextureAsset.filename;
		DebugAssert(weaponAnimDefFrameTextureAsset.index.has_value());
		const int weaponAnimDefFrameTextureIndex = *weaponAnimDefFrameTextureAsset.index;
		// @todo: some WeaponAnimationDefinitionFrames are duplicates, this can cause duplicate UiTextureID allocations, maybe map TextureAsset to UiTextureID to avoid it
		const UiTextureID weaponTextureID = GameWorldUiView::allocWeaponAnimTexture(weaponAnimDefFrameTextureFilename, weaponAnimDefFrameTextureIndex, textureManager, renderer);
		this->weaponAnimTextureRefs.set(i, ScopedUiTextureRef(weaponTextureID, renderer));
	}

	const int keyTextureCount = GameWorldUiView::getKeyTextureCount(textureManager);
	this->keyTextureRefs.init(keyTextureCount);
	for (int i = 0; i < keyTextureCount; i++)
	{
		const UiTextureID keyTextureID = GameWorldUiView::allocKeyTexture(i, textureManager, renderer);
		this->keyTextureRefs.set(i, ScopedUiTextureRef(keyTextureID, renderer));
	}	

	const UiTextureID compassFrameTextureID = GameWorldUiView::allocCompassFrameTexture(textureManager, renderer);
	const UiTextureID compassSliderTextureID = GameWorldUiView::allocCompassSliderTexture(textureManager, renderer);
	this->compassFrameTextureRef.init(compassFrameTextureID, renderer);
	this->compassSliderTextureRef.init(compassSliderTextureID, renderer);

	const UiTextureID defaultCursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->defaultCursorTextureRef.init(defaultCursorTextureID, renderer);

	const UiTextureID modernModeCursorTextureID = GameWorldUiView::allocModernModeReticleTexture(textureManager, renderer);
	this->modernModeReticleTextureRef.init(modernModeCursorTextureID, renderer);

	this->arrowCursorTextureRefs.init(GameWorldUiView::ArrowCursorRegionCount);
	for (int i = 0; i < GameWorldUiView::ArrowCursorRegionCount; i++)
	{
		const UiTextureID arrowTextureID = GameWorldUiView::allocArrowCursorTexture(i, textureManager, renderer);
		this->arrowCursorTextureRefs.set(i, ScopedUiTextureRef(arrowTextureID, renderer));
	}

	for (int i = 0; i < this->keyTextureRefs.getCount(); i++)
	{
		auto getKeyIdAtIndex = [&game](int keyIndex)
		{
			const Player &player = game.player;
			const auto &keyInventory = player.keyInventory;
			DebugAssertIndex(keyInventory, keyIndex);
			const int keyID = keyInventory[keyIndex];
			return keyID;
		};

		UiDrawCallInitInfo keyDrawCallInitInfo;
		keyDrawCallInitInfo.textureFunc = [this, &game, i, getKeyIdAtIndex]()
		{
			const int keyID = getKeyIdAtIndex(i);
			DebugAssert(keyID != ArenaItemUtils::InvalidDoorKeyID);
			const ScopedUiTextureRef &textureRef = this->keyTextureRefs.get(keyID);
			return textureRef.get();
		};

		keyDrawCallInitInfo.position = GameWorldUiView::getKeyPosition(i);
		keyDrawCallInitInfo.sizeFunc = [this, i, getKeyIdAtIndex]()
		{
			const int keyID = getKeyIdAtIndex(i);
			const ScopedUiTextureRef &textureRef = this->keyTextureRefs.get(keyID);
			return textureRef.getDimensions();
		};

		keyDrawCallInitInfo.activeFunc = [this, &game, i, getKeyIdAtIndex]()
		{
			const int keyID = getKeyIdAtIndex(i);
			return !this->isPaused() && (keyID != ArenaItemUtils::InvalidDoorKeyID);
		};

		this->addDrawCall(keyDrawCallInitInfo);
	}

	if (modernInterface)
	{
		UiDrawCallInitInfo weaponDrawCallInitInfo;
		weaponDrawCallInitInfo.textureFunc = [this, &player]()
		{
			const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
			const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
			const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
			const int weaponAnimFrameIndex = WeaponAnimationUtils::getFrameIndex(weaponAnimInst, weaponAnimDef);
			const ScopedUiTextureRef &textureRef = this->weaponAnimTextureRefs.get(weaponAnimFrameIndex);
			return textureRef.get();
		};

		weaponDrawCallInitInfo.positionFunc = [this, &game, &player]()
		{
			const int classicViewHeight = ArenaRenderUtils::SCREEN_HEIGHT - this->gameWorldInterfaceTextureRef.getHeight();

			const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
			const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
			const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
			const int weaponAnimFrameIndex = WeaponAnimationUtils::getFrameIndex(weaponAnimInst, weaponAnimDef);
			const WeaponAnimationDefinitionFrame &weaponAnimFrame = weaponAnimDef.frames[weaponAnimFrameIndex];

			auto &textureManager = game.textureManager;
			const Double2 offsetPercents(
				static_cast<double>(weaponAnimFrame.xOffset) / ArenaRenderUtils::SCREEN_WIDTH_REAL,
				static_cast<double>(weaponAnimFrame.yOffset) / static_cast<double>(classicViewHeight));

			const Window &window = game.window;
			const Int2 windowDims = window.getPixelDimensions();
			const Int2 nativePosition(
				static_cast<int>(std::round(offsetPercents.x * static_cast<double>(windowDims.x))),
				static_cast<int>(std::round(offsetPercents.y * static_cast<double>(windowDims.y))));
			return nativePosition;
		};

		weaponDrawCallInitInfo.sizeFunc = [this, &game, &player]()
		{
			const int classicViewHeight = ArenaRenderUtils::SCREEN_HEIGHT - this->gameWorldInterfaceTextureRef.getHeight();

			const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
			const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
			const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
			const int weaponAnimFrameIndex = WeaponAnimationUtils::getFrameIndex(weaponAnimInst, weaponAnimDef);
			const ScopedUiTextureRef &textureRef = this->weaponAnimTextureRefs.get(weaponAnimFrameIndex);
			const Int2 textureDims = textureRef.getDimensions();
			const Double2 texturePercents(
				static_cast<double>(textureDims.x) / ArenaRenderUtils::SCREEN_WIDTH_REAL,
				static_cast<double>(textureDims.y) / static_cast<double>(classicViewHeight));

			const Window &window = game.window;
			const Int2 windowDims = window.getPixelDimensions();
			const Int2 nativeTextureDims(
				static_cast<int>(std::round(texturePercents.x * static_cast<double>(windowDims.x))),
				static_cast<int>(std::round(texturePercents.y * static_cast<double>(windowDims.y))));
			return nativeTextureDims;
		};

		weaponDrawCallInitInfo.activeFunc = [this, &player]()
		{
			const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
			const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
			const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
			const WeaponAnimationDefinitionState &weaponAnimDefState = weaponAnimDef.states[weaponAnimInst.currentStateIndex];
			return !this->isPaused() && !WeaponAnimationUtils::isSheathed(weaponAnimDefState);
		};

		weaponDrawCallInitInfo.renderSpace = UiRenderSpace::Native;
		this->addDrawCall(weaponDrawCallInitInfo);

		UiDrawCallInitInfo compassSliderDrawCallInitInfo;
		compassSliderDrawCallInitInfo.textureID = this->compassSliderTextureRef.get();
		compassSliderDrawCallInitInfo.positionFunc = [this, &game, &player]()
		{
			const Double2 playerDirection = player.getGroundDirectionXZ();
			const Int2 sliderPosition = GameWorldUiView::getCompassSliderPosition(game, playerDirection);
			return sliderPosition;
		};

		compassSliderDrawCallInitInfo.size = this->compassSliderTextureRef.getDimensions();
		compassSliderDrawCallInitInfo.activeFunc = [this, &game]()
		{
			const auto &options = game.options;
			return !this->isPaused() && options.getMisc_ShowCompass();
		};

		compassSliderDrawCallInitInfo.clipRect = GameWorldUiView::getCompassClipRect();
		this->addDrawCall(compassSliderDrawCallInitInfo);

		UiDrawCallInitInfo compassFrameDrawCallInitInfo;
		compassFrameDrawCallInitInfo.textureID = this->compassFrameTextureRef.get();
		compassFrameDrawCallInitInfo.position = GameWorldUiView::getCompassFramePosition();
		compassFrameDrawCallInitInfo.size = this->compassFrameTextureRef.getDimensions();
		compassFrameDrawCallInitInfo.pivotType = PivotType::Top;
		compassFrameDrawCallInitInfo.activeFunc = compassSliderDrawCallInitInfo.activeFunc;
		this->addDrawCall(compassFrameDrawCallInitInfo);

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

		constexpr PivotType statusBarPivotType = GameWorldUiView::StatusBarPivotType;
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

		UiDrawCallInitInfo triggerTextDrawCallInitInfo;
		triggerTextDrawCallInitInfo.textureFunc = [this]() { return this->triggerText.getTextureID(); };
		triggerTextDrawCallInitInfo.position = GameWorldUiView::getTriggerTextPosition(game, this->gameWorldInterfaceTextureRef.getHeight());
		triggerTextDrawCallInitInfo.size = this->triggerText.getRect().getSize();
		triggerTextDrawCallInitInfo.pivotType = PivotType::Bottom;
		triggerTextDrawCallInitInfo.activeFunc = [this, &game]()
		{
			const auto &gameState = game.gameState;
			return !this->isPaused() && gameState.triggerTextIsVisible();
		};

		this->addDrawCall(triggerTextDrawCallInitInfo);

		UiDrawCallInitInfo actionTextDrawCallInitInfo;
		actionTextDrawCallInitInfo.textureFunc = [this]() { return this->actionText.getTextureID(); };
		actionTextDrawCallInitInfo.position = GameWorldUiView::getActionTextPosition();
		actionTextDrawCallInitInfo.size = this->actionText.getRect().getSize();
		actionTextDrawCallInitInfo.pivotType = PivotType::Top;
		actionTextDrawCallInitInfo.activeFunc = [this, &game]()
		{
			const auto &gameState = game.gameState;
			return !this->isPaused() && gameState.actionTextIsVisible();
		};

		this->addDrawCall(actionTextDrawCallInitInfo);

		const Rect effectTextBoxRect = this->effectText.getRect();
		// @todo initialize effect text
		/*UiDrawCallInitInfo effectTextDrawCallInitInfo;
		effectTextDrawCallInitInfo.textureFunc = [this]() { return this->effectText.getTextureID(); };
		effectTextDrawCallInitInfo.position = Int2(effectTextBoxRect.getLeft() + (effectTextBoxRect.width / 2), effectTextBoxRect.getTop());
		effectTextDrawCallInitInfo.size = effectTextBoxRect.getSize();
		effectTextDrawCallInitInfo.pivotType = PivotType::Bottom;
		effectTextDrawCallInitInfo.activeFunc = [this, &game]()
		{
			const auto &gameState = game.gameState;
			return !this->isPaused() && gameState.effectTextIsVisible();
		};

		this->addDrawCall(effectTextDrawCallInitInfo);*/

		UiDrawCallInitInfo reticleDrawCallInitInfo;
		reticleDrawCallInitInfo.textureID = this->modernModeReticleTextureRef.get();
		reticleDrawCallInitInfo.position = GameWorldUiView::getInterfaceCenter(game);
		reticleDrawCallInitInfo.size = this->modernModeReticleTextureRef.getDimensions();
		reticleDrawCallInitInfo.pivotType = PivotType::Middle;
		this->addDrawCall(reticleDrawCallInitInfo);
	}
	else
	{
		UiDrawCallInitInfo weaponDrawCallInitInfo;
		weaponDrawCallInitInfo.textureFunc = [this, &player]()
		{
			const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
			const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
			const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
			const int weaponAnimFrameIndex = WeaponAnimationUtils::getFrameIndex(weaponAnimInst, weaponAnimDef);
			const ScopedUiTextureRef &textureRef = this->weaponAnimTextureRefs.get(weaponAnimFrameIndex);
			return textureRef.get();
		};

		weaponDrawCallInitInfo.positionFunc = [this, &game, &player]()
		{
			const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
			const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
			const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
			const int weaponAnimFrameIndex = WeaponAnimationUtils::getFrameIndex(weaponAnimInst, weaponAnimDef);
			const WeaponAnimationDefinitionFrame &weaponAnimFrame = weaponAnimDef.frames[weaponAnimFrameIndex];
			return Int2(weaponAnimFrame.xOffset, weaponAnimFrame.yOffset);
		};

		weaponDrawCallInitInfo.sizeFunc = [this, &player]()
		{
			const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
			const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
			const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
			const int weaponAnimFrameIndex = WeaponAnimationUtils::getFrameIndex(weaponAnimInst, weaponAnimDef);
			const ScopedUiTextureRef &textureRef = this->weaponAnimTextureRefs.get(weaponAnimFrameIndex);
			return textureRef.getDimensions();
		};

		weaponDrawCallInitInfo.activeFunc = [this, &player]()
		{
			const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
			const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
			const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
			const WeaponAnimationDefinitionState &weaponAnimDefState = weaponAnimDef.states[weaponAnimInst.currentStateIndex];
			return !this->isPaused() && !WeaponAnimationUtils::isSheathed(weaponAnimDefState);
		};

		this->addDrawCall(weaponDrawCallInitInfo);

		UiDrawCallInitInfo gameWorldInterfaceDrawCallInitInfo;
		gameWorldInterfaceDrawCallInitInfo.textureID = this->gameWorldInterfaceTextureRef.get();
		gameWorldInterfaceDrawCallInitInfo.position = GameWorldUiView::getGameWorldInterfacePosition();
		gameWorldInterfaceDrawCallInitInfo.size = this->gameWorldInterfaceTextureRef.getDimensions();
		gameWorldInterfaceDrawCallInitInfo.pivotType = PivotType::Bottom;
		this->addDrawCall(gameWorldInterfaceDrawCallInitInfo);

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

		constexpr PivotType statusBarPivotType = GameWorldUiView::StatusBarPivotType;

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

		const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
		const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(player.charClassDefID);
		if (!charClassDef.castsMagic)
		{
			UiDrawCallInitInfo noMagicDrawCallInitInfo;
			noMagicDrawCallInitInfo.textureID = this->noMagicTextureRef.get();
			noMagicDrawCallInitInfo.position = GameWorldUiView::getNoMagicTexturePosition();
			noMagicDrawCallInitInfo.size = this->noMagicTextureRef.getDimensions();
			this->addDrawCall(noMagicDrawCallInitInfo);
		}

		const Rect playerNameTextBoxRect = this->playerNameTextBox.getRect();
		UiDrawCallInitInfo playerNameDrawCallInitInfo;
		playerNameDrawCallInitInfo.textureID = this->playerNameTextBox.getTextureID();
		playerNameDrawCallInitInfo.position = playerNameTextBoxRect.getTopLeft();
		playerNameDrawCallInitInfo.size = playerNameTextBoxRect.getSize();
		this->addDrawCall(playerNameDrawCallInitInfo);

		UiDrawCallInitInfo compassSliderDrawCallInitInfo;
		compassSliderDrawCallInitInfo.textureID = this->compassSliderTextureRef.get();
		compassSliderDrawCallInitInfo.positionFunc = [this, &game, &player]()
		{
			const Double2 playerDirection = player.getGroundDirectionXZ();
			const Int2 sliderPosition = GameWorldUiView::getCompassSliderPosition(game, playerDirection);
			return sliderPosition;
		};

		compassSliderDrawCallInitInfo.size = this->compassSliderTextureRef.getDimensions();
		compassSliderDrawCallInitInfo.activeFunc = [this, &game]()
		{
			const auto &options = game.options;
			return !this->isPaused() && options.getMisc_ShowCompass();
		};

		compassSliderDrawCallInitInfo.clipRect = GameWorldUiView::getCompassClipRect();
		this->addDrawCall(compassSliderDrawCallInitInfo);

		UiDrawCallInitInfo compassFrameDrawCallInitInfo;
		compassFrameDrawCallInitInfo.textureID = this->compassFrameTextureRef.get();
		compassFrameDrawCallInitInfo.position = GameWorldUiView::getCompassFramePosition();
		compassFrameDrawCallInitInfo.size = this->compassFrameTextureRef.getDimensions();
		compassFrameDrawCallInitInfo.pivotType = PivotType::Top;
		compassFrameDrawCallInitInfo.activeFunc = compassSliderDrawCallInitInfo.activeFunc;
		this->addDrawCall(compassFrameDrawCallInitInfo);

		UiDrawCallInitInfo triggerTextDrawCallInitInfo;
		triggerTextDrawCallInitInfo.textureFunc = [this]() { return this->triggerText.getTextureID(); };
		triggerTextDrawCallInitInfo.position = GameWorldUiView::getTriggerTextPosition(game, this->gameWorldInterfaceTextureRef.getHeight());
		triggerTextDrawCallInitInfo.size = this->triggerText.getRect().getSize();
		triggerTextDrawCallInitInfo.pivotType = PivotType::Bottom;
		triggerTextDrawCallInitInfo.activeFunc = [this, &game]()
		{
			const auto &gameState = game.gameState;
			return !this->isPaused() && gameState.triggerTextIsVisible();
		};

		this->addDrawCall(triggerTextDrawCallInitInfo);

		UiDrawCallInitInfo actionTextDrawCallInitInfo;
		actionTextDrawCallInitInfo.textureFunc = [this]() { return this->actionText.getTextureID(); };
		actionTextDrawCallInitInfo.position = GameWorldUiView::getActionTextPosition();
		actionTextDrawCallInitInfo.size = this->actionText.getRect().getSize();
		actionTextDrawCallInitInfo.pivotType = PivotType::Top;
		actionTextDrawCallInitInfo.activeFunc = [this, &game]()
		{
			const auto &gameState = game.gameState;
			return !this->isPaused() && gameState.actionTextIsVisible();
		};

		this->addDrawCall(actionTextDrawCallInitInfo);

		const Rect effectTextBoxRect = this->effectText.getRect();
		// @todo initialize effect text
		/*UiDrawCallInitInfo effectTextDrawCallInitInfo;
		effectTextDrawCallInitInfo.textureFunc = [this]() { return this->effectText.getTextureID(); };
		effectTextDrawCallInitInfo.position = Int2(effectTextBoxRect.getLeft() + (effectTextBoxRect.width / 2), effectTextBoxRect.getTop());
		effectTextDrawCallInitInfo.size = effectTextBoxRect.getSize();
		effectTextDrawCallInitInfo.pivotType = PivotType::Bottom;
		effectTextDrawCallInitInfo.activeFunc = [this, &game]()
		{
			const auto &gameState = game.gameState;
			return !this->isPaused() && gameState.effectTextIsVisible();
		};

		this->addDrawCall(effectTextDrawCallInitInfo);*/

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

		tooltipDrawCallInitInfo.pivotType = PivotType::BottomLeft;
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

		UiDrawCallPositionFunc cursorPositionFunc = [&game]()
		{
			const auto &inputManager = game.inputManager;
			return inputManager.getMousePosition();
		};

		auto getCursorRegionIndex = [&game, cursorPositionFunc]() -> std::optional<int>
		{
			const Int2 cursorPosition = cursorPositionFunc();

			// See which arrow cursor region the native mouse is in.
			for (int i = 0; i < GameWorldUiView::ArrowCursorRegionCount; i++)
			{
				const Rect &nativeCursorRegion = game.getNativeCursorRegion(i);
				if (nativeCursorRegion.contains(cursorPosition))
				{
					return i;
				}
			}

			// Not in any of the arrow regions.
			return std::nullopt;
		};

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
				return PivotType::TopLeft;
			}

			Span<const PivotType> arrowCursorPivotTypes = GameWorldUiView::ArrowCursorPivotTypes;
			return arrowCursorPivotTypes[*index];
		};

		cursorDrawCallInitInfo.activeFunc = [this]() { return !this->isPaused(); };
		cursorDrawCallInitInfo.renderSpace = UiRenderSpace::Native;
		this->addDrawCall(cursorDrawCallInitInfo);
	}
}

TextBox &GameWorldPanel::getTriggerTextBox()
{
	return this->triggerText;
}

TextBox &GameWorldPanel::getActionTextBox()
{
	return this->actionText;
}

void GameWorldPanel::onPauseChanged(bool paused)
{
	Panel::onPauseChanged(paused);

	auto &game = this->getGame();

	// If in modern mode, set free-look to the given value.
	const auto &options = game.options;
	const bool modernInterface = options.getGraphics_ModernInterface();

	if (modernInterface)
	{
		GameWorldUiModel::setFreeLookActive(game, !paused);
	}

	game.shouldSimulateScene = !paused;
}
