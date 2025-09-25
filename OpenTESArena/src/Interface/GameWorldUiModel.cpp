#include <cmath>

#include "GameWorldUiModel.h"
#include "GameWorldUiView.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/RMDFile.h"
#include "../Game/Game.h"
#include "../Player/ArenaPlayerUtils.h"
#include "../Player/Player.h"
#include "../Rendering/RenderCamera.h"
#include "../Rendering/RendererUtils.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Time/ArenaClockUtils.h"
#include "../Time/ArenaDateUtils.h"
#include "../Time/ClockLibrary.h"
#include "../World/MapType.h"

#include "components/utilities/String.h"

namespace
{
	// Original game has eight time ranges for each time of day. They aren't uniformly distributed -- midnight and noon are only one minute.
	constexpr std::pair<const char*, int> TimeOfDayIndices[] =
	{
		std::make_pair(ArenaClockUtils::Midnight, 6),
		std::make_pair(ArenaClockUtils::Night1, 5),
		std::make_pair(ArenaClockUtils::EarlyMorning, 0),
		std::make_pair(ArenaClockUtils::Morning, 1),
		std::make_pair(ArenaClockUtils::Noon, 2),
		std::make_pair(ArenaClockUtils::Afternoon, 3),
		std::make_pair(ArenaClockUtils::Evening, 4),
		std::make_pair(ArenaClockUtils::Night2, 5)
	};

	std::string GetStatusTimeString(const Clock &clock, const ExeData &exeData)
	{
		const int hours12 = clock.getHours12();
		const int minutes = clock.minutes;
		const std::string clockTimeString = std::to_string(hours12) + ":" + ((minutes < 10) ? "0" : "") + std::to_string(minutes);

		// Reverse iterate, checking which range the active clock is in.
		const auto pairIter = std::find_if(std::rbegin(TimeOfDayIndices), std::rend(TimeOfDayIndices),
			[&clock](const std::pair<const char*, int> &pair)
		{
			const ClockLibrary &clockLibrary = ClockLibrary::getInstance();
			const Clock &currentClock = clockLibrary.getClock(pair.first);
			return clock.getTotalSeconds() >= currentClock.getTotalSeconds();
		});

		DebugAssertMsg(pairIter != std::rend(TimeOfDayIndices), "No valid time of day.");
		const int timeOfDayIndex = pairIter->second;

		DebugAssertIndex(exeData.calendar.timesOfDay, timeOfDayIndex);
		const std::string &timeOfDayString = exeData.calendar.timesOfDay[timeOfDayIndex];
		return clockTimeString + ' ' + timeOfDayString;
	}

	// Healthy/diseased/etc.
	std::string GetStatusEffectString(const ExeData &exeData)
	{
		std::string text = exeData.status.effect;
		text = String::replace(text, '\r', '\n');

		// Replace %s with placeholder.
		const std::string &effectStr = exeData.status.effectsList[0];
		size_t index = text.find("%s");
		text.replace(index, 2, effectStr);

		// Remove newline on end.
		text.pop_back();

		return text;
	}
}

std::string GameWorldUiModel::getPlayerNameText(Game &game)
{
	return game.player.firstName;
}

std::string GameWorldUiModel::getStatusButtonText(Game &game)
{
	auto &gameState = game.gameState;
	const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const auto &exeData = binaryAssetLibrary.getExeData();
	const LocationDefinition &locationDef = gameState.getLocationDefinition();
	const LocationInstance &locationInst = gameState.getLocationInstance();
	const std::string &locationName = locationInst.getName(locationDef);
	const Player &player = game.player;
	const std::string timeString = GetStatusTimeString(gameState.getClock(), exeData);

	std::string baseText = exeData.status.popUp;
	baseText = String::replace(baseText, '\r', '\n');

	size_t index = baseText.find("%s");
	baseText.replace(index, 2, locationName);

	index = baseText.find("%s", index);
	baseText.replace(index, 2, timeString);

	std::string dateString = ArenaDateUtils::makeDateString(gameState.getDate(), exeData);
	dateString.back() = '\n'; // Replace \r with \n.

	index = baseText.find("%s", index);
	baseText.replace(index, 2, dateString);

	const int currentWeight = static_cast<int>(player.inventory.getWeight());
	index = baseText.find("%d", index);
	baseText.replace(index, 2, std::to_string(currentWeight));

	const PrimaryAttributes &primaryAttributes = player.primaryAttributes;
	const int weightCapacity = ArenaPlayerUtils::calculateMaxWeight(primaryAttributes.strength.maxValue); // @todo use current value instead
	index = baseText.find("%d", index);
	baseText.replace(index, 2, std::to_string(weightCapacity));

	const std::string effectText = GetStatusEffectString(exeData);

	return baseText + effectText;
}

