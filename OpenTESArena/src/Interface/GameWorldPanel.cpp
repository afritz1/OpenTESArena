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
#include "../Entities/CharacterClassLibrary.h"
#include "../Game/ArenaClockUtils.h"
#include "../Game/Game.h"
#include "../GameLogic/MapLogicController.h"
#include "../GameLogic/PlayerLogicController.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Rendering/RenderCamera.h"
#include "../Rendering/RendererUtils.h"
#include "../UI/CursorData.h"
#include "../UI/FontLibrary.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"

GameWorldPanel::GameWorldPanel(Game &game)
	: Panel(game) { }

GameWorldPanel::~GameWorldPanel()
{
	auto &game = this->getGame();
	auto &inputManager = game.getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::GameWorld, false);

	// If in modern mode, disable free-look.
	const auto &options = game.getOptions();
	const bool modernInterface = options.getGraphics_ModernInterface();

	if (modernInterface)
	{
		GameWorldUiModel::setFreeLookActive(game, false);
	}

	game.setIsSimulatingScene(false);
	game.setGameWorldRenderCallback([](Game&) { return true; });
}

bool GameWorldPanel::init()
{
	auto &game = this->getGame();

	auto &renderer = game.getRenderer();
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

	auto &player = game.getPlayer();
	const auto &options = game.getOptions();
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

	auto &inputManager = game.getInputManager();
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
		GameState &gameState = game.getGameState();

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

	game.setGameWorldRenderCallback(GameWorldPanel::gameWorldRenderCallback);
	game.setIsSimulatingScene(true);

	return true;
}

