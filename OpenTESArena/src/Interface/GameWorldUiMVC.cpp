#include <algorithm>
#include <cmath>

#include "CinematicLibrary.h"
#include "GameWorldUiMVC.h"
#include "GameWorldUiState.h"
#include "InventoryUiMVC.h"
#include "PauseMenuUiMVC.h"
#include "TextCinematicUiState.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/RMDFile.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Assets/TextureFileMetadata.h"
#include "../Audio/MusicUtils.h"
#include "../Collision/RayCastTypes.h"
#include "../Entities/ArenaCitizenUtils.h"
#include "../Entities/ArenaEntityUtils.h"
#include "../Entities/EntityChunkManager.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Game/Game.h"
#include "../Input/InputManager.h"
#include "../Player/ArenaPlayerUtils.h"
#include "../Player/Player.h"
#include "../Rendering/RenderCamera.h"
#include "../Rendering/RendererUtils.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Stats/CharacterRaceLibrary.h"
#include "../Time/ArenaClockUtils.h"
#include "../Time/ArenaDateUtils.h"
#include "../Time/ClockLibrary.h"
#include "../UI/FontLibrary.h"
#include "../UI/GuiUtils.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/UiDrawCommand.h"
#include "../UI/UiRenderSpace.h"
#include "../World/MapType.h"
#include "../WorldMap/ArenaLocationUtils.h"
#include "../WorldMap/LocationDefinition.h"
#include "../WorldMap/LocationInstance.h"

#include "components/debug/Debug.h"
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
	GameState &gameState = game.gameState;
	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const ExeData &exeData = binaryAssetLibrary.getExeData();
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

OriginalInt2 GameWorldUiModel::getOriginalPlayerPositionArenaUnits(const WorldDouble3 &playerPos, MapType mapType)
{
	const OriginalInt2 originalPosArenaUnits(
		static_cast<WEInt>(playerPos.x * MIFUtils::ARENA_UNITS),
		static_cast<SNInt>(playerPos.z * MIFUtils::ARENA_UNITS));

	// The displayed coordinates in the wilderness behave differently in the original
	// game due to how the 128x128 grid shifts to keep the player roughly centered.
	if (mapType != MapType::Wilderness)
	{
		return originalPosArenaUnits;
	}
	else
	{
		constexpr int ArenaUnitsInteger = static_cast<int>(MIFUtils::ARENA_UNITS);
		constexpr int rmdWidthArenaUnits = RMDFile::WIDTH * ArenaUnitsInteger;
		constexpr int rmdDepthArenaUnits = RMDFile::DEPTH * ArenaUnitsInteger;
		const int halfWidthArenaUnits = rmdWidthArenaUnits / 2;
		const int halfDepthArenaUnits = rmdDepthArenaUnits / 2;
		return OriginalInt2(
			halfWidthArenaUnits + ((originalPosArenaUnits.x + halfWidthArenaUnits) % rmdWidthArenaUnits),
			halfDepthArenaUnits + ((originalPosArenaUnits.y + halfDepthArenaUnits) % rmdDepthArenaUnits));
	}
}

