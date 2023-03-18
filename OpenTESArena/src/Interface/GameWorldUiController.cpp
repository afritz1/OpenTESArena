#include "AutomapPanel.h"
#include "CharacterPanel.h"
#include "GameWorldPanel.h"
#include "GameWorldUiController.h"
#include "GameWorldUiModel.h"
#include "GameWorldUiView.h"
#include "LogbookPanel.h"
#include "PauseMenuPanel.h"
#include "TextSubPanel.h"
#include "WorldMapPanel.h"
#include "../Entities/Player.h"
#include "../Game/ArenaClockUtils.h"
#include "../Game/ArenaDateUtils.h"
#include "../Game/Game.h"
#include "../GameLogic/PlayerLogicController.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

void GameWorldUiController::onActivate(Game &game, const Int2 &screenPoint, TextBox &actionText)
{
	constexpr bool primaryInteraction = true;
	const auto &inputManager = game.getInputManager();
	const bool debugFadeVoxel = inputManager.keyIsDown(SDL_SCANCODE_G);
	PlayerLogicController::handleScreenToWorldInteraction(game, screenPoint, primaryInteraction,
		debugFadeVoxel, actionText);
}

void GameWorldUiController::onActivateInputAction(const InputActionCallbackValues &values, TextBox &actionText)
{
	if (values.performed)
	{
		Game &game = values.game;
		const auto &options = game.getOptions();
		if (options.getGraphics_ModernInterface())
		{
			const Int2 screenPoint = GameWorldUiView::getNativeWindowCenter(game.getRenderer());
			GameWorldUiController::onActivate(game, screenPoint, actionText);
		}
	}
}

void GameWorldUiController::onInspect(Game &game, const Int2 &screenPoint, TextBox &actionText)
{
	constexpr bool primaryInteraction = false;
	constexpr bool debugFadeVoxel = false;
	PlayerLogicController::handleScreenToWorldInteraction(game, screenPoint, primaryInteraction,
		debugFadeVoxel, actionText);
}

void GameWorldUiController::onInspectInputAction(const InputActionCallbackValues &values, TextBox &actionText)
{
	if (values.performed)
	{
		Game &game = values.game;
		const auto &options = game.getOptions();
		if (options.getGraphics_ModernInterface())
		{
			const Int2 screenPoint = GameWorldUiView::getNativeWindowCenter(game.getRenderer());
			GameWorldUiController::onInspect(game, screenPoint, actionText);
		}
	}
}

void GameWorldUiController::onMouseButtonChanged(Game &game, MouseButtonType type, const Int2 &position, bool pressed,
	const Rect &centerCursorRegion, TextBox &actionText)
{
	const auto &options = game.getOptions();
	if (!options.getGraphics_ModernInterface() && pressed && centerCursorRegion.contains(position))
	{
		if (type == MouseButtonType::Left)
		{
			GameWorldUiController::onActivate(game, position, actionText);
		}
		else if (type == MouseButtonType::Right)
		{
			GameWorldUiController::onInspect(game, position, actionText);
		}
	}
}

void GameWorldUiController::onMouseButtonHeld(Game &game, MouseButtonType type, const Int2 &position, double dt,
	const Rect &centerCursorRegion)
{
	const auto &options = game.getOptions();
	if (!options.getGraphics_ModernInterface() && !centerCursorRegion.contains(position))
	{
		if (type == MouseButtonType::Left)
		{
			// @todo: move out of PlayerLogicController::handlePlayerTurning() and handlePlayerAttack()
		}
	}
}

void GameWorldUiController::onCharacterSheetButtonSelected(Game &game)
{
	game.setPanel<CharacterPanel>();
}

void GameWorldUiController::onWeaponButtonSelected(Player &player)
{
	WeaponAnimation &weaponAnimation = player.getWeaponAnimation();

	if (weaponAnimation.isSheathed())
	{
		// Begin unsheathing the weapon.
		weaponAnimation.setState(WeaponAnimation::State::Unsheathing);
	}
	else if (weaponAnimation.isIdle())
	{
		// Begin sheathing the weapon.
		weaponAnimation.setState(WeaponAnimation::State::Sheathing);
	}
}

void GameWorldUiController::onStealButtonSelected()
{
	DebugLog("Steal.");
}

