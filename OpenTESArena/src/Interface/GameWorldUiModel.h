#ifndef GAME_WORLD_UI_MODEL_H
#define GAME_WORLD_UI_MODEL_H

#include <optional>
#include <string>

#include "../Math/MathUtils.h"
#include "../World/Coord.h"

#include "components/utilities/Span.h"

class Game;

enum class MapType;

struct ExeData;
struct Rect;

namespace GameWorldUiModel
{
	enum class ButtonType
	{
		CharacterSheet,
		ToggleWeapon,
		Map,
		Steal,
		Status,
		Magic,
		Logbook,
		UseItem,
		Camp
	};

	constexpr int BUTTON_COUNT = 9;

	std::string getPlayerNameText(Game &game);
	std::string getStatusButtonText(Game &game);

	OriginalInt2 getOriginalPlayerPosition(const WorldDouble3 &playerPos, MapType mapType);
	OriginalInt2 getOriginalPlayerPositionArenaUnits(const WorldDouble3 &playerPos, MapType mapType);
	std::string getPlayerPositionText(Game &game);

	std::optional<ButtonType> getHoveredButtonType(Game &game);
	bool isButtonTooltipAllowed(ButtonType buttonType, Game &game);
	std::string getButtonTooltip(ButtonType buttonType);

	void setFreeLookActive(Game &game, bool active);

	// Converts a screen point to a 3D direction in the world.
	VoxelDouble3 screenToWorldRayDirection(Game &game, const Int2 &windowPoint);

	Radians getCompassAngle(const VoxelDouble2 &direction);

	// Modifies the values in the native cursor regions array so rectangles in
	// the current window correctly represent regions for different arrow cursors.
	void updateNativeCursorRegions(Span<Rect> nativeCursorRegions, int width, int height);

	std::string getEnemyInspectedMessage(const std::string &entityName, const ExeData &exeData);
	std::string getEnemyCorpseGoldMessage(int goldCount, const ExeData &exeData);
	std::string getEnemyCorpseEmptyInventoryMessage(const std::string &entityName, const ExeData &exeData);
	std::string getCitizenKillGoldMessage(int goldCount, const ExeData &exeData);

	std::string getLockDifficultyMessage(int lockLevel, const ExeData &exeData);
	std::string getKeyPickUpMessage(int keyID, const ExeData &exeData);
	std::string getDoorUnlockWithKeyMessage(int keyID, const ExeData &exeData);

	std::string getStaminaExhaustedMessage(bool isSwimming, bool isInterior, bool isNight, const ExeData &exeData);
}

#endif