std::string GameWorldUiModel::getPlayerPositionText(Game &game)
{
	const Player &player = game.player;
	const GameState &gameState = game.gameState;
	const OriginalInt2 displayedCoords = GameWorldUiModel::getOriginalPlayerPosition(player.getEyePosition(), gameState.getActiveMapType());

	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	std::string str = exeData.ui.currentWorldPosition;

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

DebugVoxelVisibilityQuadtreeState::DebugVoxelVisibilityQuadtreeState()
{
	std::fill(std::begin(this->textureIDs), std::end(this->textureIDs), -1);
	std::fill(std::begin(this->drawPositionYs), std::end(this->drawPositionYs), 0);
}

void DebugVoxelVisibilityQuadtreeState::populateCommandList(Game &game, UiDrawCommandList &commandList)
{
	const SceneManager &sceneManager = game.sceneManager;
	const Player &player = game.player;
	const CoordDouble3 playerCoord = player.getEyeCoord();
	const VoxelInt2 playerVoxelXZ = VoxelUtils::pointToVoxel(playerCoord.point.getXZ());
	const VoxelFrustumCullingChunkManager &voxelFrustumCullingChunkManager = sceneManager.voxelFrustumCullingChunkManager;
	const VoxelFrustumCullingChunk *playerVoxelFrustumCullingChunk = voxelFrustumCullingChunkManager.findChunkAtPosition(playerCoord.chunk);
	if (playerVoxelFrustumCullingChunk == nullptr)
	{
		return;
	}

	constexpr uint8_t alpha = 192;
	constexpr uint32_t visibleColor = Color(0, 255, 0, alpha).toRGBA();
	constexpr uint32_t partiallyVisibleColor = Color(255, 255, 0, alpha).toRGBA();
	constexpr uint32_t invisibleColor = Color(255, 0, 0, alpha).toRGBA();
	constexpr uint32_t playerColor = Color(255, 255, 255, alpha).toRGBA();

	Renderer &renderer = game.renderer;

	for (int treeLevelIndex = 0; treeLevelIndex < VoxelFrustumCullingChunk::TREE_LEVEL_COUNT; treeLevelIndex++)
	{
		const UiTextureID quadtreeTextureID = this->textureIDs[treeLevelIndex];
		const Int2 quadtreeTextureDims = this->textureDimsList[treeLevelIndex];
		const int quadtreeSideLength = VoxelFrustumCullingChunk::NODES_PER_SIDE[treeLevelIndex];

		LockedTexture lockedTexture = renderer.lockUiTexture(quadtreeTextureID);
		Span2D<uint32_t> quadtreeTexels = lockedTexture.getTexels32();

		for (int y = 0; y < quadtreeTextureDims.y; y++)
		{
			for (int x = 0; x < quadtreeTextureDims.x; x++)
			{
				VisibilityType visibilityType = VisibilityType::Outside;
				const bool isLeaf = treeLevelIndex == VoxelFrustumCullingChunk::TREE_LEVEL_INDEX_LEAF;
				if (isLeaf)
				{
					const int leafNodeIndex = y + (x * quadtreeTextureDims.y);
					DebugAssertIndex(playerVoxelFrustumCullingChunk->leafNodeFrustumTests, leafNodeIndex);
					visibilityType = playerVoxelFrustumCullingChunk->leafNodeFrustumTests[leafNodeIndex] ? VisibilityType::Inside : VisibilityType::Outside;
				}
				else
				{
					const int globalNodeOffset = VoxelFrustumCullingChunk::GLOBAL_NODE_OFFSETS[treeLevelIndex];
					const int internalNodeIndex = globalNodeOffset + (y + (x * quadtreeTextureDims.y));
					DebugAssertIndex(playerVoxelFrustumCullingChunk->internalNodeVisibilityTypes, internalNodeIndex);
					visibilityType = playerVoxelFrustumCullingChunk->internalNodeVisibilityTypes[internalNodeIndex];
				}

				const int dstX = (quadtreeTextureDims.x - 1) - x;
				const int dstY = y;
				uint32_t color = 0;

				const bool inPlayerVoxel = isLeaf && (y == playerVoxelXZ.x) && (x == playerVoxelXZ.y);
				if (inPlayerVoxel)
				{
					color = playerColor;
				}
				else
				{
					switch (visibilityType)
					{
					case VisibilityType::Outside:
						color = invisibleColor;
						break;
					case VisibilityType::Partial:
						color = partiallyVisibleColor;
						break;
					case VisibilityType::Inside:
						color = visibleColor;
						break;
					}
				}

				quadtreeTexels.set(dstX, dstY, color);
			}
		}

		renderer.unlockUiTexture(quadtreeTextureID);

		const Int2 position(ArenaRenderUtils::SCREEN_WIDTH, this->drawPositionYs[treeLevelIndex]);
		const Int2 size = quadtreeTextureDims;
		const Window &window = game.window;
		const Int2 windowDims = window.getPixelDimensions();
		const Rect letterboxRect = window.getLetterboxRect();

		RenderElement2D &renderElement = this->renderElements[treeLevelIndex];
		renderElement.id = this->textureIDs[treeLevelIndex];
		renderElement.rect = GuiUtils::makeWindowSpaceRect(position.x, position.y, size.x, size.y, UiPivotType::TopRight, UiRenderSpace::Classic, windowDims.x, windowDims.y, letterboxRect);
	}

	commandList.addElements(this->renderElements);
}

void DebugVoxelVisibilityQuadtreeState::free(Renderer &renderer)
{
	for (UiTextureID &textureID : this->textureIDs)
	{
		if (textureID >= 0)
		{
			renderer.freeUiTexture(textureID);
			textureID = -1;
		}
	}
}

Rect GameWorldUiView::scaleClassicCursorRectToNative(int rectIndex, double xScale, double yScale)
{
	DebugAssertIndex(GameWorldUiView::CursorRegions, rectIndex);
	const Rect &classicRect = GameWorldUiView::CursorRegions[rectIndex];
	return Rect(
		static_cast<int>(std::ceil(static_cast<double>(classicRect.getLeft()) * xScale)),
		static_cast<int>(std::ceil(static_cast<double>(classicRect.getTop()) * yScale)),
		static_cast<int>(std::ceil(static_cast<double>(classicRect.width) * xScale)),
		static_cast<int>(std::ceil(static_cast<double>(classicRect.height) * yScale)));
}

Rect GameWorldUiView::getCharacterSheetButtonRect()
{
	return Rect(14, 166, 40, 29);
}

Rect GameWorldUiView::getPlayerPortraitRect()
{
	return GameWorldUiView::getCharacterSheetButtonRect();
}

Rect GameWorldUiView::getWeaponSheathButtonRect()
{
	return Rect(88, 151, 29, 22);
}

Rect GameWorldUiView::getMapButtonRect()
{
	return Rect(118, 151, 29, 22);
}

Rect GameWorldUiView::getStealButtonRect()
{
	return Rect(147, 151, 29, 22);
}

Rect GameWorldUiView::getStatusButtonRect()
{
	return Rect(177, 151, 29, 22);
}

Rect GameWorldUiView::getMagicButtonRect()
{
	return Rect(88, 175, 29, 22);
}

Rect GameWorldUiView::getLogbookButtonRect()
{
	return Rect(118, 175, 29, 22);
}

Rect GameWorldUiView::getUseItemButtonRect()
{
	return Rect(147, 175, 29, 22);
}

Rect GameWorldUiView::getCampButtonRect()
{
	return Rect(177, 175, 29, 22);
}

Rect GameWorldUiView::getScrollUpButtonRect()
{
	return Rect(208, ArenaRenderUtils::SCENE_VIEW_HEIGHT + 3, 9, 9);
}

Rect GameWorldUiView::getScrollDownButtonRect()
{
	return Rect(208, ArenaRenderUtils::SCENE_VIEW_HEIGHT + 44, 9, 9);
}

Rect GameWorldUiView::getButtonRect(GameWorldUiModel::ButtonType buttonType)
{
	switch (buttonType)
	{
	case GameWorldUiModel::ButtonType::CharacterSheet:
		return GameWorldUiView::getCharacterSheetButtonRect();
	case GameWorldUiModel::ButtonType::ToggleWeapon:
		return GameWorldUiView::getWeaponSheathButtonRect();
	case GameWorldUiModel::ButtonType::Map:
		return GameWorldUiView::getMapButtonRect();
	case GameWorldUiModel::ButtonType::Steal:
		return GameWorldUiView::getStealButtonRect();
	case GameWorldUiModel::ButtonType::Status:
		return GameWorldUiView::getStatusButtonRect();
	case GameWorldUiModel::ButtonType::Magic:
		return GameWorldUiView::getMagicButtonRect();
	case GameWorldUiModel::ButtonType::Logbook:
		return GameWorldUiView::getLogbookButtonRect();
	case GameWorldUiModel::ButtonType::UseItem:
		return GameWorldUiView::getUseItemButtonRect();
	case GameWorldUiModel::ButtonType::Camp:
		return GameWorldUiView::getCampButtonRect();
	default:
		DebugUnhandledReturnMsg(Rect, std::to_string(static_cast<int>(buttonType)));
	}
}

Int2 GameWorldUiView::getStatusPopUpTextCenterPoint(Game &game)
{
	return GameWorldUiView::getInterfaceCenter(game);
}

int GameWorldUiView::getStatusPopUpTextureWidth(int textWidth)
{
	return textWidth + 12;
}

int GameWorldUiView::getStatusPopUpTextureHeight(int textHeight)
{
	return textHeight + 12;
}

Int2 GameWorldUiView::getGameWorldInterfacePosition()
{
	return Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, ArenaRenderUtils::SCREEN_HEIGHT);
}