void GameWorldUiController::onStatusButtonSelected(Game &game)
{
	const std::string text = GameWorldUiModel::getStatusButtonText(game);
	const Int2 center = GameWorldUiView::getStatusPopUpTextCenterPoint(game);
	const TextBox::InitInfo textBoxInitInfo = TextBox::InitInfo::makeWithCenter(
		text,
		center,
		GameWorldUiView::StatusPopUpFontName,
		GameWorldUiView::StatusPopUpTextColor,
		GameWorldUiView::StatusPopUpTextAlignment,
		std::nullopt,
		GameWorldUiView::StatusPopUpTextLineSpacing,
		game.getFontLibrary());

	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();
	Surface surface = TextureUtils::generate(
		GameWorldUiView::StatusPopUpTexturePatternType,
		GameWorldUiView::getStatusPopUpTextureWidth(textBoxInitInfo.rect.getWidth()),
		GameWorldUiView::getStatusPopUpTextureHeight(textBoxInitInfo.rect.getHeight()),
		game.getTextureManager(),
		renderer);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTextureFromSurface(surface, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create status pop-up texture.");
	}

	ScopedUiTextureRef textureRef(textureID, renderer);
	const Int2 textureCenter = center;
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, GameWorldUiController::onStatusPopUpSelected,
		std::move(textureRef), textureCenter);
}

void GameWorldUiController::onStatusPopUpSelected(Game &game)
{
	game.popSubPanel();
}

void GameWorldUiController::onMagicButtonSelected()
{
	DebugLog("Magic.");
}

void GameWorldUiController::onMapButtonSelected(Game &game, bool goToAutomap)
{
	if (goToAutomap)
	{
		const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
		auto &gameState = game.getGameState();
		const LocationDefinition &locationDef = gameState.getLocationDefinition();
		const LocationInstance &locationInst = gameState.getLocationInstance();
		const MapDefinition &mapDef = gameState.getActiveMapDef();
		const MapInstance &mapInst = gameState.getActiveMapInst();
		const int activeLevelIndex = mapInst.getActiveLevelIndex();
		const LevelDefinition &levelDef = mapDef.getLevel(activeLevelIndex);
		const LevelInfoDefinition &levelInfoDef = mapDef.getLevelInfoForLevel(activeLevelIndex);
		const LevelInstance &levelInst = mapInst.getLevel(activeLevelIndex);

		// Some places (like named/wild dungeons) do not display a name on the automap.
		const std::string automapLocationName = [&gameState, &exeData, &locationDef, &locationInst]()
		{
			const std::string &locationName = locationInst.getName(locationDef);
			const bool isCity = locationDef.getType() == LocationDefinition::Type::City;
			const bool isMainQuestDungeon = locationDef.getType() == LocationDefinition::Type::MainQuestDungeon;
			return (isCity || isMainQuestDungeon) ? locationName : std::string();
		}();

		const auto &player = game.getPlayer();
		game.setPanel<AutomapPanel>(player.getPosition(), player.getGroundDirection(),
			levelInst.getVoxelChunkManager(), automapLocationName);
	}
	else
	{
		game.setPanel<WorldMapPanel>();
	}
}

void GameWorldUiController::onLogbookButtonSelected(Game &game)
{
	game.setPanel<LogbookPanel>();
}

void GameWorldUiController::onUseItemButtonSelected()
{
	DebugLog("Use item.");
}

void GameWorldUiController::onCampButtonSelected()
{
	DebugLog("Camp.");
}

void GameWorldUiController::onScrollUpButtonSelected(GameWorldPanel &panel)
{
	// Nothing yet.
}

void GameWorldUiController::onScrollDownButtonSelected(GameWorldPanel &panel)
{
	// Nothing yet.
}

void GameWorldUiController::onToggleCompassInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		auto &options = values.game.getOptions();
		options.setMisc_ShowCompass(!options.getMisc_ShowCompass());
	}
}

void GameWorldUiController::onPlayerPositionInputAction(const InputActionCallbackValues &values, TextBox &actionText)
{
	if (values.performed)
	{
		auto &game = values.game;

		// Refresh player coordinates display (probably intended for debugging in the original game).
		// These coordinates are in Arena's coordinate system.
		const std::string text = GameWorldUiModel::getPlayerPositionText(game);
		actionText.setText(text);

		auto &gameState = game.getGameState();
		gameState.setActionTextDuration(text);
	}
}

void GameWorldUiController::onPauseInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		auto &game = values.game;
		game.setPanel<PauseMenuPanel>();
	}
}
