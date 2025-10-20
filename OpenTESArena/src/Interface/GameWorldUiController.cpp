#include "AutomapPanel.h"
#include "CharacterPanel.h"
#include "CinematicLibrary.h"
#include "GameWorldPanel.h"
#include "GameWorldUiController.h"
#include "GameWorldUiModel.h"
#include "GameWorldUiView.h"
#include "LogbookPanel.h"
#include "LootSubPanel.h"
#include "PauseMenuPanel.h"
#include "TextSubPanel.h"
#include "WorldMapPanel.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Audio/MusicUtils.h"
#include "../Entities/ArenaCitizenUtils.h"
#include "../Entities/ArenaEntityUtils.h"
#include "../Game/Game.h"
#include "../Interface/PauseMenuUiController.h"
#include "../Interface/TextCinematicPanel.h"
#include "../Player/Player.h"
#include "../Player/PlayerLogic.h"
#include "../Player/WeaponAnimationLibrary.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Stats/CharacterRaceLibrary.h"
#include "../Time/ArenaClockUtils.h"
#include "../Time/ArenaDateUtils.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../World/MapType.h"
#include "../WorldMap/ArenaLocationUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace
{
	void GetDefaultStatusPopUpInitValues(Game &game, const std::string &text, Int2 *outCenter, TextBoxInitInfo *outTextBoxInitInfo, UiTextureID *outTextureID)
	{
		*outCenter = GameWorldUiView::getStatusPopUpTextCenterPoint(game);
		*outTextBoxInitInfo = TextBoxInitInfo::makeWithCenter(
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
			GameWorldUiView::getStatusPopUpTextureWidth(outTextBoxInitInfo->rect.width),
			GameWorldUiView::getStatusPopUpTextureHeight(outTextBoxInitInfo->rect.height),
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
	PlayerLogic::handleScreenToWorldInteraction(game, screenPoint, primaryInteraction, debugFadeVoxel, actionText);
}

void GameWorldUiController::onActivateInputAction(const InputActionCallbackValues &values, TextBox &actionText)
{
	if (values.performed)
	{
		Game &game = values.game;
		const auto &options = game.options;
		if (options.getGraphics_ModernInterface())
		{
			const Int2 screenPoint = GameWorldUiView::getNativeWindowCenter(game.window);
			GameWorldUiController::onActivate(game, screenPoint, actionText);
		}
	}
}

void GameWorldUiController::onInspect(Game &game, const Int2 &screenPoint, TextBox &actionText)
{
	constexpr bool primaryInteraction = false;
	constexpr bool debugFadeVoxel = false;
	PlayerLogic::handleScreenToWorldInteraction(game, screenPoint, primaryInteraction, debugFadeVoxel, actionText);
}

void GameWorldUiController::onInspectInputAction(const InputActionCallbackValues &values, TextBox &actionText)
{
	if (values.performed)
	{
		Game &game = values.game;
		const auto &options = game.options;
		if (options.getGraphics_ModernInterface())
		{
			const Int2 screenPoint = GameWorldUiView::getNativeWindowCenter(game.window);
			GameWorldUiController::onInspect(game, screenPoint, actionText);
		}
	}
}

void GameWorldUiController::onMouseButtonChanged(Game &game, MouseButtonType type, const Int2 &position, bool pressed,
	const Rect &centerCursorRegion, TextBox &actionText)
{
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
					GameWorldUiController::onActivate(game, position, actionText);
				}
				else if (isRightClick)
				{
					GameWorldUiController::onInspect(game, position, actionText);
				}
			}
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
	TextBoxInitInfo textBoxInitInfo;
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

void GameWorldUiController::onEnemyAliveInspected(Game &game, EntityInstanceID entityInstID, const EntityDefinition &entityDef, TextBox &actionTextBox)
{
	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();

	std::string entityName;
	if (!EntityUtils::tryGetDisplayName(entityDef, charClassLibrary, &entityName))
	{
		DebugLogErrorFormat("Expected enemy entity %d to have display name.", entityInstID);
		return;
	}

	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const ExeData &exeData = binaryAssetLibrary.getExeData();
	const std::string text = GameWorldUiModel::getEnemyInspectedMessage(entityName, exeData);
	actionTextBox.setText(text);

	GameState &gameState = game.gameState;
	gameState.setActionTextDuration(text);
}

void GameWorldUiController::onContainerInventoryOpened(Game &game, EntityInstanceID entityInstID, ItemInventory &itemInventory, bool destroyEntityIfEmpty)
{
	// @todo: need to queue entity destroy if container is empty
	// @todo: if closing and container is not empty, then inventory.compact(). Don't compact while removing items since that would invalidate mappings

	auto callback = [entityInstID, &itemInventory, destroyEntityIfEmpty](Game &game)
	{
		if (destroyEntityIfEmpty && (itemInventory.getOccupiedSlotCount() == 0))
		{
			EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
			entityChunkManager.queueEntityDestroy(entityInstID, true);
		}

		GameWorldUiController::onStatusPopUpSelected(game);
	};

	game.pushSubPanel<LootSubPanel>(itemInventory, callback);
}

void GameWorldUiController::onEnemyCorpseInteracted(Game &game, EntityInstanceID entityInstID, const EntityDefinition &entityDef)
{
	EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
	const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
	ItemInventory &enemyItemInventory = entityChunkManager.getEntityItemInventory(entityInst.itemInventoryInstID);

	if (enemyItemInventory.getOccupiedSlotCount() > 0)
	{
		constexpr bool destroyEntityIfEmpty = false; // Don't remove empty corpses.
		GameWorldUiController::onContainerInventoryOpened(game, entityInstID, enemyItemInventory, destroyEntityIfEmpty);
	}
	else
	{
		GameWorldUiController::onEnemyCorpseEmptyInventoryOpened(game, entityInstID, entityDef);
	}
}

void GameWorldUiController::onEnemyCorpseInteractedFirstTime(Game &game, EntityInstanceID entityInstID, const EntityDefinition &entityDef)
{
	Random &random = game.random;

	int corpseGoldCount = 0;
	if (entityDef.type == EntityDefinitionType::Enemy)
	{
		const EnemyEntityDefinition &enemyDef = entityDef.enemy;

		int creatureLevel = 1;
		if (enemyDef.type == EnemyEntityDefinitionType::Creature)
		{
			creatureLevel = enemyDef.creature.level;
		}

		corpseGoldCount = ArenaEntityUtils::getCreatureGold(creatureLevel, enemyDef.creature.lootChances, random); // @todo This should be done at creature instantiation
	}

	if (corpseGoldCount == 0)
	{
		GameWorldUiController::onEnemyCorpseInteracted(game, entityInstID, entityDef);
		return;
	}

	Player &player = game.player;
	player.gold += corpseGoldCount;

	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const ExeData &exeData = binaryAssetLibrary.getExeData();
	const std::string text = GameWorldUiModel::getEnemyCorpseGoldMessage(corpseGoldCount, exeData);

	Int2 center;
	TextBoxInitInfo textBoxInitInfo;
	UiTextureID textureID;
	GetDefaultStatusPopUpInitValues(game, text, &center, &textBoxInitInfo, &textureID);

	auto callback = [entityInstID, entityDef](Game &game)
	{
		GameWorldUiController::onStatusPopUpSelected(game);

		EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
		const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
		ItemInventory &enemyItemInventory = entityChunkManager.getEntityItemInventory(entityInst.itemInventoryInstID);

		if (enemyItemInventory.getOccupiedSlotCount() > 0)
		{
			constexpr bool destroyEntityIfEmpty = false; // Don't remove empty corpses.
			GameWorldUiController::onContainerInventoryOpened(game, entityInstID, enemyItemInventory, destroyEntityIfEmpty);
		}
	};

	ScopedUiTextureRef textureRef(textureID, game.renderer);
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, callback, std::move(textureRef), center);
}