Int2 GameWorldUiView::getNoMagicTexturePosition()
{
	return Int2(91, 177);
}

int GameWorldUiView::getKeyTextureCount(TextureManager &textureManager)
{
	const TextureAsset &textureAsset = GameWorldUiView::getKeyTextureAsset(0);
	std::optional<TextureFileMetadataID> textureFileMetadataID = textureManager.tryGetMetadataID(textureAsset.filename.c_str());
	if (!textureFileMetadataID.has_value())
	{
		DebugLogErrorFormat("Couldn't get texture file metadata ID for key textures \"%s\".", textureAsset.filename.c_str());
		return 0;
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*textureFileMetadataID);
	return textureFileMetadata.getTextureCount();
}

Int2 GameWorldUiView::getKeyPosition(int keyIndex)
{
	return Int2(8, 16 + (10 * keyIndex));
}

Int2 GameWorldUiView::getTriggerTextPosition(Game &game, int gameWorldInterfaceTextureHeight)
{
	const auto &options = game.options;
	const bool modernInterface = options.getGraphics_ModernInterface();

	const int textX = ArenaRenderUtils::SCREEN_WIDTH / 2;

	const int interfaceOffsetY = modernInterface ? (gameWorldInterfaceTextureHeight / 2) : gameWorldInterfaceTextureHeight;
	const int textY = ArenaRenderUtils::SCREEN_HEIGHT - interfaceOffsetY - 3;

	return Int2(textX, textY);
}

