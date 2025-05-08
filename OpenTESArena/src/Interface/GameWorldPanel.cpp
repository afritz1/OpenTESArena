#include <algorithm>
#include <cmath>

#include "SDL.h"

#include "CommonUiView.h"
#include "GameWorldPanel.h"
#include "GameWorldUiController.h"
#include "GameWorldUiModel.h"
#include "GameWorldUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaPortraitUtils.h"
#include "../Assets/ArenaTextureName.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Items/ArenaItemUtils.h"
#include "../Player/WeaponAnimationLibrary.h"
#include "../Rendering/RenderCamera.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../Rendering/RendererUtils.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Time/ArenaClockUtils.h"
#include "../UI/FontLibrary.h"
#include "../World/MapLogicController.h"
#include "../World/MapType.h"

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

	auto &renderer = game.renderer;
	const auto &fontLibrary = FontLibrary::getInstance();
	const std::string playerNameText = GameWorldUiModel::getPlayerNameText(game);
	const TextBox::InitInfo playerNameTextBoxInitInfo =
		GameWorldUiView::getPlayerNameTextBoxInitInfo(playerNameText, fontLibrary);
	if (!this->playerNameTextBox.init(playerNameTextBoxInitInfo, playerNameText, renderer))
	{
		DebugLogError("Couldn't init player name text box.");
		return false;
	}

	const TextBox::InitInfo triggerTextBoxInitInfo = GameWorldUiView::getTriggerTextBoxInitInfo(fontLibrary);
	if (!this->triggerText.init(triggerTextBoxInitInfo, renderer))
	{
		DebugLogError("Couldn't init trigger text box.");
		return false;
	}

	const TextBox::InitInfo actionTextBoxInitInfo = GameWorldUiView::getActionTextBoxInitInfo(fontLibrary);
	if (!this->actionText.init(actionTextBoxInitInfo, renderer))
	{
		DebugLogError("Couldn't init action text box.");
		return false;
	}

	// @todo: effect text box initialization

	this->characterSheetButton = Button<Game&>(
		GameWorldUiView::getCharacterSheetButtonRect(), GameWorldUiController::onCharacterSheetButtonSelected);
	this->drawWeaponButton = Button<Player&>(
		GameWorldUiView::getWeaponSheathButtonRect(), GameWorldUiController::onWeaponButtonSelected);
	this->stealButton = Button<>(
		GameWorldUiView::getStealButtonRect(), GameWorldUiController::onStealButtonSelected);
	this->statusButton = Button<Game&>(
		GameWorldUiView::getStatusButtonRect(), GameWorldUiController::onStatusButtonSelected);
	this->magicButton = Button<>(
		GameWorldUiView::getMagicButtonRect(), GameWorldUiController::onMagicButtonSelected);
	this->logbookButton = Button<Game&>(
		GameWorldUiView::getLogbookButtonRect(), GameWorldUiController::onLogbookButtonSelected);
	this->useItemButton = Button<>(
		GameWorldUiView::getUseItemButtonRect(), GameWorldUiController::onUseItemButtonSelected);
	this->campButton = Button<>(
		GameWorldUiView::getCampButtonRect(), GameWorldUiController::onCampButtonSelected);
	this->scrollUpButton = Button<GameWorldPanel&>(
		GameWorldUiView::getScrollUpButtonRect(), GameWorldUiController::onScrollUpButtonSelected);
	this->scrollDownButton = Button<GameWorldPanel&>(
		GameWorldUiView::getScrollDownButtonRect(), GameWorldUiController::onScrollDownButtonSelected);
	this->mapButton = Button<Game&, bool>(
		GameWorldUiView::getMapButtonRect(), GameWorldUiController::onMapButtonSelected);

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
	auto &game = this->getGame();
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	const auto &options = game.options;
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

	const auto &player = game.player;
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

		UiDrawCall::TextureFunc keyTextureFunc = [this, &game, i, getKeyIdAtIndex]()
		{
			const int keyID = getKeyIdAtIndex(i);
			DebugAssert(keyID != ArenaItemUtils::InvalidDoorKeyID);
			const ScopedUiTextureRef &textureRef = this->keyTextureRefs.get(keyID);
			return textureRef.get();
		};

		UiDrawCall::PositionFunc keyPositionFunc = [i]()
		{
			return GameWorldUiView::getKeyPosition(i);
		};

		UiDrawCall::SizeFunc keySizeFunc = [this, i, getKeyIdAtIndex]()
		{
			const int keyID = getKeyIdAtIndex(i);
			const ScopedUiTextureRef &textureRef = this->keyTextureRefs.get(keyID);
			return Int2(textureRef.getWidth(), textureRef.getHeight());
		};

		UiDrawCall::PivotFunc keyPivotFunc = []() { return PivotType::TopLeft; };

		UiDrawCall::ActiveFunc keyActiveFunc = [this, &game, i, getKeyIdAtIndex]()
		{
			const int keyID = getKeyIdAtIndex(i);
			return !this->isPaused() && (keyID != ArenaItemUtils::InvalidDoorKeyID);
		};

		this->addDrawCall(
			keyTextureFunc,
			keyPositionFunc,
			keySizeFunc,
			keyPivotFunc,
			keyActiveFunc);
	}

	if (modernInterface)
	{
		UiDrawCall::TextureFunc weaponAnimTextureFunc = [this, &player]()
		{
			const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
			const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
			const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
			const int weaponAnimFrameIndex = WeaponAnimationUtils::getFrameIndex(weaponAnimInst, weaponAnimDef);
			const ScopedUiTextureRef &textureRef = this->weaponAnimTextureRefs.get(weaponAnimFrameIndex);
			return textureRef.get();
		};

		UiDrawCall::PositionFunc weaponAnimPositionFunc = [this, &game, &player]()
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

			const auto &renderer = game.renderer;
			const Int2 windowDims = renderer.getWindowDimensions();
			const Int2 nativePosition(
				static_cast<int>(std::round(offsetPercents.x * static_cast<double>(windowDims.x))),
				static_cast<int>(std::round(offsetPercents.y * static_cast<double>(windowDims.y))));
			return nativePosition;
		};

		UiDrawCall::SizeFunc weaponAnimSizeFunc = [this, &game, &player]()
		{
			const int classicViewHeight = ArenaRenderUtils::SCREEN_HEIGHT - this->gameWorldInterfaceTextureRef.getHeight();

			const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
			const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
			const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
			const int weaponAnimFrameIndex = WeaponAnimationUtils::getFrameIndex(weaponAnimInst, weaponAnimDef);
			const ScopedUiTextureRef &textureRef = this->weaponAnimTextureRefs.get(weaponAnimFrameIndex);
			const Int2 textureDims(textureRef.getWidth(), textureRef.getHeight());
			const Double2 texturePercents(
				static_cast<double>(textureDims.x) / ArenaRenderUtils::SCREEN_WIDTH_REAL,
				static_cast<double>(textureDims.y) / static_cast<double>(classicViewHeight));

			const auto &renderer = game.renderer;
			const Int2 windowDims = renderer.getWindowDimensions();
			const Int2 nativeTextureDims(
				static_cast<int>(std::round(texturePercents.x * static_cast<double>(windowDims.x))),
				static_cast<int>(std::round(texturePercents.y * static_cast<double>(windowDims.y))));
			return nativeTextureDims;
		};

		UiDrawCall::PivotFunc weaponAnimPivotFunc = []() { return PivotType::TopLeft; };

		UiDrawCall::ActiveFunc weaponAnimActiveFunc = [this, &player]()
		{
			const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
			const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
			const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
			const WeaponAnimationDefinitionState &weaponAnimDefState = weaponAnimDef.states[weaponAnimInst.currentStateIndex];
			return !this->isPaused() && !WeaponAnimationUtils::isSheathed(weaponAnimDefState);
		};

		this->addDrawCall(
			weaponAnimTextureFunc,
			weaponAnimPositionFunc,
			weaponAnimSizeFunc,
			weaponAnimPivotFunc,
			weaponAnimActiveFunc,
			std::nullopt,
			RenderSpace::Native);

		UiDrawCall::PositionFunc compassSliderPositionFunc = [this, &game, &player]()
		{
			const Double2 playerDirection = player.getGroundDirectionXZ();
			const Int2 sliderPosition = GameWorldUiView::getCompassSliderPosition(game, playerDirection);
			return sliderPosition;
		};

		UiDrawCall::ActiveFunc compassActiveFunc = [this, &game]()
		{
			const auto &options = game.options;
			return !this->isPaused() && options.getMisc_ShowCompass();
		};

		this->addDrawCall(
			[this]() { return this->compassSliderTextureRef.get(); },
			compassSliderPositionFunc,
			[this]() { return Int2(this->compassSliderTextureRef.getWidth(), this->compassSliderTextureRef.getHeight()); },
			[]() { return PivotType::TopLeft; },
			compassActiveFunc,
			GameWorldUiView::getCompassClipRect());
		this->addDrawCall(
			[this]() { return this->compassFrameTextureRef.get(); },
			[]() { return GameWorldUiView::getCompassFramePosition(); },
			[this]() { return Int2(this->compassFrameTextureRef.getWidth(), this->compassFrameTextureRef.getHeight()); },
			[]() { return PivotType::Top; },
			compassActiveFunc);

		UiDrawCall::TextureFunc healthBarTextureFunc = [this]() { return this->healthBarTextureRef.get(); };
		UiDrawCall::TextureFunc staminaBarTextureFunc = [this]() { return this->staminaBarTextureRef.get(); };
		UiDrawCall::TextureFunc spellPointsBarTextureFunc = [this]() { return this->spellPointsBarTextureRef.get(); };

		auto getStatusBarsModernModeOrigin = [&renderer]()
		{
			const Int2 windowDims = renderer.getWindowDimensions();
			return Int2(GameWorldUiView::StatusBarModernModeXOffset, windowDims.y - GameWorldUiView::StatusBarModernModeYOffset);
		};

		auto getStatusBarScaledXDelta = [&renderer](const Rect &statusBarRect)
		{
			const int originalXDelta = statusBarRect.getLeft() - GameWorldUiView::HealthBarRect.getLeft();
			const double originalXPercentDelta = static_cast<double>(originalXDelta) / ArenaRenderUtils::SCREEN_WIDTH_REAL;

			const Int2 windowDims = renderer.getWindowDimensions();
			const double scaleXRatio = static_cast<double>(windowDims.x) / ArenaRenderUtils::SCREEN_WIDTH_REAL;
			const double aspectRatioMultiplier = ArenaRenderUtils::ASPECT_RATIO / renderer.getWindowAspect();
			return static_cast<int>(std::round(static_cast<double>(originalXDelta) * (scaleXRatio * aspectRatioMultiplier)));
		};

		UiDrawCall::PositionFunc healthBarPositionFunc = [&renderer, getStatusBarsModernModeOrigin]()
		{			
			const Int2 windowDims = renderer.getWindowDimensions();
			const Int2 nativePoint = getStatusBarsModernModeOrigin();
			return renderer.nativeToOriginal(nativePoint);
		};

		UiDrawCall::PositionFunc staminaBarPositionFunc = [&renderer, getStatusBarsModernModeOrigin, getStatusBarScaledXDelta]()
		{
			const int scaledXDelta = getStatusBarScaledXDelta(GameWorldUiView::StaminaBarRect);
			const Int2 nativePoint = getStatusBarsModernModeOrigin() + Int2(scaledXDelta, 0);
			return renderer.nativeToOriginal(nativePoint);
		};

		UiDrawCall::PositionFunc spellPointsBarPositionFunc = [&renderer, getStatusBarsModernModeOrigin, getStatusBarScaledXDelta]()
		{
			const int scaledXDelta = getStatusBarScaledXDelta(GameWorldUiView::SpellPointsBarRect);
			const Int2 nativePoint = getStatusBarsModernModeOrigin() + Int2(scaledXDelta, 0);
			return renderer.nativeToOriginal(nativePoint);
		};

		UiDrawCall::SizeFunc healthBarSizeFunc = [&game]()
		{
			const Player &player = game.player;
			const Rect &barRect = GameWorldUiView::HealthBarRect;
			return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentHealth, player.maxHealth));
		};

		UiDrawCall::SizeFunc staminaBarSizeFunc = [&game]()
		{
			const Player &player = game.player;
			const Rect &barRect = GameWorldUiView::StaminaBarRect;
			return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentStamina, player.maxStamina));
		};

		UiDrawCall::SizeFunc spellPointsBarSizeFunc = [&game]()
		{
			const Player &player = game.player;
			const Rect &barRect = GameWorldUiView::SpellPointsBarRect;
			return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentSpellPoints, player.maxSpellPoints));
		};

		UiDrawCall::PivotFunc statusBarPivotFunc = []() { return GameWorldUiView::StatusBarPivotType; };
		UiDrawCall::ActiveFunc statusBarActiveFunc = []() { return true; };

		this->addDrawCall(
			healthBarTextureFunc,
			healthBarPositionFunc,
			healthBarSizeFunc,
			statusBarPivotFunc,
			statusBarActiveFunc);
		this->addDrawCall(
			staminaBarTextureFunc,
			staminaBarPositionFunc,
			staminaBarSizeFunc,
			statusBarPivotFunc,
			statusBarActiveFunc);
		this->addDrawCall(
			spellPointsBarTextureFunc,
			spellPointsBarPositionFunc,
			spellPointsBarSizeFunc,
			statusBarPivotFunc,
			statusBarActiveFunc);

		UiDrawCall::TextureFunc triggerTextTextureFunc = [this]()
		{
			return this->triggerText.getTextureID();
		};

		UiDrawCall::TextureFunc actionTextTextureFunc = [this]()
		{
			return this->actionText.getTextureID();
		};

		UiDrawCall::TextureFunc effectTextTextureFunc = [this]()
		{
			return this->effectText.getTextureID();
		};

		UiDrawCall::ActiveFunc triggerTextActiveFunc = [this, &game]()
		{
			const auto &gameState = game.gameState;
			return !this->isPaused() && gameState.triggerTextIsVisible();
		};

		UiDrawCall::ActiveFunc actionTextActiveFunc = [this, &game]()
		{
			const auto &gameState = game.gameState;
			return !this->isPaused() && gameState.actionTextIsVisible();
		};

		UiDrawCall::ActiveFunc effectTextActiveFunc = [this, &game]()
		{
			const auto &gameState = game.gameState;
			return !this->isPaused() && gameState.effectTextIsVisible();
		};

		const Rect &triggerTextBoxRect = this->triggerText.getRect(); // Position is not usable due to classic/modern mode differences.
		this->addDrawCall(
			triggerTextTextureFunc,
			[this, &game]() { return GameWorldUiView::getTriggerTextPosition(game, this->gameWorldInterfaceTextureRef.getHeight()); },
			[triggerTextBoxRect]() { return triggerTextBoxRect.getSize(); },
			[]() { return PivotType::Bottom; },
			triggerTextActiveFunc);

		const Rect &actionTextBoxRect = this->actionText.getRect(); // Position is not usable due to classic/modern mode differences.
		this->addDrawCall(
			actionTextTextureFunc,
			[]() { return GameWorldUiView::getActionTextPosition(); },
			[actionTextBoxRect]() { return actionTextBoxRect.getSize(); },
			[]() { return PivotType::Top; },
			actionTextActiveFunc);

		const Rect &effectTextBoxRect = this->effectText.getRect();
		this->addDrawCall(
			effectTextTextureFunc,
			[effectTextBoxRect]() { return Int2(effectTextBoxRect.getLeft() + (effectTextBoxRect.width / 2), effectTextBoxRect.getTop()); },
			[effectTextBoxRect]() { return effectTextBoxRect.getSize(); },
			[]() { return PivotType::Bottom; },
			effectTextActiveFunc);

		this->addDrawCall(
			this->modernModeReticleTextureRef.get(),
			GameWorldUiView::getInterfaceCenter(game),
			Int2(this->modernModeReticleTextureRef.getWidth(), this->modernModeReticleTextureRef.getHeight()),
			PivotType::Middle);
	}
	else
	{
		UiDrawCall::TextureFunc weaponAnimTextureFunc = [this, &player]()
		{
			const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
			const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
			const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
			const int weaponAnimFrameIndex = WeaponAnimationUtils::getFrameIndex(weaponAnimInst, weaponAnimDef);
			const ScopedUiTextureRef &textureRef = this->weaponAnimTextureRefs.get(weaponAnimFrameIndex);
			return textureRef.get();
		};

		UiDrawCall::PositionFunc weaponAnimPositionFunc = [this, &game, &player]()
		{
			const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
			const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
			const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
			const int weaponAnimFrameIndex = WeaponAnimationUtils::getFrameIndex(weaponAnimInst, weaponAnimDef);
			const WeaponAnimationDefinitionFrame &weaponAnimFrame = weaponAnimDef.frames[weaponAnimFrameIndex];
			return Int2(weaponAnimFrame.xOffset, weaponAnimFrame.yOffset);
		};

		UiDrawCall::SizeFunc weaponAnimSizeFunc = [this, &player]()
		{
			const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
			const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
			const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
			const int weaponAnimFrameIndex = WeaponAnimationUtils::getFrameIndex(weaponAnimInst, weaponAnimDef);
			const ScopedUiTextureRef &textureRef = this->weaponAnimTextureRefs.get(weaponAnimFrameIndex);
			return Int2(textureRef.getWidth(), textureRef.getHeight());
		};

		UiDrawCall::PivotFunc weaponAnimPivotFunc = []() { return PivotType::TopLeft; };

		UiDrawCall::ActiveFunc weaponAnimActiveFunc = [this, &player]()
		{
			const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
			const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
			const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
			const WeaponAnimationDefinitionState &weaponAnimDefState = weaponAnimDef.states[weaponAnimInst.currentStateIndex];
			return !this->isPaused() && !WeaponAnimationUtils::isSheathed(weaponAnimDefState);
		};

		this->addDrawCall(
			weaponAnimTextureFunc,
			weaponAnimPositionFunc,
			weaponAnimSizeFunc,
			weaponAnimPivotFunc,
			weaponAnimActiveFunc);

		this->addDrawCall(
			this->gameWorldInterfaceTextureRef.get(),
			GameWorldUiView::getGameWorldInterfacePosition(),
			Int2(this->gameWorldInterfaceTextureRef.getWidth(), this->gameWorldInterfaceTextureRef.getHeight()),
			PivotType::Bottom);

		const Rect portraitRect = GameWorldUiView::getPlayerPortraitRect();
		this->addDrawCall(
			this->statusGradientTextureRef.get(),
			portraitRect.getTopLeft(),
			Int2(this->statusGradientTextureRef.getWidth(), this->statusGradientTextureRef.getHeight()),
			PivotType::TopLeft);
		this->addDrawCall(
			this->playerPortraitTextureRef.get(),
			portraitRect.getTopLeft(),
			Int2(this->playerPortraitTextureRef.getWidth(), this->playerPortraitTextureRef.getHeight()),
			PivotType::TopLeft);

		UiDrawCall::TextureFunc healthBarTextureFunc = [this]() { return this->healthBarTextureRef.get(); };
		UiDrawCall::TextureFunc staminaBarTextureFunc = [this]() { return this->staminaBarTextureRef.get(); };
		UiDrawCall::TextureFunc spellPointsBarTextureFunc = [this]() { return this->spellPointsBarTextureRef.get(); };
		UiDrawCall::PositionFunc healthBarPositionFunc = []() { return GameWorldUiView::HealthBarRect.getBottomLeft(); };
		UiDrawCall::PositionFunc staminaBarPositionFunc = []() { return GameWorldUiView::StaminaBarRect.getBottomLeft(); };
		UiDrawCall::PositionFunc spellPointsBarPositionFunc = []() { return GameWorldUiView::SpellPointsBarRect.getBottomLeft(); };

		UiDrawCall::SizeFunc healthBarSizeFunc = [&game]()
		{
			const Player &player = game.player;
			const Rect &barRect = GameWorldUiView::HealthBarRect;
			return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentHealth, player.maxHealth));
		};

		UiDrawCall::SizeFunc staminaBarSizeFunc = [&game]()
		{
			const Player &player = game.player;
			const Rect &barRect = GameWorldUiView::StaminaBarRect;
			return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentStamina, player.maxStamina));
		};

		UiDrawCall::SizeFunc spellPointsBarSizeFunc = [&game]()
		{
			const Player &player = game.player;
			const Rect &barRect = GameWorldUiView::SpellPointsBarRect;
			return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentSpellPoints, player.maxSpellPoints));
		};

		UiDrawCall::PivotFunc statusBarPivotFunc = []() { return GameWorldUiView::StatusBarPivotType; };
		UiDrawCall::ActiveFunc statusBarActiveFunc = []() { return true; };

		this->addDrawCall(
			healthBarTextureFunc,
			healthBarPositionFunc,
			healthBarSizeFunc,
			statusBarPivotFunc,
			statusBarActiveFunc);
		this->addDrawCall(
			staminaBarTextureFunc,
			staminaBarPositionFunc,
			staminaBarSizeFunc,
			statusBarPivotFunc,
			statusBarActiveFunc);
		this->addDrawCall(
			spellPointsBarTextureFunc,
			spellPointsBarPositionFunc,
			spellPointsBarSizeFunc,
			statusBarPivotFunc,
			statusBarActiveFunc);

		const auto &charClassLibrary = CharacterClassLibrary::getInstance();
		const auto &charClassDef = charClassLibrary.getDefinition(player.charClassDefID);
		if (!charClassDef.castsMagic)
		{
			this->addDrawCall(
				this->noMagicTextureRef.get(),
				GameWorldUiView::getNoMagicTexturePosition(),
				Int2(this->noMagicTextureRef.getWidth(), this->noMagicTextureRef.getHeight()),
				PivotType::TopLeft);
		}

		const Rect &playerNameTextBoxRect = this->playerNameTextBox.getRect();
		this->addDrawCall(
			this->playerNameTextBox.getTextureID(),
			playerNameTextBoxRect.getTopLeft(),
			playerNameTextBoxRect.getSize(),
			PivotType::TopLeft);

		UiDrawCall::PositionFunc compassSliderPositionFunc = [this, &game, &player]()
		{
			const Double2 playerDirection = player.getGroundDirectionXZ();
			const Int2 sliderPosition = GameWorldUiView::getCompassSliderPosition(game, playerDirection);
			return sliderPosition;
		};

		UiDrawCall::ActiveFunc compassActiveFunc = [this, &game]()
		{
			const auto &options = game.options;
			return !this->isPaused() && options.getMisc_ShowCompass();
		};

		this->addDrawCall(
			[this]() { return this->compassSliderTextureRef.get(); },
			compassSliderPositionFunc,
			[this]() { return Int2(this->compassSliderTextureRef.getWidth(), this->compassSliderTextureRef.getHeight()); },
			[]() { return PivotType::TopLeft; },
			compassActiveFunc,
			GameWorldUiView::getCompassClipRect());
		this->addDrawCall(
			[this]() { return this->compassFrameTextureRef.get(); },
			[]() { return GameWorldUiView::getCompassFramePosition(); },
			[this]() { return Int2(this->compassFrameTextureRef.getWidth(), this->compassFrameTextureRef.getHeight()); },
			[]() { return PivotType::Top; },
			compassActiveFunc);

		UiDrawCall::TextureFunc triggerTextTextureFunc = [this]()
		{
			return this->triggerText.getTextureID();
		};

		UiDrawCall::TextureFunc actionTextTextureFunc = [this]()
		{
			return this->actionText.getTextureID();
		};

		UiDrawCall::TextureFunc effectTextTextureFunc = [this]()
		{
			return this->effectText.getTextureID();
		};

		UiDrawCall::ActiveFunc triggerTextActiveFunc = [this, &game]()
		{
			const auto &gameState = game.gameState;
			return !this->isPaused() && gameState.triggerTextIsVisible();
		};

		UiDrawCall::ActiveFunc actionTextActiveFunc = [this, &game]()
		{
			const auto &gameState = game.gameState;
			return !this->isPaused() && gameState.actionTextIsVisible();
		};

		UiDrawCall::ActiveFunc effectTextActiveFunc = [this, &game]()
		{
			const auto &gameState = game.gameState;
			return !this->isPaused() && gameState.effectTextIsVisible();
		};

		const Rect &triggerTextBoxRect = this->triggerText.getRect(); // Position is not usable due to classic/modern mode differences.
		this->addDrawCall(
			triggerTextTextureFunc,
			[this, &game]() { return GameWorldUiView::getTriggerTextPosition(game, this->gameWorldInterfaceTextureRef.getHeight()); },
			[triggerTextBoxRect]() { return triggerTextBoxRect.getSize(); },
			[]() { return PivotType::Bottom; },
			triggerTextActiveFunc);

		const Rect &actionTextBoxRect = this->actionText.getRect(); // Position is not usable due to classic/modern mode differences.
		this->addDrawCall(
			actionTextTextureFunc,
			[]() { return GameWorldUiView::getActionTextPosition(); },
			[actionTextBoxRect]() { return actionTextBoxRect.getSize(); },
			[]() { return PivotType::Top; },
			actionTextActiveFunc);

		const Rect &effectTextBoxRect = this->effectText.getRect();
		this->addDrawCall(
			effectTextTextureFunc,
			[effectTextBoxRect]() { return Int2(effectTextBoxRect.getLeft() + (effectTextBoxRect.width / 2), effectTextBoxRect.getTop()); },
			[effectTextBoxRect]() { return effectTextBoxRect.getSize(); },
			[]() { return PivotType::Bottom; },
			effectTextActiveFunc);

		const auto &fontLibrary = FontLibrary::getInstance();
		this->tooltipTextureRefs.init(GameWorldUiModel::BUTTON_COUNT);
		for (int i = 0; i < GameWorldUiModel::BUTTON_COUNT; i++)
		{
			const GameWorldUiModel::ButtonType buttonType = static_cast<GameWorldUiModel::ButtonType>(i);
			const UiTextureID tooltipTextureID = GameWorldUiView::allocTooltipTexture(buttonType, fontLibrary, renderer);
			this->tooltipTextureRefs.set(i, ScopedUiTextureRef(tooltipTextureID, renderer));
		}

		UiDrawCall::TextureFunc tooltipTextureFunc = [this, &game]()
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

		UiDrawCall::PositionFunc tooltipPositionFunc = [&game]()
		{
			return GameWorldUiView::getTooltipPosition(game);
		};

		UiDrawCall::SizeFunc tooltipSizeFunc = [this, &game]()
		{
			std::optional<GameWorldUiModel::ButtonType> buttonType = GameWorldUiModel::getHoveredButtonType(game);
			if (!buttonType.has_value())
			{
				DebugCrash("Expected tooltip size func to only be called when actually drawing a tooltip.");
			}

			const int index = static_cast<int>(*buttonType);
			const ScopedUiTextureRef &tooltipTextureRef = this->tooltipTextureRefs.get(index);
			return Int2(tooltipTextureRef.getWidth(), tooltipTextureRef.getHeight());
		};

		UiDrawCall::PivotFunc tooltipPivotFunc = []()
		{
			return PivotType::BottomLeft;
		};

		UiDrawCall::ActiveFunc tooltipActiveFunc = [this, &game]()
		{
			if (this->isPaused())
			{
				return false;
			}

			const std::optional<GameWorldUiModel::ButtonType> buttonType = GameWorldUiModel::getHoveredButtonType(game);
			return buttonType.has_value() && GameWorldUiModel::isButtonTooltipAllowed(*buttonType, game);
		};

		this->addDrawCall(
			tooltipTextureFunc,
			tooltipPositionFunc,
			tooltipSizeFunc,
			tooltipPivotFunc,
			tooltipActiveFunc);

		UiDrawCall::PositionFunc cursorPositionFunc = [&game]()
		{
			const auto &inputManager = game.inputManager;
			return inputManager.getMousePosition();
		};

		auto getCursorRegionIndex = [this, &game, cursorPositionFunc]() -> std::optional<int>
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

		UiDrawCall::TextureFunc cursorTextureFunc = [this, &game, getCursorRegionIndex]()
		{
			const std::optional<int> index = getCursorRegionIndex();
			if (!index.has_value())
			{
				return this->defaultCursorTextureRef.get();
			}

			const ScopedUiTextureRef &arrowCursorTextureRef = this->arrowCursorTextureRefs.get(*index);
			return arrowCursorTextureRef.get();
		};

		UiDrawCall::SizeFunc cursorSizeFunc = [this, &game, getCursorRegionIndex]()
		{
			const std::optional<int> index = getCursorRegionIndex();
			const Int2 textureDims = [this, &index]()
			{
				if (!index.has_value())
				{
					return Int2(this->defaultCursorTextureRef.getWidth(), this->defaultCursorTextureRef.getHeight());
				}

				const ScopedUiTextureRef &arrowCursorTextureRef = this->arrowCursorTextureRefs.get(*index);
				return Int2(arrowCursorTextureRef.getWidth(), arrowCursorTextureRef.getHeight());
			}();

			const auto &options = game.options;
			const double cursorScale = options.getGraphics_CursorScale();
			return Int2(
				static_cast<int>(static_cast<double>(textureDims.x) * cursorScale),
				static_cast<int>(static_cast<double>(textureDims.y) * cursorScale));
		};

		UiDrawCall::PivotFunc cursorPivotFunc = [this, getCursorRegionIndex]()
		{
			const std::optional<int> index = getCursorRegionIndex();
			if (!index.has_value())
			{
				return PivotType::TopLeft;
			}

			constexpr auto &arrowCursorPivotTypes = GameWorldUiView::ArrowCursorPivotTypes;
			DebugAssertIndex(arrowCursorPivotTypes, *index);
			return arrowCursorPivotTypes[*index];
		};

		UiDrawCall::ActiveFunc cursorActiveFunc = [this]()
		{
			return !this->isPaused();
		};

		this->addDrawCall(
			cursorTextureFunc,
			cursorPositionFunc,
			cursorSizeFunc,
			cursorPivotFunc,
			cursorActiveFunc,
			std::nullopt,
			RenderSpace::Native);
	}
}