OriginalInt2 GameWorldUiModel::getOriginalPlayerPosition(const WorldDouble3 &playerPos, MapType mapType)
{
	const WorldInt2 absolutePlayerVoxelXZ = VoxelUtils::pointToVoxel(playerPos.getXZ());
	const OriginalInt2 originalVoxel = VoxelUtils::worldVoxelToOriginalVoxel(absolutePlayerVoxelXZ);

	// The displayed coordinates in the wilderness behave differently in the original
	// game due to how the 128x128 grid shifts to keep the player roughly centered.
	if (mapType != MapType::Wilderness)
	{
		return originalVoxel;
	}
	else
	{
		const int halfWidth = RMDFile::WIDTH / 2;
		const int halfDepth = RMDFile::DEPTH / 2;
		return OriginalInt2(
			halfWidth + ((originalVoxel.x + halfWidth) % RMDFile::WIDTH),
			halfDepth + ((originalVoxel.y + halfDepth) % RMDFile::DEPTH));
	}
}

std::string GameWorldUiModel::getPlayerPositionText(Game &game)
{	
	const Player &player = game.player;
	const GameState &gameState = game.gameState;
	const OriginalInt2 displayedCoords = GameWorldUiModel::getOriginalPlayerPosition(player.getEyePosition(), gameState.getActiveMapType());

	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	std::string str = exeData.ui.currentWorldPosition;

	// Replace first %d with X, second %d with Y.
	size_t index = str.find("%d");
	str.replace(index, 2, std::to_string(displayedCoords.x));

	index = str.find("%d", index);
	str.replace(index, 2, std::to_string(displayedCoords.y));

	return str;
}

std::optional<GameWorldUiModel::ButtonType> GameWorldUiModel::getHoveredButtonType(Game &game)
{
	const auto &options = game.options;
	const bool modernInterface = options.getGraphics_ModernInterface();
	if (modernInterface)
	{
		return std::nullopt;
	}

	const Window &window = game.window;
	const auto &inputManager = game.inputManager;
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 classicPosition = window.nativeToOriginal(mousePosition);
	for (int i = 0; i < GameWorldUiModel::BUTTON_COUNT; i++)
	{
		const ButtonType buttonType = static_cast<ButtonType>(i);
		const Rect buttonRect = GameWorldUiView::getButtonRect(buttonType);
		if (buttonRect.contains(classicPosition))
		{
			return buttonType;
		}
	}

	return std::nullopt;
}

bool GameWorldUiModel::isButtonTooltipAllowed(ButtonType buttonType, Game &game)
{
	if (buttonType == ButtonType::Magic)
	{
		const Player &player = game.player;
		const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
		const int charClassDefID = player.charClassDefID;
		const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
		return charClassDef.castsMagic;
	}
	else
	{
		return true;
	}
}

std::string GameWorldUiModel::getButtonTooltip(ButtonType buttonType)
{
	switch (buttonType)
	{
	case ButtonType::CharacterSheet:
		return "Character Sheet";
	case ButtonType::ToggleWeapon:
		return "Draw/Sheathe Weapon";
	case ButtonType::Map:
		return "Automap/World Map";
	case ButtonType::Steal:
		return "Steal";
	case ButtonType::Status:
		return "Status";
	case ButtonType::Magic:
		return "Spells";
	case ButtonType::Logbook:
		return "Logbook";
	case ButtonType::UseItem:
		return "Use Item";
	case ButtonType::Camp:
		return "Camp";
	default:
		DebugUnhandledReturnMsg(std::string, std::to_string(static_cast<int>(buttonType)));
	}
}

void GameWorldUiModel::setFreeLookActive(Game &game, bool active)
{
	// Set relative mouse mode. When enabled, this freezes the hardware cursor in place but relative motion
	// events are still recorded.
	InputManager &inputManager = game.inputManager;
	inputManager.setRelativeMouseMode(active);

	Window &window = game.window;
	const Int2 logicalDims = window.getLogicalDimensions();
	window.warpMouse(logicalDims.x / 2, logicalDims.y / 2);
}