void GameWorldUiController::onEnemyCorpseEmptyInventoryOpened(Game &game, EntityInstanceID entityInstID, const EntityDefinition &entityDef)
{
	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();

	std::string entityName;
	if (!EntityUtils::tryGetDisplayName(entityDef, charClassLibrary, &entityName))
	{
		DebugLogErrorFormat("Expected enemy entity %d to have display name.", entityInstID);
		return;
	}

	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const ExeData &exeData = binaryAssetLibrary.getExeData();
	const std::string text = GameWorldUiModel::getEnemyCorpseEmptyInventoryMessage(entityName, exeData);

	Int2 center;
	TextBoxInitInfo textBoxInitInfo;
	UiTextureID textureID;
	GetDefaultStatusPopUpInitValues(game, text, &center, &textBoxInitInfo, &textureID);

	ScopedUiTextureRef textureRef(textureID, game.renderer);
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, GameWorldUiController::onStatusPopUpSelected, std::move(textureRef), center);
}

void GameWorldUiController::onKeyPickedUp(Game &game, int keyID, const ExeData &exeData, const std::function<void()> postStatusPopUpCallback)
{
	const std::string text = GameWorldUiModel::getKeyPickUpMessage(keyID, exeData);

	Int2 center;
	TextBoxInitInfo textBoxInitInfo;
	UiTextureID textureID;
	GetDefaultStatusPopUpInitValues(game, text, &center, &textBoxInitInfo, &textureID);

	auto onSelectedFunc = [postStatusPopUpCallback](Game &game)
	{
		GameWorldUiController::onStatusPopUpSelected(game);
		postStatusPopUpCallback();
	};

	ScopedUiTextureRef textureRef(textureID, game.renderer);
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, onSelectedFunc, std::move(textureRef), center);
}