Int2 GameWorldUiView::getActionTextPosition()
{
	const int textX = ArenaRenderUtils::SCREEN_WIDTH / 2;
	const int textY = 20;
	return Int2(textX, textY);
}

Int2 GameWorldUiView::getEffectTextPosition()
{
	// @todo
	return Int2::Zero;
}

double GameWorldUiView::getTriggerTextSeconds(const std::string_view text)
{
	return std::max(2.50, static_cast<double>(text.size()) * 0.050);
}

double GameWorldUiView::getActionTextSeconds(const std::string_view text)
{
	return std::max(2.25, static_cast<double>(text.size()) * 0.050);
}

double GameWorldUiView::getEffectTextSeconds(const std::string_view text)
{
	return std::max(2.50, static_cast<double>(text.size()) * 0.050);
}

UiListBoxInitInfo GameWorldUiView::getLootListBoxProperties()
{
	const FontLibrary &fontLibrary = FontLibrary::getInstance();

	constexpr const char *fontName = ArenaFontName::Teeny;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName, &fontDefIndex))
	{
		DebugCrash("Couldn't get loot list box font \"" + std::string(fontName) + "\".");
	}

	constexpr int maxDisplayedItemCount = 7;
	std::string dummyText;
	for (int i = 0; i < maxDisplayedItemCount; i++)
	{
		if (i > 0)
		{
			dummyText += '\n';
		}

		std::string dummyLine(24, TextRenderUtils::LARGEST_CHAR); // Arbitrary worst-case line size.
		dummyText += dummyLine;
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	UiListBoxInitInfo listBoxInitInfo;
	listBoxInitInfo.textureWidth = textureGenInfo.width;
	listBoxInitInfo.textureHeight = textureGenInfo.height;
	listBoxInitInfo.fontName = fontName;
	listBoxInitInfo.defaultTextColor = InventoryUiView::PlayerInventoryEquipmentColor;
	return listBoxInitInfo;
}

Int2 GameWorldUiView::getTooltipPosition(Game &game)
{
	DebugAssert(!game.options.getGraphics_ModernInterface());

	const int x = 0;
	const int y = ArenaRenderUtils::SCREEN_HEIGHT - GameWorldUiView::UiBottomRegion.height;
	return Int2(x, y);
}

Rect GameWorldUiView::getCompassClipRect()
{
	constexpr int width = 32;
	constexpr int height = 7;
	return Rect(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - (width / 2),
		height,
		width,
		height);
}

Int2 GameWorldUiView::getCompassSliderPosition(Game &game, const VoxelDouble2 &playerDirection)
{
	const double angle = GameWorldUiModel::getCompassAngle(playerDirection);

	// Offset in the "slider" texture. Due to how SLIDER.IMG is drawn, there's a small "pop-in" when turning from
	// N to NE, because N is drawn in two places, but the second place (offset == 256) has tick marks where "NE"
	// should be.
	const int xOffset = static_cast<int>(240.0 + std::round(256.0 * (angle / (2.0 * Constants::Pi)))) % 256;
	const Rect clipRect = GameWorldUiView::getCompassClipRect();
	return clipRect.getTopLeft() - Int2(xOffset, 0);
}

Int2 GameWorldUiView::getCompassFramePosition()
{
	return Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, 0);
}

Int2 GameWorldUiView::getWeaponAnimationOffset(const std::string &weaponFilename, int frameIndex,
	TextureManager &textureManager)
{
	// @todo: this is obsoleted by WeaponAnimationDefinition

	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(weaponFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugCrash("Couldn't get weapon animation metadata from \"" + weaponFilename + "\".");
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	return textureFileMetadata.getOffset(frameIndex);
}

Int2 GameWorldUiView::getInterfaceCenter(Game &game)
{
	const bool modernInterface = game.options.getGraphics_ModernInterface();
	if (modernInterface)
	{
		return Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, ArenaRenderUtils::SCREEN_HEIGHT / 2);
	}
	else
	{
		return Int2(
			ArenaRenderUtils::SCREEN_WIDTH / 2,
			(ArenaRenderUtils::SCREEN_HEIGHT - GameWorldUiView::UiBottomRegion.height) / 2);
	}
}

Int2 GameWorldUiView::getNativeWindowCenter(const Window &window)
{
	const Int2 windowDims = window.getPixelDimensions();
	const Int2 nativeCenter = windowDims / 2;
	return nativeCenter;
}

