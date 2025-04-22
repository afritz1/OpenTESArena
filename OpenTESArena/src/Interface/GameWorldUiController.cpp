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
#include "../Game/Game.h"
#include "../Player/Player.h"
#include "../Player/PlayerLogicController.h"
#include "../Player/WeaponAnimationLibrary.h"
#include "../Time/ArenaClockUtils.h"
#include "../Time/ArenaDateUtils.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace
{
	void GetDefaultStatusPopUpInitValues(Game &game, const std::string &text, Int2 *outCenter, TextBox::InitInfo *outTextBoxInitInfo, UiTextureID *outTextureID)
	{
		*outCenter = GameWorldUiView::getStatusPopUpTextCenterPoint(game);
		*outTextBoxInitInfo = TextBox::InitInfo::makeWithCenter(
			text,
			*outCenter,
			GameWorldUiView::StatusPopUpFontName,
			GameWorldUiView::StatusPopUpTextColor,
			GameWorldUiView::StatusPopUpTextAlignment,
			std::nullopt,
			GameWorldUiView::StatusPopUpTextLineSpacing,
			FontLibrary::getInstance());

		auto &textureManager = game.textureManager;
		auto &renderer = game.renderer;
		Surface surface = TextureUtils::generate(
			GameWorldUiView::StatusPopUpTexturePatternType,
			GameWorldUiView::getStatusPopUpTextureWidth(outTextBoxInitInfo->rect.getWidth()),
			GameWorldUiView::getStatusPopUpTextureHeight(outTextBoxInitInfo->rect.getHeight()),
			textureManager,
			renderer);

		if (!TextureUtils::tryAllocUiTextureFromSurface(surface, textureManager, renderer, outTextureID))
		{
			DebugCrash("Couldn't create default status pop-up texture.");
		}
	}
}

void GameWorldUiController::onActivate(Game &game, const Int2 &screenPoint, TextBox &actionText)
{
	constexpr bool primaryInteraction = true;
	const auto &inputManager = game.inputManager;
	const bool debugFadeVoxel = inputManager.keyIsDown(SDL_SCANCODE_G);
	PlayerLogicController::handleScreenToWorldInteraction(game, screenPoint, primaryInteraction, debugFadeVoxel, actionText);
}

void GameWorldUiController::onActivateInputAction(const InputActionCallbackValues &values, TextBox &actionText)
{
	if (values.performed)
	{
		Game &game = values.game;
		const auto &options = game.options;
		if (options.getGraphics_ModernInterface())
		{
			const Int2 screenPoint = GameWorldUiView::getNativeWindowCenter(game.renderer);
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
		const auto &options = game.options;
		if (options.getGraphics_ModernInterface())
		{
			const Int2 screenPoint = GameWorldUiView::getNativeWindowCenter(game.renderer);
			GameWorldUiController::onInspect(game, screenPoint, actionText);
		}
	}
}

void GameWorldUiController::onMouseButtonChanged(Game &game, MouseButtonType type, const Int2 &position, bool pressed,
	const Rect &centerCursorRegion, TextBox &actionText)
{
	const auto &options = game.options;
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
	const auto &options = game.options;
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

void GameWorldUiController::onStealButtonSelected()
{
	DebugLog("Steal.");
}

void GameWorldUiController::onStatusButtonSelected(Game &game)
{
	const std::string text = GameWorldUiModel::getStatusButtonText(game);

	Int2 center;
	TextBox::InitInfo textBoxInitInfo;
	UiTextureID textureID;
	GetDefaultStatusPopUpInitValues(game, text, &center, &textBoxInitInfo, &textureID);

	Renderer &renderer = game.renderer;
	ScopedUiTextureRef textureRef(textureID, renderer);
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, GameWorldUiController::onStatusPopUpSelected, std::move(textureRef), center);
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
		auto &gameState = game.gameState;
		const LocationDefinition &locationDef = gameState.getLocationDefinition();
		const LocationInstance &locationInst = gameState.getLocationInstance();
		const int activeLevelIndex = gameState.getActiveLevelIndex();
		const SceneManager &sceneManager = game.sceneManager;
		const VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;

		// Some places (like named/wild dungeons) do not display a name on the automap.
		const std::string automapLocationName = [&gameState, &exeData, &locationDef, &locationInst]()
		{
			const std::string &locationName = locationInst.getName(locationDef);
			const bool isCity = locationDef.getType() == LocationDefinitionType::City;
			const bool isMainQuestDungeon = locationDef.getType() == LocationDefinitionType::MainQuestDungeon;
			return (isCity || isMainQuestDungeon) ? locationName : std::string();
		}();

		const auto &player = game.player;
		game.setPanel<AutomapPanel>(player.getEyeCoord(), player.getGroundDirectionXZ(), voxelChunkManager, automapLocationName);
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
		auto &options = values.game.options;
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

		auto &gameState = game.gameState;
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

void GameWorldUiController::onKeyPickedUp(Game &game, int keyID, const ExeData &exeData)
{
	const std::string text = GameWorldUiModel::getKeyPickUpMessage(keyID, exeData);
	
	Int2 center;
	TextBox::InitInfo textBoxInitInfo;
	UiTextureID textureID;
	GetDefaultStatusPopUpInitValues(game, text, &center, &textBoxInitInfo, &textureID);

	Renderer &renderer = game.renderer;
	ScopedUiTextureRef textureRef(textureID, renderer);
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, GameWorldUiController::onStatusPopUpSelected, std::move(textureRef), center);
}

void GameWorldUiController::onDoorUnlockedWithKey(Game &game, int keyID, const std::string &soundFilename, const WorldDouble3 &soundPosition, const ExeData &exeData)
{
	const std::string text = GameWorldUiModel::getDoorUnlockWithKeyMessage(keyID, exeData);

	Int2 center;
	TextBox::InitInfo textBoxInitInfo;
	UiTextureID textureID;
	GetDefaultStatusPopUpInitValues(game, text, &center, &textBoxInitInfo, &textureID);

	auto onCloseCallback = [soundFilename, soundPosition](Game &game)
	{
		GameWorldUiController::onStatusPopUpSelected(game);

		AudioManager &audioManager = game.audioManager;
		audioManager.playSound(soundFilename.c_str(), soundPosition);
	};

	Renderer &renderer = game.renderer;
	ScopedUiTextureRef textureRef(textureID, renderer);
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, onCloseCallback, std::move(textureRef), center);
}