void GameWorldUiController::onDoorUnlockedWithKey(Game &game, int keyID, const std::string &soundFilename, const WorldDouble3 &soundPosition, const ExeData &exeData)
{
	const std::string text = GameWorldUiModel::getDoorUnlockWithKeyMessage(keyID, exeData);

	Int2 center;
	TextBoxInitInfo textBoxInitInfo;
	UiTextureID textureID;
	GetDefaultStatusPopUpInitValues(game, text, &center, &textBoxInitInfo, &textureID);

	auto onCloseCallback = [soundFilename, soundPosition](Game &game)
	{
		GameWorldUiController::onStatusPopUpSelected(game);

		AudioManager &audioManager = game.audioManager;
		audioManager.playSound(soundFilename.c_str(), soundPosition);
	};

	ScopedUiTextureRef textureRef(textureID, game.renderer);
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, onCloseCallback, std::move(textureRef), center);
}

void GameWorldUiController::onCitizenInteracted(Game &game, const EntityInstance &entityInst)
{
	const EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
	const EntityCitizenName &citizenName = entityChunkManager.getEntityCitizenName(entityInst.citizenNameID);
	const std::string citizenNameStr(citizenName.name);
	const std::string text = citizenNameStr + "\n(dialogue not implemented)";

	Int2 center;
	TextBoxInitInfo textBoxInitInfo;
	UiTextureID textureID;
	GetDefaultStatusPopUpInitValues(game, text, &center, &textBoxInitInfo, &textureID);

	ScopedUiTextureRef textureRef(textureID, game.renderer);
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, GameWorldUiController::onStatusPopUpSelected, std::move(textureRef), center);
}

void GameWorldUiController::onCitizenKilled(Game &game)
{
	// Randomly give player some gold.
	Random &random = game.random;
	const int citizenCorpseGold = ArenaCitizenUtils::DEATH_MIN_GOLD_PIECES + random.next(ArenaCitizenUtils::DEATH_MAX_GOLD_PIECES);

	Player &player = game.player;
	player.gold += citizenCorpseGold;

	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const ExeData &exeData = binaryAssetLibrary.getExeData();
	const std::string text = GameWorldUiModel::getCitizenKillGoldMessage(citizenCorpseGold, exeData);

	TextBox &actionTextBox = *game.getActionTextBox();
	actionTextBox.setText(text);
	game.gameState.setActionTextDuration(text);
}