TextureAsset GameWorldUiView::getPaletteTextureAsset()
{
	return TextureAsset(std::string(ArenaPaletteName::Default));
}

TextureAsset GameWorldUiView::getGameWorldInterfaceTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::GameWorldInterface));
}

TextureAsset GameWorldUiView::getStatusGradientTextureAsset(StatusGradientType gradientType)
{
	const int gradientID = static_cast<int>(gradientType);
	return TextureAsset(std::string(ArenaTextureName::StatusGradients), gradientID);
}

TextureAsset GameWorldUiView::getPlayerPortraitTextureAsset(bool isMale, int raceID, int portraitID)
{
	const CharacterRaceLibrary &characterRaceLibrary = CharacterRaceLibrary::getInstance();
	const CharacterRaceDefinition &characterRaceDefinition = characterRaceLibrary.getDefinition(raceID);

	if (isMale)
	{
		return TextureAsset(std::string(characterRaceDefinition.maleGameUiHeadsFilename), portraitID);
	}
	else
	{
		return TextureAsset(std::string(characterRaceDefinition.femaleGameUiHeadsFilename), portraitID);
	}
}

TextureAsset GameWorldUiView::getNoMagicTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::NoSpell));
}

TextureAsset GameWorldUiView::getWeaponAnimTextureAsset(const std::string &weaponFilename, int index)
{
	// @todo: this is obsoleted by WeaponAnimationDefinition
	return TextureAsset(std::string(weaponFilename), index);
}

TextureAsset GameWorldUiView::getCompassFrameTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::CompassFrame));
}

TextureAsset GameWorldUiView::getCompassSliderTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::CompassSlider));
}

TextureAsset GameWorldUiView::getArrowCursorTextureAsset(int cursorIndex)
{
	return TextureAsset(std::string(ArenaTextureName::ArrowCursors), cursorIndex);
}

TextureAsset GameWorldUiView::getKeyTextureAsset(int keyIndex)
{
	return TextureAsset(std::string(ArenaTextureName::DoorKeys), keyIndex);
}

TextureAsset GameWorldUiView::getContainerInventoryTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::ContainerInventory));
}

UiTextureID GameWorldUiView::allocStatusBarsTexture(TextureManager &textureManager, Renderer &renderer)
{
	constexpr int textureWidth = (GameWorldUiView::SpellPointsBarRect.x + GameWorldUiView::SpellPointsBarRect.width) - GameWorldUiView::HealthBarRect.x;
	constexpr int textureHeight = GameWorldUiView::HealthBarRect.height;
	const UiTextureID textureID = renderer.createUiTexture(textureWidth, textureHeight);
	if (textureID < 0)
	{
		DebugLogError("Couldn't create status bars texture.");
		return -1;
	}

	LockedTexture lockedTexture = renderer.lockUiTexture(textureID);
	if (!lockedTexture.isValid())
	{
		DebugLogError("Couldn't lock status bars texture.");
		return textureID;
	}

	Span2D<uint32_t> texelsView = lockedTexture.getTexels32();
	texelsView.fill(Colors::TransparentRGBA);
	renderer.unlockUiTexture(textureID);

	return textureID;
}

int GameWorldUiView::getStatusBarCurrentPixelHeight(double currentValue, double maxValue)
{
	constexpr int barHeight = GameWorldUiView::HealthBarRect.height;
	constexpr double barHeightReal = static_cast<double>(barHeight);

	const double percent = currentValue / maxValue;
	return std::clamp(static_cast<int>(std::round(barHeightReal * percent)), 0, barHeight);
}