TextBox &GameWorldPanel::getTriggerTextBox()
{
	return this->triggerText;
}

bool GameWorldPanel::renderScene(Game &game)
{
	static RenderCommandBuffer commandBuffer;
	commandBuffer.clear();

	// Draw game world onto the native frame buffer. The game world buffer might not completely fill
	// up the native buffer (bottom corners), so clearing the native buffer beforehand is still necessary.
	const auto &player = game.player;
	const WorldDouble3 playerPosition = player.getEyePosition();
	const VoxelDouble3 &playerDir = player.forward;

	auto &gameState = game.gameState;
	const MapDefinition &activeMapDef = gameState.getActiveMapDef();
	const WeatherInstance &activeWeatherInst = gameState.getWeatherInstance();

	const SceneManager &sceneManager = game.sceneManager;
	const RenderSkyManager &renderSkyManager = sceneManager.renderSkyManager;
	renderSkyManager.populateCommandBuffer(commandBuffer);

	const RenderVoxelChunkManager &renderVoxelChunkManager = sceneManager.renderVoxelChunkManager;
	renderVoxelChunkManager.populateCommandBuffer(commandBuffer);

	const RenderEntityChunkManager &renderEntityChunkManager = sceneManager.renderEntityChunkManager;
	renderEntityChunkManager.populateCommandBuffer(commandBuffer);

	const RenderWeatherManager &renderWeatherManager = sceneManager.renderWeatherManager;
	const bool isFoggy = gameState.isFogActive();
	renderWeatherManager.populateCommandBuffer(commandBuffer, activeWeatherInst, isFoggy);

	const MapType activeMapType = activeMapDef.getMapType();
	const double ambientPercent = ArenaRenderUtils::getAmbientPercent(gameState.getClock(), activeMapType, isFoggy);
	const double latitude = [&gameState]()
	{
		const LocationDefinition &locationDef = gameState.getLocationDefinition();
		return locationDef.getLatitude();
	}();

	auto &renderer = game.renderer;
	const auto &options = game.options;
	const Degrees fovY = options.getGraphics_VerticalFOV();
	const double viewAspectRatio = renderer.getViewAspect();
	const RenderCamera renderCamera = RendererUtils::makeCamera(playerPosition, playerDir, fovY, viewAspectRatio, options.getGraphics_TallPixelCorrection());
	const ObjectTextureID paletteTextureID = sceneManager.gameWorldPaletteTextureRef.get();
	
	const bool isInterior = gameState.getActiveMapType() == MapType::Interior;
	const double dayPercent = gameState.getDayPercent();
	const bool isBefore6AM = dayPercent < 0.25;
	const bool isAfter6PM = dayPercent > 0.75;

	ObjectTextureID lightTableTextureID = sceneManager.normalLightTableDaytimeTextureRef.get();
	if (isFoggy)
	{
		lightTableTextureID = sceneManager.fogLightTableTextureRef.get();
	}
	else if (isInterior || isBefore6AM || isAfter6PM)
	{
		lightTableTextureID = sceneManager.normalLightTableNightTextureRef.get();
	}

	const DitheringMode ditheringMode = static_cast<DitheringMode>(options.getGraphics_DitheringMode());
	renderer.submitFrame(renderCamera, commandBuffer, ambientPercent, paletteTextureID, lightTableTextureID, renderSkyManager.getBgTextureID(),
		options.getGraphics_RenderThreadsMode(), ditheringMode);

	return true;
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