VoxelDouble3 GameWorldUiModel::screenToWorldRayDirection(Game &game, const Int2 &windowPoint)
{
	const Options &options = game.options;
	const Window &window = game.window;
	const Player &player = game.player;
	const WorldDouble3 playerPosition = player.getEyePosition();
	const double tallPixelRatio = RendererUtils::getTallPixelRatio(options.getGraphics_TallPixelCorrection());

	RenderCamera renderCamera;
	renderCamera.init(playerPosition, player.angleX, player.angleY, options.getGraphics_VerticalFOV(), window.getSceneViewAspectRatio(), tallPixelRatio);

	// Mouse position percents across the screen. Add 0.50 to sample at the center of the pixel.
	const Int2 viewDims = window.getSceneViewDimensions();
	const double screenXPercent = (static_cast<double>(windowPoint.x) + 0.50) / static_cast<double>(viewDims.x);
	const double screenYPercent = (static_cast<double>(windowPoint.y) + 0.50) / static_cast<double>(viewDims.y);

	return renderCamera.screenToWorld(screenXPercent, screenYPercent);
}

Radians GameWorldUiModel::getCompassAngle(const VoxelDouble2 &direction)
{
	return std::atan2(-direction.y, -direction.x);
}

void GameWorldUiModel::updateNativeCursorRegions(Span<Rect> nativeCursorRegions, int width, int height)
{
	// @todo: maybe the classic rects should be converted to vector space then scaled by the ratio of aspect ratios?
	const double xScale = static_cast<double>(width) / ArenaRenderUtils::SCREEN_WIDTH_REAL;
	const double yScale = static_cast<double>(height) / ArenaRenderUtils::SCREEN_HEIGHT_REAL;

	for (int i = 0; i < nativeCursorRegions.getCount(); i++)
	{
		nativeCursorRegions[i] = GameWorldUiView::scaleClassicCursorRectToNative(i, xScale, yScale);
	}
}

std::string GameWorldUiModel::getEnemyInspectedMessage(const std::string &entityName, const ExeData &exeData)
{
	std::string text = exeData.ui.inspectedEntityName;
	text.replace(text.find("%s"), 2, entityName);
	return text;
}

std::string GameWorldUiModel::getEnemyCorpseGoldMessage(int goldCount, const ExeData &exeData)
{
	std::string text = exeData.status.enemyCorpseGold;
	text.replace(text.find("%u"), 2, std::to_string(goldCount));
	return text;
}

std::string GameWorldUiModel::getEnemyCorpseEmptyInventoryMessage(const std::string &entityName, const ExeData &exeData)
{
	std::string text = exeData.status.enemyCorpseEmptyInventory;
	text.replace(text.find("%s"), 2, entityName);
	return text;
}

std::string GameWorldUiModel::getCitizenKillGoldMessage(int goldCount, const ExeData &exeData)
{
	std::string text = exeData.status.citizenCorpseGold;
	text.replace(text.find("%d"), 2, std::to_string(goldCount));
	return text;
}

std::string GameWorldUiModel::getLockDifficultyMessage(int lockLevel, const ExeData &exeData)
{
	DebugAssertIndex(exeData.status.lockDifficultyMessages, lockLevel);
	return exeData.status.lockDifficultyMessages[lockLevel];
}

std::string GameWorldUiModel::getKeyPickUpMessage(int keyID, const ExeData &exeData)
{
	DebugAssertIndex(exeData.status.keyNames, keyID);
	const std::string &keyName = exeData.status.keyNames[keyID];

	std::string keyPickupMessage = exeData.status.keyPickedUp;
	size_t replaceIndex = keyPickupMessage.find("%s");
	keyPickupMessage.replace(replaceIndex, 2, keyName);
	return keyPickupMessage;
}

std::string GameWorldUiModel::getDoorUnlockWithKeyMessage(int keyID, const ExeData &exeData)
{
	DebugAssertIndex(exeData.status.keyNames, keyID);
	const std::string &keyName = exeData.status.keyNames[keyID];

	std::string doorUnlockMessage = exeData.status.doorUnlockedWithKey;
	size_t replaceIndex = doorUnlockMessage.find("%s");
	doorUnlockMessage.replace(replaceIndex, 2, keyName);
	return doorUnlockMessage;
}

std::string GameWorldUiModel::getStaminaExhaustedMessage(bool isSwimming, bool isInterior, bool isNight, const ExeData &exeData)
{
	std::string text;

	if (isSwimming)
	{
		text = exeData.status.staminaDrowning;
	}
	else if (!isInterior && isNight)
	{
		text = exeData.status.staminaExhaustedDeath;
	}
	else
	{
		text = exeData.status.staminaExhaustedRecover;
	}

	text = String::replace(text, '\r', '\n');
	text.erase(text.find_last_of('\n'));

	return text;
}