void GameWorldUiView::updateStatusBarsTexture(UiTextureID textureID, const Player &player, Renderer &renderer)
{
	LockedTexture lockedTexture = renderer.lockUiTexture(textureID);
	if (!lockedTexture.isValid())
	{
		DebugLogError("Couldn't lock status bars texture for updating.");
		return;
	}

	Span2D<uint32_t> texelsView = lockedTexture.getTexels32();
	texelsView.fill(Colors::TransparentRGBA);

	constexpr int barWidth = GameWorldUiView::HealthBarRect.width;
	constexpr int barHeight = GameWorldUiView::HealthBarRect.height;

	const int currentHealthBarHeight = GameWorldUiView::getStatusBarCurrentPixelHeight(player.currentHealth, player.maxHealth);
	const int currentStaminaBarHeight = GameWorldUiView::getStatusBarCurrentPixelHeight(player.currentStamina, player.maxStamina);
	const int currentSpellPointsBarHeight = GameWorldUiView::getStatusBarCurrentPixelHeight(player.currentSpellPoints, player.maxSpellPoints);

	constexpr uint32_t healthBarColorRGBA = GameWorldUiView::HealthBarColor.toRGBA();
	constexpr uint32_t staminaBarColorRGBA = GameWorldUiView::StaminaBarColor.toRGBA();
	constexpr uint32_t spellPointsBarColorRGBA = GameWorldUiView::SpellPointsBarColor.toRGBA();

	constexpr int staminaBarXOffset = 10;
	constexpr int spellPointsBarXOffset = 20;

	for (int y = 0; y < currentHealthBarHeight; y++)
	{
		for (int x = 0; x < barWidth; x++)
		{
			texelsView.set(x, barHeight - 1 - y, healthBarColorRGBA);
		}
	}

	for (int y = 0; y < currentStaminaBarHeight; y++)
	{
		for (int x = 0; x < barWidth; x++)
		{
			texelsView.set(staminaBarXOffset + x, barHeight - 1 - y, staminaBarColorRGBA);
		}
	}

	for (int y = 0; y < currentSpellPointsBarHeight; y++)
	{
		for (int x = 0; x < barWidth; x++)
		{
			texelsView.set(spellPointsBarXOffset + x, barHeight - 1 - y, spellPointsBarColorRGBA);
		}
	}

	renderer.unlockUiTexture(textureID);
}

UiTextureID GameWorldUiView::allocStatusGradientTexture(StatusGradientType gradientType,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = GameWorldUiView::getStatusGradientTextureAsset(gradientType);
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for status gradient " + std::to_string(static_cast<int>(gradientType)) + ".");
	}

	return textureID;
}

UiTextureID GameWorldUiView::allocPlayerPortraitTexture(bool isMale, int raceID, int portraitID,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = GameWorldUiView::getPlayerPortraitTextureAsset(isMale, raceID, portraitID);
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for player portrait (male: " + std::to_string(static_cast<int>(isMale)) +
			", race: " + std::to_string(raceID) + ", portrait: " + std::to_string(portraitID) + ").");
	}

	return textureID;
}

UiTextureID GameWorldUiView::allocWeaponAnimTexture(const std::string &weaponFilename, int index,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = GameWorldUiView::getWeaponAnimTextureAsset(weaponFilename, index);
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for weapon animation \"" + weaponFilename +
			"\" index " + std::to_string(index) + ".");
	}

	return textureID;
}

UiTextureID GameWorldUiView::allocTooltipTexture(GameWorldUiModel::ButtonType buttonType,
	const FontLibrary &fontLibrary, Renderer &renderer)
{
	const std::string text = GameWorldUiModel::getButtonTooltip(buttonType);
	const Surface surface = TextureUtils::createTooltip(text, fontLibrary);

	Span2D<const uint32_t> pixels = surface.getPixels();
	const UiTextureID textureID = renderer.createUiTexture(pixels.getWidth(), pixels.getHeight());
	if (textureID < 0)
	{
		DebugLogErrorFormat("Couldn't create tooltip texture for \"%s\".", text.c_str());
		return -1;
	}

	if (!renderer.populateUiTextureNoPalette(textureID, pixels))
	{
		DebugLogErrorFormat("Couldn't populate tooltip texture for \"%s\".", text.c_str());
	}

	return textureID;
}

UiTextureID GameWorldUiView::allocArrowCursorTexture(int cursorIndex, TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = GameWorldUiView::getArrowCursorTextureAsset(cursorIndex);
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for arrow cursor " + std::to_string(cursorIndex) + ".");
	}

	return textureID;
}

UiTextureID GameWorldUiView::allocModernModeReticleTexture(TextureManager &textureManager, Renderer &renderer)
{
	constexpr int width = 7;
	constexpr int height = width;

	const UiTextureID textureID = renderer.createUiTexture(width, height);
	if (textureID < 0)
	{
		DebugCrash("Couldn't create modern mode cursor texture.");
	}

	LockedTexture lockedTexture = renderer.lockUiTexture(textureID);
	Span2D<uint32_t> texelsView = lockedTexture.getTexels32();

	constexpr Color cursorBgColor(0, 0, 0, 0);
	const uint32_t cursorBgRGBA = cursorBgColor.toRGBA();
	texelsView.fill(cursorBgRGBA);

	constexpr Color cursorColor(255, 255, 255, 160);
	const uint32_t cursorColorRGBA = cursorColor.toRGBA();

	constexpr int middleX = width / 2;
	constexpr int middleY = height / 2;

	for (int x = 0; x < (middleX - 1); x++)
	{
		texelsView.set(x, middleY, cursorColorRGBA);
		texelsView.set(width - x - 1, middleY, cursorColorRGBA);
	}

	for (int y = 0; y < (middleY - 1); y++)
	{
		texelsView.set(middleX, y, cursorColorRGBA);
		texelsView.set(middleX, height - y - 1, cursorColorRGBA);
	}

	renderer.unlockUiTexture(textureID);

	return textureID;
}