void GameWorldPanel::initUiDrawCalls()
{
	auto &game = this->getGame();
	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();

	const auto &options = game.getOptions();
	const bool modernInterface = options.getGraphics_ModernInterface();

	const UiTextureID gameWorldInterfaceTextureID =
		GameWorldUiView::allocGameWorldInterfaceTexture(textureManager, renderer);
	this->gameWorldInterfaceTextureRef.init(gameWorldInterfaceTextureID, renderer);

	constexpr GameWorldUiView::StatusGradientType gradientType = GameWorldUiView::StatusGradientType::Default;
	const UiTextureID statusGradientTextureID =
		GameWorldUiView::allocStatusGradientTexture(gradientType, textureManager, renderer);
	this->statusGradientTextureRef.init(statusGradientTextureID, renderer);

	const auto &player = game.getPlayer();
	const UiTextureID playerPortraitTextureID = GameWorldUiView::allocPlayerPortraitTexture(
		player.isMale(), player.getRaceID(), player.getPortraitID(), textureManager, renderer);
	this->playerPortraitTextureRef.init(playerPortraitTextureID, renderer);

	const UiTextureID noMagicTextureID = GameWorldUiView::allocNoMagicTexture(textureManager, renderer);
	this->noMagicTextureRef.init(noMagicTextureID, renderer);

	const auto &weaponAnimation = player.getWeaponAnimation();
	const std::string &weaponFilename = weaponAnimation.getAnimationFilename();
	const std::optional<TextureFileMetadataID> weaponAnimMetadataID = textureManager.tryGetMetadataID(weaponFilename.c_str());
	if (!weaponAnimMetadataID.has_value())
	{
		DebugCrash("Couldn't get texture file metadata ID for weapon animation \"" + weaponFilename + "\".");
	}

	const TextureFileMetadata &weaponAnimMetadata = textureManager.getMetadataHandle(*weaponAnimMetadataID);
	this->weaponAnimTextureRefs.init(weaponAnimMetadata.getTextureCount());
	for (int i = 0; i < weaponAnimMetadata.getTextureCount(); i++)
	{
		const UiTextureID weaponTextureID =
			GameWorldUiView::allocWeaponAnimTexture(weaponFilename, i, textureManager, renderer);
		this->weaponAnimTextureRefs.set(i, ScopedUiTextureRef(weaponTextureID, renderer));
	}

	const UiTextureID compassFrameTextureID = GameWorldUiView::allocCompassFrameTexture(textureManager, renderer);
	const UiTextureID compassSliderTextureID = GameWorldUiView::allocCompassSliderTexture(textureManager, renderer);
	this->compassFrameTextureRef.init(compassFrameTextureID, renderer);
	this->compassSliderTextureRef.init(compassSliderTextureID, renderer);

	const UiTextureID defaultCursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->defaultCursorTextureRef.init(defaultCursorTextureID, renderer);

	this->arrowCursorTextureRefs.init(GameWorldUiView::ArrowCursorRegionCount);
	for (int i = 0; i < GameWorldUiView::ArrowCursorRegionCount; i++)
	{
		const UiTextureID arrowTextureID = GameWorldUiView::allocArrowCursorTexture(i, textureManager, renderer);
		this->arrowCursorTextureRefs.set(i, ScopedUiTextureRef(arrowTextureID, renderer));
	}

	if (modernInterface)
	{
		UiDrawCall::TextureFunc weaponAnimTextureFunc = [this, &player]()
		{
			const auto &weaponAnimation = player.getWeaponAnimation();
			const ScopedUiTextureRef &textureRef = this->weaponAnimTextureRefs.get(weaponAnimation.getFrameIndex());
			return textureRef.get();
		};

		UiDrawCall::PositionFunc weaponAnimPositionFunc = [this, &game, &player]()
		{
			const int classicViewHeight = ArenaRenderUtils::SCREEN_HEIGHT - this->gameWorldInterfaceTextureRef.getHeight();

			const auto &weaponAnimation = player.getWeaponAnimation();
			const std::string &weaponFilename = weaponAnimation.getAnimationFilename();
			const int weaponAnimIndex = weaponAnimation.getFrameIndex();

			auto &textureManager = game.getTextureManager();
			const Int2 offset = GameWorldUiView::getWeaponAnimationOffset(weaponFilename, weaponAnimIndex, textureManager);
			const Double2 offsetPercents(
				static_cast<double>(offset.x) / ArenaRenderUtils::SCREEN_WIDTH_REAL,
				static_cast<double>(offset.y) / static_cast<double>(classicViewHeight));

			const auto &renderer = game.getRenderer();
			const Int2 windowDims = renderer.getWindowDimensions();
			const Int2 nativePosition(
				static_cast<int>(std::round(offsetPercents.x * static_cast<double>(windowDims.x))),
				static_cast<int>(std::round(offsetPercents.y * static_cast<double>(windowDims.y))));
			return nativePosition;
		};

		UiDrawCall::SizeFunc weaponAnimSizeFunc = [this, &game, &player]()
		{
			const int classicViewHeight = ArenaRenderUtils::SCREEN_HEIGHT - this->gameWorldInterfaceTextureRef.getHeight();

			const auto &weaponAnimation = player.getWeaponAnimation();
			const ScopedUiTextureRef &textureRef = this->weaponAnimTextureRefs.get(weaponAnimation.getFrameIndex());
			const Int2 textureDims(textureRef.getWidth(), textureRef.getHeight());
			const Double2 texturePercents(
				static_cast<double>(textureDims.x) / ArenaRenderUtils::SCREEN_WIDTH_REAL,
				static_cast<double>(textureDims.y) / static_cast<double>(classicViewHeight));

			const auto &renderer = game.getRenderer();
			const Int2 windowDims = renderer.getWindowDimensions();
			const Int2 nativeTextureDims(
				static_cast<int>(std::round(texturePercents.x * static_cast<double>(windowDims.x))),
				static_cast<int>(std::round(texturePercents.y * static_cast<double>(windowDims.y))));
			return nativeTextureDims;
		};

		UiDrawCall::PivotFunc weaponAnimPivotFunc = []() { return PivotType::TopLeft; };

		UiDrawCall::ActiveFunc weaponAnimActiveFunc = [this, &player]()
		{
			const auto &weaponAnimation = player.getWeaponAnimation();
			return !this->isPaused() && !weaponAnimation.isSheathed();
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
			const VoxelDouble2 &playerDirection = player.getGroundDirection();
			const Int2 sliderPosition = GameWorldUiView::getCompassSliderPosition(game, playerDirection);
			return sliderPosition;
		};

		UiDrawCall::ActiveFunc compassActiveFunc = [this, &game]()
		{
			const auto &options = game.getOptions();
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
			const auto &gameState = game.getGameState();
			return !this->isPaused() && gameState.triggerTextIsVisible();
		};

		UiDrawCall::ActiveFunc actionTextActiveFunc = [this, &game]()
		{
			const auto &gameState = game.getGameState();
			return !this->isPaused() && gameState.actionTextIsVisible();
		};

		UiDrawCall::ActiveFunc effectTextActiveFunc = [this, &game]()
		{
			const auto &gameState = game.getGameState();
			return !this->isPaused() && gameState.effectTextIsVisible();
		};

		const Rect &triggerTextBoxRect = this->triggerText.getRect(); // Position is not usable due to classic/modern mode differences.
		this->addDrawCall(
			triggerTextTextureFunc,
			[this, &game]() { return GameWorldUiView::getTriggerTextPosition(game, this->gameWorldInterfaceTextureRef.getHeight()); },
			[triggerTextBoxRect]() { return Int2(triggerTextBoxRect.getWidth(), triggerTextBoxRect.getHeight()); },
			[]() { return PivotType::Bottom; },
			triggerTextActiveFunc);

		const Rect &actionTextBoxRect = this->actionText.getRect(); // Position is not usable due to classic/modern mode differences.
		this->addDrawCall(
			actionTextTextureFunc,
			[]() { return GameWorldUiView::getActionTextPosition(); },
			[actionTextBoxRect]() { return Int2(actionTextBoxRect.getWidth(), actionTextBoxRect.getHeight()); },
			[]() { return PivotType::Top; },
			actionTextActiveFunc);

		const Rect &effectTextBoxRect = this->effectText.getRect();
		this->addDrawCall(
			effectTextTextureFunc,
			[effectTextBoxRect]() { return Int2(effectTextBoxRect.getLeft() + (effectTextBoxRect.getWidth() / 2), effectTextBoxRect.getTop()); },
			[effectTextBoxRect]() { return Int2(effectTextBoxRect.getWidth(), effectTextBoxRect.getHeight()); },
			[]() { return PivotType::Bottom; },
			effectTextActiveFunc);
	}
	else
	{
		UiDrawCall::TextureFunc weaponAnimTextureFunc = [this, &player]()
		{
			const auto &weaponAnimation = player.getWeaponAnimation();
			const ScopedUiTextureRef &textureRef = this->weaponAnimTextureRefs.get(weaponAnimation.getFrameIndex());
			return textureRef.get();
		};

		UiDrawCall::PositionFunc weaponAnimPositionFunc = [this, &game, &player]()
		{
			const auto &weaponAnimation = player.getWeaponAnimation();
			const std::string &weaponFilename = weaponAnimation.getAnimationFilename();
			const int weaponAnimIndex = weaponAnimation.getFrameIndex();

			auto &textureManager = game.getTextureManager();
			const Int2 offset = GameWorldUiView::getWeaponAnimationOffset(weaponFilename, weaponAnimIndex, textureManager);
			return offset;
		};

		UiDrawCall::SizeFunc weaponAnimSizeFunc = [this, &player]()
		{
			const auto &weaponAnimation = player.getWeaponAnimation();
			const ScopedUiTextureRef &textureRef = this->weaponAnimTextureRefs.get(weaponAnimation.getFrameIndex());
			return Int2(textureRef.getWidth(), textureRef.getHeight());
		};

		UiDrawCall::PivotFunc weaponAnimPivotFunc = []() { return PivotType::TopLeft; };

		UiDrawCall::ActiveFunc weaponAnimActiveFunc = [this, &player]()
		{
			const auto &weaponAnimation = player.getWeaponAnimation();
			return !this->isPaused() && !weaponAnimation.isSheathed();
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

		const auto &charClassLibrary = CharacterClassLibrary::getInstance();
		const auto &charClassDef = charClassLibrary.getDefinition(player.getCharacterClassDefID());
		if (!charClassDef.canCastMagic())
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
			Int2(playerNameTextBoxRect.getWidth(), playerNameTextBoxRect.getHeight()),
			PivotType::TopLeft);

		UiDrawCall::PositionFunc compassSliderPositionFunc = [this, &game, &player]()
		{
			const VoxelDouble2 &playerDirection = player.getGroundDirection();
			const Int2 sliderPosition = GameWorldUiView::getCompassSliderPosition(game, playerDirection);
			return sliderPosition;
		};

		UiDrawCall::ActiveFunc compassActiveFunc = [this, &game]()
		{
			const auto &options = game.getOptions();
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
			const auto &gameState = game.getGameState();
			return !this->isPaused() && gameState.triggerTextIsVisible();
		};

		UiDrawCall::ActiveFunc actionTextActiveFunc = [this, &game]()
		{
			const auto &gameState = game.getGameState();
			return !this->isPaused() && gameState.actionTextIsVisible();
		};

		UiDrawCall::ActiveFunc effectTextActiveFunc = [this, &game]()
		{
			const auto &gameState = game.getGameState();
			return !this->isPaused() && gameState.effectTextIsVisible();
		};

		const Rect &triggerTextBoxRect = this->triggerText.getRect(); // Position is not usable due to classic/modern mode differences.
		this->addDrawCall(
			triggerTextTextureFunc,
			[this, &game]() { return GameWorldUiView::getTriggerTextPosition(game, this->gameWorldInterfaceTextureRef.getHeight()); },
			[triggerTextBoxRect]() { return Int2(triggerTextBoxRect.getWidth(), triggerTextBoxRect.getHeight()); },
			[]() { return PivotType::Bottom; },
			triggerTextActiveFunc);

		const Rect &actionTextBoxRect = this->actionText.getRect(); // Position is not usable due to classic/modern mode differences.
		this->addDrawCall(
			actionTextTextureFunc,
			[]() { return GameWorldUiView::getActionTextPosition(); },
			[actionTextBoxRect]() { return Int2(actionTextBoxRect.getWidth(), actionTextBoxRect.getHeight()); },
			[]() { return PivotType::Top; },
			actionTextActiveFunc);

		const Rect &effectTextBoxRect = this->effectText.getRect();
		this->addDrawCall(
			effectTextTextureFunc,
			[effectTextBoxRect]() { return Int2(effectTextBoxRect.getLeft() + (effectTextBoxRect.getWidth() / 2), effectTextBoxRect.getTop()); },
			[effectTextBoxRect]() { return Int2(effectTextBoxRect.getWidth(), effectTextBoxRect.getHeight()); },
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
			const auto &inputManager = game.getInputManager();
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

			const auto &options = game.getOptions();
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

bool GameWorldPanel::gameWorldRenderCallback(Game &game)
{
	// Draw game world onto the native frame buffer. The game world buffer might not completely fill
	// up the native buffer (bottom corners), so clearing the native buffer beforehand is still necessary.
	const auto &player = game.getPlayer();
	const CoordDouble3 &playerPos = player.getPosition();
	const VoxelDouble3 &playerDir = player.getDirection();

	auto &gameState = game.getGameState();
	const MapDefinition &activeMapDef = gameState.getActiveMapDef();
	const MapInstance &activeMapInst = gameState.getActiveMapInst();
	const LevelInstance &activeLevelInst = activeMapInst.getActiveLevel();
	const SkyInstance &activeSkyInst = activeMapInst.getActiveSky();
	const WeatherInstance &activeWeatherInst = gameState.getWeatherInstance();

	const RenderChunkManager &renderChunkManager = game.getRenderChunkManager();
	const BufferView<const RenderDrawCall> drawCalls = renderChunkManager.getTotalDrawCalls();

	// @todo: determine which of these per-frame values will go in draw calls instead for voxels/entities/sky
	const double ambientPercent = gameState.getAmbientPercent();
	const double latitude = [&gameState]()
	{
		const LocationDefinition &locationDef = gameState.getLocationDefinition();
		return locationDef.getLatitude();
	}();

	const bool isExterior = activeMapDef.getSubDefinition().type != MapType::Interior;

	auto &renderer = game.getRenderer();
	const auto &options = game.getOptions();
	const Degrees fovY = options.getGraphics_VerticalFOV();
	const double viewAspectRatio = renderer.getViewAspect();
	const RenderCamera renderCamera = RendererUtils::makeCamera(playerPos.chunk, playerPos.point, playerDir, fovY, viewAspectRatio, options.getGraphics_TallPixelCorrection());

	// @todo: shore up loadScene() and that overall design before attempting to change dirty voxels and entities between frames.
	/*renderer.updateScene(renderCamera, activeLevelInst, activeSkyInst, gameState.getDaytimePercent(),
		latitude, gameState.getChasmAnimPercent(), gameState.nightLightsAreActive(), options.getMisc_PlayerHasLight(),
		EntityDefinitionLibrary::getInstance());*/

	// @todo: get all object texture IDs properly (probably want whoever owns them to use ScopedObjectTextureRef)
	const ObjectTextureID paletteTextureID = activeLevelInst.getPaletteTextureID();
	const ObjectTextureID lightTableTextureID = activeLevelInst.getLightTableTextureID();
	const ObjectTextureID skyColorsTextureID = activeSkyInst.getSkyColorsTextureID();
	const ObjectTextureID thunderstormColorsTextureID = -1;

	renderer.submitFrame(renderCamera, drawCalls, ambientPercent, paletteTextureID, lightTableTextureID,
		skyColorsTextureID, thunderstormColorsTextureID, options.getGraphics_RenderThreadsMode());

	return true;
}

TextBox &GameWorldPanel::getTriggerTextBox()
{
	return this->triggerText;
}

void GameWorldPanel::onPauseChanged(bool paused)
{
	Panel::onPauseChanged(paused);

	auto &game = this->getGame();

	// If in modern mode, set free-look to the given value.
	const auto &options = game.getOptions();
	const bool modernInterface = options.getGraphics_ModernInterface();

	if (modernInterface)
	{
		GameWorldUiModel::setFreeLookActive(game, !paused);
	}

	game.setIsSimulatingScene(!paused);
}
