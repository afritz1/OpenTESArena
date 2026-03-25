#include "CinematicLibrary.h"
#include "GameWorldUiController.h"
#include "GameWorldUiModel.h"
#include "GameWorldUiState.h"
#include "PauseMenuUiController.h"
#include "TextCinematicUiState.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Audio/MusicUtils.h"
#include "../Entities/ArenaCitizenUtils.h"
#include "../Entities/ArenaEntityUtils.h"
#include "../Game/Game.h"
#include "../Player/Player.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Stats/CharacterRaceLibrary.h"
#include "../Time/ArenaClockUtils.h"
#include "../Time/ArenaDateUtils.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../World/MapType.h"
#include "../WorldMap/ArenaLocationUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

void GameWorldUiController::onStatusPopUpSelected(Game &game)
{
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
}

void GameWorldUiController::onEnemyAliveInspected(Game &game, EntityInstanceID entityInstID, const EntityDefinition &entityDef)
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
	GameWorldUI::setActionText(text.c_str());
}

void GameWorldUiController::onContainerInventoryOpened(Game &game, EntityInstanceID entityInstID, ItemInventory &itemInventory, bool destroyEntityIfEmpty)
{
	// @todo: need to queue entity destroy if container is empty
	// @todo: if closing and container is not empty, then inventory.compact(). Don't compact while removing items since that would invalidate mappings

	auto callback = [&game, entityInstID, &itemInventory, destroyEntityIfEmpty]()
	{
		if (destroyEntityIfEmpty && (itemInventory.getOccupiedSlotCount() == 0))
		{
			EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
			entityChunkManager.queueEntityDestroy(entityInstID, true);
		}

		GameWorldUiController::onStatusPopUpSelected(game);
	};

	GameWorldUI::showLootPopUp(itemInventory, callback);
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

	auto callback = [&game, entityInstID, entityDef]()
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

	GameWorldUI::showTextPopUp(text.c_str(), callback);
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
	GameWorldUI::showTextPopUp(text.c_str());
}

void GameWorldUiController::onKeyPickedUp(Game &game, int keyID, const ExeData &exeData, const std::function<void()> &postStatusPopUpCallback)
{
	const std::string text = GameWorldUiModel::getKeyPickUpMessage(keyID, exeData);
	auto callback = [&game, postStatusPopUpCallback]()
	{
		GameWorldUiController::onStatusPopUpSelected(game);
		postStatusPopUpCallback();
	};

	GameWorldUI::showTextPopUp(text.c_str(), callback);
}

void GameWorldUiController::onDoorUnlockedWithKey(Game &game, int keyID, const std::string &soundFilename, const WorldDouble3 &soundPosition, const ExeData &exeData)
{
	const std::string text = GameWorldUiModel::getDoorUnlockWithKeyMessage(keyID, exeData);
	auto callback = [&game, soundFilename, soundPosition]()
	{
		GameWorldUiController::onStatusPopUpSelected(game);

		AudioManager &audioManager = game.audioManager;
		audioManager.playSound(soundFilename.c_str(), soundPosition);
	};

	GameWorldUI::showTextPopUp(text.c_str(), callback);
}

void GameWorldUiController::onCitizenInteracted(Game &game, const EntityInstance &entityInst)
{
	const EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
	const EntityCitizenName &citizenName = entityChunkManager.getEntityCitizenName(entityInst.citizenNameID);
	const std::string citizenNameStr(citizenName.name);
	const std::string text = citizenNameStr + "\n(dialogue not implemented)";
	GameWorldUI::showTextPopUp(text.c_str());
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
	GameWorldUI::setActionText(text.c_str());
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

	// @todo use textAlignment?
	GameWorldUI::showTextPopUp(text.c_str());
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

	TextCinematicUiInitInfo &textCinematicInitInfo = TextCinematicUI::state.initInfo;
	textCinematicInitInfo.init(textCinematicDefIndex, metadata.getSecondsPerFrame(), [&game]() { PauseMenuUiController::onNewGameButtonSelected(game); });
	game.setNextContext(TextCinematicUI::ContextName);

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

	auto callback = [&game, isSwimming, isInterior, isNight, &exeData]()
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

	GameWorldUI::showTextPopUp(text.c_str(), callback);
}