UiTextureID GameWorldUiView::allocKeyTexture(int keyIndex, TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = GameWorldUiView::getKeyTextureAsset(keyIndex);
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrashFormat("Couldn't create UI texture for key %d.", keyIndex);
	}

	return textureID;
}

// @temp: keep until 3D-DDA ray casting is fully correct (i.e. entire ground is red dots for
// levels where ceilingScale < 1.0, and same with ceiling blue dots).
// @todo: As of SDL 2.0.10 which introduced batching, this now behaves like the color is per frame, not per call, which isn't correct, and flushing doesn't help.
void GameWorldUiView::DEBUG_ColorRaycastPixel(Game &game)
{
	const Window &window = game.window;
	auto &renderer = game.renderer;
	const int selectionDim = 3;
	const Int2 windowDims = window.getPixelDimensions();

	constexpr int xOffset = 16;
	constexpr int yOffset = 16;

	const auto &gameState = game.gameState;
	if (!gameState.isActiveMapValid())
	{
		return;
	}

	const auto &player = game.player;
	const CoordDouble3 rayStart = player.getEyeCoord();
	const Double3 &cameraDirection = player.forward;
	const double viewAspectRatio = window.getSceneViewAspectRatio();

	const double ceilingScale = gameState.getActiveCeilingScale();
	const SceneManager &sceneManager = game.sceneManager;
	const VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	const EntityChunkManager &entityChunkManager = sceneManager.entityChunkManager;
	const CollisionChunkManager &collisionChunkManager = sceneManager.collisionChunkManager;

	for (int y = 0; y < windowDims.y; y += yOffset)
	{
		for (int x = 0; x < windowDims.x; x += xOffset)
		{
			const Double3 rayDirection = GameWorldUiModel::screenToWorldRayDirection(game, Int2(x, y));

			// Not registering entities with ray cast hits for efficiency since this debug visualization is for voxels.
			constexpr bool includeEntities = false;
			RayCastHit hit;
			const bool success = Physics::rayCast(rayStart, rayDirection, ceilingScale, cameraDirection,
				includeEntities, voxelChunkManager, entityChunkManager, collisionChunkManager,
				EntityDefinitionLibrary::getInstance(), hit);

			if (success)
			{
				Color color;
				switch (hit.type)
				{
				case RayCastHitType::Voxel:
				{
					constexpr Color colors[] = { Colors::Red, Colors::Green, Colors::Blue, Colors::Cyan, Colors::Yellow };
					const RayCastVoxelHit &voxelHit = hit.voxelHit;
					const VoxelInt3 voxel = voxelHit.voxelCoord.voxel;
					const int colorsIndex = std::clamp<int>(voxel.y, 0, std::size(colors) - 1);
					DebugAssertIndex(colors, colorsIndex);
					color = colors[colorsIndex];
					break;
				}
				case RayCastHitType::Entity:
				{
					color = Colors::Yellow;
					break;
				}
				}

				DebugNotImplemented();
				//renderer.drawRect(color, x, y, selectionDim, selectionDim);
			}
		}
	}
}