void GameWorldUiController::onStaticNpcInteracted(Game &game, StaticNpcPersonalityType personalityType)
{
	constexpr const char *personalityTypeNames[] =
	{
		"Shopkeeper",
		"Beggar",
		"Firebreather",
		"Prostitute",
		"Jester",
		"Street Vendor",
		"Musician",
		"Priest",
		"Thief",
		"Snake Charmer",
		"Street Vendor Alchemist",
		"Wizard",
		"Tavern Patron"
	};

	std::string text;
	TextAlignment textAlignment = TextAlignment::MiddleCenter;

	if (personalityType == StaticNpcPersonalityType::TavernPatron)
	{
		const TextAssetLibrary &textAssetLibrary = TextAssetLibrary::getInstance();
		const ArenaTemplateDatEntry &patronDialoguesEntry = textAssetLibrary.templateDat.getEntry(1430);

		Random &random = game.random;
		const int patronDialoguesRandomIndex = random.next(patronDialoguesEntry.values.size());
		text = patronDialoguesEntry.values[patronDialoguesRandomIndex];

		const Player &player = game.player;
		const CharacterRaceLibrary &charRaceLibrary = CharacterRaceLibrary::getInstance();
		const CharacterRaceDefinition &charRaceDef = charRaceLibrary.getDefinition(player.raceID);

		const GameState &gameState = game.gameState;
		const LocationDefinition &locationDef = gameState.getLocationDefinition();
		const std::string &locationName = locationDef.getName();

		const ProvinceDefinition &provinceDef = gameState.getProvinceDefinition();
		const std::string &provinceName = provinceDef.getName();
		const int provinceID = provinceDef.getRaceID();

		const int oathsProvinceID = (provinceID != ArenaLocationUtils::CENTER_PROVINCE_ID) ? provinceID : random.next(ArenaLocationUtils::CENTER_PROVINCE_ID);
		const int oathsID = 364 + oathsProvinceID;
		const ArenaTemplateDatEntry &oathsEntry = textAssetLibrary.templateDat.getEntry(oathsID);
		const int oathsRandomIndex = random.next(oathsEntry.values.size());
		const std::string &oathString = oathsEntry.values[oathsRandomIndex];

		std::string interiorDisplayName;
		const MapDefinition &mapDef = gameState.getActiveMapDef();
		if (mapDef.getMapType() == MapType::Interior)
		{
			const MapDefinitionInterior &mapDefInterior = mapDef.getSubDefinition().interior;
			interiorDisplayName = mapDefInterior.displayName;
		}

		// @todo move this into a global dialogue processor, see "Dialog" wiki
		text = String::replace(text, "%ra", charRaceDef.singularName);
		text = String::replace(text, "%cn", locationName);
		text = String::replace(text, "%lp", provinceName);
		text = String::replace(text, "%oth", oathString);
		text = String::replace(text, "%nt", interiorDisplayName);
		text = String::distributeNewlines(text, 65);

		textAlignment = TextAlignment::TopLeft;
	}
	else
	{
		const int personalityTypeIndex = static_cast<int>(personalityType);
		text = std::string(personalityTypeNames[personalityTypeIndex]) + "\n(dialogue not implemented)";
	}

	const Int2 center = GameWorldUiView::getStatusPopUpTextCenterPoint(game);
	const TextBoxInitInfo textBoxInitInfo = TextBoxInitInfo::makeWithCenter(
		text,
		center,
		GameWorldUiView::StatusPopUpFontName,
		GameWorldUiView::StatusPopUpTextColor,
		textAlignment,
		std::nullopt,
		GameWorldUiView::StatusPopUpTextLineSpacing,
		FontLibrary::getInstance());

	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	Surface surface = TextureUtils::generate(
		GameWorldUiView::StatusPopUpTexturePatternType,
		GameWorldUiView::getStatusPopUpTextureWidth(textBoxInitInfo.rect.width),
		GameWorldUiView::getStatusPopUpTextureHeight(textBoxInitInfo.rect.height),
		textureManager,
		renderer);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTextureFromSurface(surface, textureManager, renderer, &textureID))
	{
		DebugLogError("Couldn't create pop-up texture for static NPC conversation.");
		return;
	}

	ScopedUiTextureRef textureRef(textureID, game.renderer);
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, GameWorldUiController::onStatusPopUpSelected, std::move(textureRef), center);
}

