#include <cmath>

#include "GameWorldUiModel.h"
#include "GameWorldUiView.h"
#include "../Entities/Player.h"
#include "../Game/ArenaClockUtils.h"
#include "../Game/ArenaDateUtils.h"
#include "../Game/Game.h"
#include "../World/MapType.h"

#include "components/utilities/String.h"

std::string GameWorldUiModel::getPlayerNameText(Game &game)
{
	auto &gameState = game.getGameState();
	const auto &player = gameState.getPlayer();
	return player.getFirstName();
}

std::string GameWorldUiModel::getStatusButtonText(Game &game)
{
	auto &gameState = game.getGameState();
	const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
	const auto &exeData = binaryAssetLibrary.getExeData();
	const LocationDefinition &locationDef = gameState.getLocationDefinition();
	const LocationInstance &locationInst = gameState.getLocationInstance();
	const std::string &locationName = locationInst.getName(locationDef);

	const std::string timeString = [&game, &gameState, &exeData]()
	{
		const Clock &clock = gameState.getClock();
		const int hours = clock.getHours12();
		const int minutes = clock.getMinutes();
		const std::string clockTimeString = std::to_string(hours) + ":" +
			((minutes < 10) ? "0" : "") + std::to_string(minutes);

		const int timeOfDayIndex = [&gameState]()
		{
			// Arena has eight time ranges for each time of day. They aren't
			// uniformly distributed -- midnight and noon are only one minute.
			const std::array<std::pair<Clock, int>, 8> clocksAndIndices =
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

			const Clock &presentClock = gameState.getClock();

			// Reverse iterate, checking which range the active clock is in.
			const auto pairIter = std::find_if(
				clocksAndIndices.rbegin(), clocksAndIndices.rend(),
				[&presentClock](const std::pair<Clock, int> &pair)
			{
				const Clock &clock = pair.first;
				return presentClock.getTotalSeconds() >= clock.getTotalSeconds();
			});

			DebugAssertMsg(pairIter != clocksAndIndices.rend(), "No valid time of day.");
			return pairIter->second;
		}();

		const std::string &timeOfDayString =
			exeData.calendar.timesOfDay.at(timeOfDayIndex);

		return clockTimeString + ' ' + timeOfDayString;
	}();

	// Get the base status text.
	std::string baseText = exeData.status.popUp;

	// Replace carriage returns with newlines.
	baseText = String::replace(baseText, '\r', '\n');

	// Replace first %s with location name.
	size_t index = baseText.find("%s");
	baseText.replace(index, 2, locationName);

	// Replace second %s with time string.
	index = baseText.find("%s", index);
	baseText.replace(index, 2, timeString);

	// Replace third %s with date string, filled in with each value.
	std::string dateString = ArenaDateUtils::makeDateString(gameState.getDate(), exeData);
	dateString.back() = '\n'; // Replace \r with \n.

	index = baseText.find("%s", index);
	baseText.replace(index, 2, dateString);

	// Replace %d's with current and total weight.
	const int currentWeight = 0;
	index = baseText.find("%d", index);
	baseText.replace(index, 2, std::to_string(currentWeight));

	const int weightCapacity = 0;
	index = baseText.find("%d", index);
	baseText.replace(index, 2, std::to_string(weightCapacity));

	// Append the list of effects at the bottom (healthy/diseased...).
	const std::string effectText = [&exeData]()
	{
		std::string text = exeData.status.effect;

		// Replace carriage returns with newlines.
		text = String::replace(text, '\r', '\n');

		// Replace %s with placeholder.
		const std::string &effectStr = exeData.status.effectsList.front();
		size_t index = text.find("%s");
		text.replace(index, 2, effectStr);

		// Remove newline on end.
		text.pop_back();

		return text;
	}();

	return baseText + effectText;
}

std::string GameWorldUiModel::getPlayerPositionText(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	GameState &gameState = game.getGameState();
	const MapDefinition &mapDef = gameState.getActiveMapDef();
	const Player &player = gameState.getPlayer();

	const MapType mapType = mapDef.getMapType();
	const OriginalInt2 displayedCoords = [&player, mapType]()
	{
		const NewDouble3 absolutePlayerPosition = VoxelUtils::coordToNewPoint(player.getPosition());
		const NewInt3 absolutePlayerVoxel = VoxelUtils::pointToVoxel(absolutePlayerPosition);
		const NewInt2 playerVoxelXZ(absolutePlayerVoxel.x, absolutePlayerVoxel.z);
		const OriginalInt2 originalVoxel = VoxelUtils::newVoxelToOriginalVoxel(playerVoxelXZ);

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
	}();

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
	const auto &options = game.getOptions();
	const bool modernInterface = options.getGraphics_ModernInterface();
	if (modernInterface)
	{
		return std::nullopt;
	}

	const auto &renderer = game.getRenderer();
	const auto &inputManager = game.getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 classicPosition = renderer.nativeToOriginal(mousePosition);
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
		auto &gameState = game.getGameState();
		const Player &player = gameState.getPlayer();
		const CharacterClassLibrary &charClassLibrary = game.getCharacterClassLibrary();
		const int charClassDefID = player.getCharacterClassDefID();
		const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
		return charClassDef.canCastMagic();
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
	auto &inputManager = game.getInputManager();
	inputManager.setRelativeMouseMode(active);

	auto &renderer = game.getRenderer();
	const Int2 windowDims = renderer.getWindowDimensions();
	renderer.warpMouse(windowDims.x / 2, windowDims.y / 2);
}

VoxelDouble3 GameWorldUiModel::screenToWorldRayDirection(Game &game, const Int2 &windowPoint)
{
	const auto &options = game.getOptions();
	const auto &renderer = game.getRenderer();
	const Int2 viewDims = renderer.getViewDimensions();
	const double viewAspectRatio = static_cast<double>(viewDims.x) / static_cast<double>(viewDims.y);

	auto &gameState = game.getGameState();
	const auto &player = gameState.getPlayer();
	const Double3 &cameraDirection = player.getDirection();

	// Mouse position percents across the screen. Add 0.50 to sample at the center
	// of the pixel.
	const double mouseXPercent = (static_cast<double>(windowPoint.x) + 0.50) / static_cast<double>(viewDims.x);
	const double mouseYPercent = (static_cast<double>(windowPoint.y) + 0.50) / static_cast<double>(viewDims.y);

	return renderer.screenPointToRay(mouseXPercent, mouseYPercent, cameraDirection,
		options.getGraphics_VerticalFOV(), viewAspectRatio);
}

Radians GameWorldUiModel::getCompassAngle(const VoxelDouble2 &direction)
{
	return std::atan2(-direction.y, -direction.x);
}

void GameWorldUiModel::updateNativeCursorRegions(BufferView<Rect> nativeCursorRegions, int width, int height)
{
	// @todo: maybe the classic rects should be converted to vector space then scaled by the ratio of aspect ratios?
	const double xScale = static_cast<double>(width) / ArenaRenderUtils::SCREEN_WIDTH_REAL;
	const double yScale = static_cast<double>(height) / ArenaRenderUtils::SCREEN_HEIGHT_REAL;

	for (int i = 0; i < nativeCursorRegions.getCount(); i++)
	{
		nativeCursorRegions.set(i, GameWorldUiView::scaleClassicCursorRectToNative(i, xScale, yScale));
	}
}
