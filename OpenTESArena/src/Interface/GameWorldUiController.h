#ifndef GAME_WORLD_UI_CONTROLLER_H
#define GAME_WORLD_UI_CONTROLLER_H

#include <functional>

#include "../Entities/EntityInstance.h"
#include "../Math/Vector2.h"

class Game;
class GameWorldPanel;
class ItemInventory;

enum class MouseButtonType;

struct EntityInstance;
struct ExeData;
struct Player;
struct Rect;

namespace GameWorldUiController
{
	void onStatusPopUpSelected(Game &game);

	void onEnemyAliveInspected(Game &game, EntityInstanceID entityInstID, const EntityDefinition &entityDef);
	void onContainerInventoryOpened(Game &game, EntityInstanceID entityInstID, ItemInventory &itemInventory, bool destroyEntityIfEmpty);
	void onEnemyCorpseInteracted(Game &game, EntityInstanceID entityInstID, const EntityDefinition &entityDef);
	void onEnemyCorpseInteractedFirstTime(Game &game, EntityInstanceID entityInstID, const EntityDefinition &entityDef);
	void onEnemyCorpseEmptyInventoryOpened(Game &game, EntityInstanceID entityInstID, const EntityDefinition &entityDef);

	void onKeyPickedUp(Game &game, int keyID, const ExeData &exeData, const std::function<void()> &postStatusPopUpCallback);
	void onDoorUnlockedWithKey(Game &game, int keyID, const std::string &soundFilename, const WorldDouble3 &soundPosition, const ExeData &exeData);

	void onCitizenInteracted(Game &game, const EntityInstance &entityInst);
	void onCitizenKilled(Game &game);

	void onStaticNpcInteracted(Game &game, StaticNpcPersonalityType personalityType);

	void onShowPlayerDeathCinematic(Game &game);
	void onHealthDepleted(Game &game);
	void onStaminaExhausted(Game &game, bool isSwimming, bool isInterior, bool isNight);
}

#endif