// @temp: keep until 3D-DDA ray casting is fully correct (i.e. entire ground is red dots for
// levels where ceilingScale < 1.0, and same with ceiling blue dots).
void GameWorldUiView::DEBUG_PhysicsRaycast(Game &game)
{
	// ray cast out from center and display hit info (faster/better than console logging).
	GameWorldUiView::DEBUG_ColorRaycastPixel(game);

	const Options &options = game.options;
	const Player &player = game.player;
	const Double3 &cameraDirection = player.forward;

	const Window &window = game.window;
	const Int2 viewDims = window.getSceneViewDimensions();
	const Int2 viewCenterPoint(viewDims.x / 2, viewDims.y / 2);

	const CoordDouble3 rayStart = player.getEyeCoord();
	const VoxelDouble3 rayDirection = GameWorldUiModel::screenToWorldRayDirection(game, viewCenterPoint);

	const SceneManager &sceneManager = game.sceneManager;
	const VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	const EntityChunkManager &entityChunkManager = sceneManager.entityChunkManager;
	const CollisionChunkManager &collisionChunkManager = sceneManager.collisionChunkManager;

	const GameState &gameState = game.gameState;
	const double ceilingScale = gameState.getActiveCeilingScale();

	EntityDefinitionLibrary &entityDefLibrary = EntityDefinitionLibrary::getInstance();

	constexpr bool includeEntities = true;
	RayCastHit hit;
	const bool success = Physics::rayCast(rayStart, rayDirection, ceilingScale, cameraDirection, includeEntities,
		voxelChunkManager, entityChunkManager, collisionChunkManager, entityDefLibrary, hit);

	std::string text;
	if (success)
	{
		switch (hit.type)
		{
		case RayCastHitType::Voxel:
		{
			const RayCastVoxelHit &voxelHit = hit.voxelHit;
			const ChunkInt2 chunkPos = voxelHit.voxelCoord.chunk;
			const VoxelInt3 voxel = voxelHit.voxelCoord.voxel;

			const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
			const VoxelTraitsDefID voxelTraitsDefID = voxelChunk.traitsDefIDs.get(voxel.x, voxel.y, voxel.z);
			const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.traitsDefs[voxelTraitsDefID];

			text = "Voxel: (" + voxel.toString() + "), " + std::to_string(static_cast<int>(voxelTraitsDef.type)) + ' ' + std::to_string(hit.t);
			break;
		}
		case RayCastHitType::Entity:
		{
			const RayCastEntityHit &entityHit = hit.entityHit;
			const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();

			// Try inspecting the entity (can be from any distance). If they have a display name, then show it.
			const EntityInstance &entityInst = entityChunkManager.getEntity(entityHit.id);
			const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
			const auto &charClassLibrary = CharacterClassLibrary::getInstance();

			std::string entityName;
			if (EntityUtils::tryGetDisplayName(entityDef, charClassLibrary, &entityName))
			{
				text = std::move(entityName);
			}
			else
			{
				// Placeholder text for testing.
				text = "Entity " + std::to_string(entityHit.id);
			}

			text.append(' ' + std::to_string(hit.t));
			break;
		}
		default:
			text.append("Unknown hit type");
			break;
		}
	}
	else
	{
		text = "No hit";
	}

	Renderer &renderer = game.renderer;

	DebugNotImplemented(); // Disabled for now until I need it again
	/*const TextBoxInitInfo textBoxInitInfo = TextBoxInitInfo::makeWithXY(
		text,
		0,
		0,
		ArenaFontName::Arena,
		Colors::White,
		TextAlignment::TopLeft,
		FontLibrary::getInstance());

	TextBox textBox;
	if (!textBox.init(textBoxInitInfo, text, renderer))
	{
		DebugCrash("Couldn't init physics ray cast text box.");
	}

	const int originalX = ArenaRenderUtils::SCREEN_WIDTH / 2;
	const int originalY = (ArenaRenderUtils::SCREEN_HEIGHT / 2) + 10;*/
	//renderer.drawOriginal(textBox.getTextureID(), originalX, originalY);
}

DebugVoxelVisibilityQuadtreeState GameWorldUiView::allocDebugVoxelVisibilityQuadtreeState(Renderer &renderer)
{
	DebugVoxelVisibilityQuadtreeState state;

	for (int treeLevelIndex = 0; treeLevelIndex < VoxelFrustumCullingChunk::TREE_LEVEL_COUNT; treeLevelIndex++)
	{
		DebugAssertIndex(VoxelFrustumCullingChunk::NODES_PER_SIDE, treeLevelIndex);
		const int quadtreeTextureDim = VoxelFrustumCullingChunk::NODES_PER_SIDE[treeLevelIndex];
		const UiTextureID quadtreeTextureID = renderer.createUiTexture(quadtreeTextureDim, quadtreeTextureDim);
		if (quadtreeTextureID < 0)
		{
			DebugLogErrorFormat("Couldn't allocate voxel visibility quadtree debug texture %d.", treeLevelIndex);
			continue;
		}

		state.textureIDs[treeLevelIndex] = quadtreeTextureID;

		const Int2 textureDims = *renderer.tryGetUiTextureDims(quadtreeTextureID);
		state.textureDimsList[treeLevelIndex] = Int2(textureDims.x, textureDims.y);
	}

	for (int treeLevelIndex = 0; treeLevelIndex < VoxelFrustumCullingChunk::TREE_LEVEL_COUNT; treeLevelIndex++)
	{
		int quadtreeDrawPositionY = 0;
		for (int i = treeLevelIndex; i < VoxelFrustumCullingChunk::TREE_LEVEL_INDEX_LEAF; i++)
		{
			const int yDimIndex = VoxelFrustumCullingChunk::TREE_LEVEL_INDEX_LEAF - (i - treeLevelIndex);
			DebugAssertIndex(state.textureDimsList, yDimIndex);
			quadtreeDrawPositionY += state.textureDimsList[yDimIndex].y;
		}

		state.drawPositionYs[treeLevelIndex] = quadtreeDrawPositionY;
	}

	return state;
}

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