void GameWorldUiController::onShowPlayerDeathCinematic(Game &game)
{
	// Death cinematic then main menu.
	const CinematicLibrary &cinematicLibrary = CinematicLibrary::getInstance();

	int textCinematicDefIndex;
	const TextCinematicDefinition *defPtr = nullptr;
	const bool success = cinematicLibrary.findTextDefinitionIndexIf(
		[&defPtr](const TextCinematicDefinition &def)
	{
		if (def.type == TextCinematicDefinitionType::Death)
		{
			const DeathTextCinematicDefinition &deathTextCinematicDef = def.death;
			if (deathTextCinematicDef.type == DeathTextCinematicType::Good)
			{
				defPtr = &def;
				return true;
			}
		}

		return false;
	}, &textCinematicDefIndex);

	if (!success)
	{
		DebugCrash("Couldn't find death text cinematic definition.");
	}

	TextureManager &textureManager = game.textureManager;
	const std::string &cinematicFilename = defPtr->animFilename;
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(cinematicFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugLogError("Couldn't get texture file metadata for death cinematic \"" + cinematicFilename + "\".");
		return;
	}

	const TextureFileMetadata &metadata = textureManager.getMetadataHandle(*metadataID);
	const double secondsPerFrame = metadata.getSecondsPerFrame();
	game.setPanel<TextCinematicPanel>(textCinematicDefIndex, secondsPerFrame, PauseMenuUiController::onNewGameButtonSelected);

	const MusicDefinition *musicDef = MusicUtils::getMainQuestCinematicGoodMusicDefinition(game.random);
	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing death cinematic music.");
	}

	AudioManager &audioManager = game.audioManager;
	audioManager.setMusic(musicDef);
}

void GameWorldUiController::onHealthDepleted(Game &game)
{
	GameWorldUiController::onShowPlayerDeathCinematic(game);
}

void GameWorldUiController::onStaminaExhausted(Game &game, bool isSwimming, bool isInterior, bool isNight)
{
	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const ExeData &exeData = binaryAssetLibrary.getExeData();
	const std::string text = GameWorldUiModel::getStaminaExhaustedMessage(isSwimming, isInterior, isNight, exeData);

	Int2 center;
	TextBoxInitInfo textBoxInitInfo;
	UiTextureID textureID;
	GetDefaultStatusPopUpInitValues(game, text, &center, &textBoxInitInfo, &textureID);

	auto onCloseCallback = [isSwimming, isInterior, isNight, &exeData](Game &game)
	{
		GameWorldUiController::onStatusPopUpSelected(game);

		const bool isPlayerDying = isSwimming || (!isInterior && isNight);
		if (isPlayerDying)
		{
			GameWorldUiController::onShowPlayerDeathCinematic(game);
		}
		else
		{
			// Rest for a while.
			Player &player = game.player;
			constexpr int restFactor = 1;
			constexpr int tavernRoomType = 1; // Hardcoded when exhausted
			player.applyRestHealing(restFactor, tavernRoomType, exeData);

			constexpr double secondsPerHour = 60.0 * 60.0;
			constexpr double realSecondsPerInGameHour = secondsPerHour / GameState::GAME_TIME_SCALE;
			game.gameState.tickGameClock(realSecondsPerInGameHour, game);
		}
	};

	ScopedUiTextureRef textureRef(textureID, game.renderer);
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, onCloseCallback, std::move(textureRef), center);
}
