#include <cstring>
#include <numeric>

#include "AutomapUiState.h"
#include "CharacterUiState.h"
#include "GameWorldUiMVC.h"
#include "GameWorldUiState.h"
#include "LogbookUiState.h"
#include "PauseMenuUiState.h"
#include "WorldMapUiState.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Interface/DialogueManager.h"
#include "../Math/RandomUtils.h"
#include "../Player/PlayerLogic.h"
#include "../Player/WeaponAnimationLibrary.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../UI/FontLibrary.h"
#include "../UI/TextEntry.h"
#include "../UI/UiRenderSpace.h"
#include "../World/ArenaInteriorUtils.h"
#include "../World/CardinalDirection.h"
#include "../World/MapType.h"

#include "components/utilities/String.h"
#include "components/utilities/StringView.h"

namespace
{
	constexpr char ContextName_TextPopUp[] = "GameWorldTextPopUp";
	constexpr char ContextName_LootPopUp[] = "GameWorldLootPopUp";
	constexpr char ContextName_CampModal[] = "GameWorldCampModal";
	constexpr char ContextName_CampManualHoursModal[] = "GameWorldCampManualHoursModal";
	constexpr char ContextName_ConversationModal[] = "GameWorldConversationModal";
	constexpr char ContextName_ShopkeeperBackground[] = "GameWorldShopkeeperBackground";

	constexpr char InterfaceImageElementName[] = "GameWorldInterfaceImage";
	constexpr char NoMagicImageElementName[] = "GameWorldNoMagicImage";
	constexpr char StatusGradientImageElementName[] = "GameWorldStatusGradientImage";
	constexpr char PlayerPortraitImageElementName[] = "GameWorldPlayerPortraitImage";
	constexpr char StatusBarsImageElementName[] = "GameWorldPlayerStatusBarsImage";

	constexpr char PlayerNameTextBoxElementName[] = "GameWorldPlayerNameTextBox";

	constexpr char CompassSliderImageElementName[] = "GameWorldCompassSlider";
	constexpr char CompassFrameImageElementName[] = "GameWorldCompassFrame";

	constexpr char WeaponImageElementName[] = "GameWorldWeaponImage";
	constexpr char PlayerHurtImageElementName[] = "GameWorldPlayerHurtImage";
	constexpr char ModernModeReticleImageElementName[] = "GameWorldModernModeReticleImage";

	constexpr char TriggerTextBoxElementName[] = "GameWorldTriggerTextBox";
	constexpr char ActionTextBoxElementName[] = "GameWorldActionTextBox";
	constexpr char EffectTextBoxElementName[] = "GameWorldEffectTextBox";
	constexpr char CampingHoursTextBoxElementName[] = "GameWorldCampingHoursTextBox";

	constexpr char ConversationModalListBoxElementName[] = "GameWorldConversationModalListBox";
	constexpr char PlayerGoldTextBoxElementName[] = "GameWorldConversationModalPlayerGoldTextBox";

	constexpr const char *ButtonElementNames[] =
	{
		"GameWorldCharacterSheetButton",
		"GameWorldWeaponToggleButton",
		"GameWorldMapButton",
		"GameWorldStealButton",
		"GameWorldStatusButton",
		"GameWorldMagicButton",
		"GameWorldLogbookButton",
		"GameWorldUseItemButton",
		"GameWorldCampButton",
		"GameWorldScrollUpButton",
		"GameWorldScrollDownButton",
	};

	constexpr MouseButtonTypeFlags PopUpMouseButtonTypeFlags = MouseButtonType::Left | MouseButtonType::Right;

	std::string GetKeyImageElementName(int keyIndex)
	{
		return String::format("GameWorldKey%dImage", keyIndex);
	}

	bool IsPlayerWeaponVisible(const Player &player)
	{
		const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
		const WeaponAnimationDefinitionID weaponAnimDefID = player.getEquippedWeaponAnimationDefID();
		const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(weaponAnimDefID);
		const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
		DebugAssertIndex(weaponAnimDef.states, weaponAnimInst.currentStateIndex);
		const WeaponAnimationDefinitionState &weaponAnimDefState = weaponAnimDef.states[weaponAnimInst.currentStateIndex];
		return !WeaponAnimationUtils::isSheathed(weaponAnimDefState);
	}

	Int2 GetStatusBarsModernModePosition(const Window &window)
	{
		const Int2 windowDims = window.getPixelDimensions();
		const Int2 statusBarModernModeWindowPosition(GameWorldUiView::StatusBarModernModeXOffset, windowDims.y - GameWorldUiView::StatusBarModernModeYOffset);
		return window.nativeToOriginal(statusBarModernModeWindowPosition);
	}

	std::string GetLootItemDisplayNameWithQty(const ItemDefinition &itemDef, int stackAmount)
	{
		std::string displayName = itemDef.getDisplayNameWithQty(stackAmount);
		if (itemDef.type == ItemType::Gold)
		{
			size_t goldCountIndex = displayName.find("%u");
			if (goldCountIndex != std::string::npos)
			{
				displayName.replace(goldCountIndex, 2, std::to_string(stackAmount));
			}
		}

		return displayName;
	}

	std::vector<DialogueDirectionsDetailEntry> GetDirectionsDetailEntries(ArenaMenuType menuType, const VoxelChunkManager &voxelChunkManager)
	{
		std::vector<DialogueDirectionsDetailEntry> detailEntries;

		const std::optional<ArenaInteriorType> interiorType = ArenaInteriorUtils::menuTypeToInteriorType(menuType);

		for (int chunkIndex = 0; chunkIndex < voxelChunkManager.getChunkCount(); chunkIndex++)
		{
			const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtIndex(chunkIndex);
			for (const std::pair<VoxelInt3, VoxelTransitionDefID> &pair : voxelChunk.transitionDefIndices)
			{
				const VoxelInt3 voxel = pair.first;
				const VoxelTransitionDefID transitionDefID = pair.second;
				const TransitionDefinition &transitionDef = voxelChunk.transitionDefs[transitionDefID];

				bool isMatch = false;
				if (transitionDef.type == TransitionType::CityGate)
				{
					isMatch = menuType == ArenaMenuType::CityGates;
				}
				else if (transitionDef.type == TransitionType::EnterInterior)
				{
					if (interiorType.has_value())
					{
						const InteriorEntranceTransitionDefinition &interiorEntranceTransitionDef = transitionDef.interiorEntrance;
						isMatch = interiorEntranceTransitionDef.interiorGenInfo.interiorType == *interiorType;
					}
				}

				if (isMatch)
				{
					std::string buildingName = "<missing building name>";
					VoxelBuildingNameID buildingNameID;
					if (voxelChunk.tryGetBuildingNameID(voxel.x, voxel.y, voxel.z, &buildingNameID))
					{
						buildingName = voxelChunk.buildingNames[buildingNameID];
					}

					const WorldInt3 worldVoxel = VoxelUtils::coordToWorldVoxel(CoordInt3(voxelChunk.position, voxel));
					DialogueDirectionsDetailEntry detailEntry(buildingName, worldVoxel);
					detailEntries.emplace_back(std::move(detailEntry));
				}
			}
		}

		std::sort(detailEntries.begin(), detailEntries.end(),
			[](const DialogueDirectionsDetailEntry &a, const DialogueDirectionsDetailEntry &b)
		{
			return a.buildingName.compare(b.buildingName) < 0;
		});

		return detailEntries;
	}

	Double2 GetDirectionsDetailEntryDirection(const DialogueDirectionsDetailEntry &detailEntry, WorldInt3 playerWorldVoxel)
	{
		const WorldDouble2 detailEntryPositionXZ = VoxelUtils::getVoxelCenter(detailEntry.entranceWorldVoxel.getXZ());
		const WorldDouble2 playerPositionXZ = VoxelUtils::getVoxelCenter(playerWorldVoxel.getXZ());
		return (detailEntryPositionXZ - playerPositionXZ).normalized();
	}

	double GetDirectionsDetailEntryDistanceSqr(const DialogueDirectionsDetailEntry &detailEntry, WorldInt3 playerWorldVoxel)
	{
		const WorldDouble2 detailEntryPositionXZ = VoxelUtils::getVoxelCenter(detailEntry.entranceWorldVoxel.getXZ());
		const WorldDouble2 playerPositionXZ = VoxelUtils::getVoxelCenter(playerWorldVoxel.getXZ());
		return (detailEntryPositionXZ - playerPositionXZ).lengthSquared();
	}

	CardinalDirectionName GetDirectionsDetailEntryCardinalDirection(Double2 direction)
	{
		DebugAssert(direction.isNormalized());
		return CardinalDirection::getDirectionName(direction);
	}

	// How far a citizen will attempt to give directions.
	constexpr double DIRECTIONS_DETAIL_ENTRY_MAX_DISTANCE = 128.0;
	constexpr double DIRECTIONS_DETAIL_ENTRY_MAX_DISTANCE_SQR = DIRECTIONS_DETAIL_ENTRY_MAX_DISTANCE * DIRECTIONS_DETAIL_ENTRY_MAX_DISTANCE;
	constexpr double DIRECTIONS_DETAIL_ENTRY_MARK_ON_MAP_DISTANCE = 16.0;
	constexpr double DIRECTIONS_DETAIL_ENTRY_MARK_ON_MAP_DISTANCE_SQR = DIRECTIONS_DETAIL_ENTRY_MARK_ON_MAP_DISTANCE * DIRECTIONS_DETAIL_ENTRY_MARK_ON_MAP_DISTANCE;

	int GetNearestDirectionsDetailEntryIndex(Span<const DialogueDirectionsDetailEntry> detailEntries, WorldInt3 playerWorldVoxel, const VoxelChunkManager &voxelChunkManager)
	{
		int closestIndex = -1;
		for (int i = 0; i < detailEntries.getCount(); i++)
		{
			if (closestIndex < 0)
			{
				closestIndex = i;
				continue;
			}

			const DialogueDirectionsDetailEntry &currentDetailEntry = detailEntries[i];
			const DialogueDirectionsDetailEntry &closestDetailEntry = detailEntries[closestIndex];
			const double currentDetailEntryDistanceSqr = GetDirectionsDetailEntryDistanceSqr(currentDetailEntry, playerWorldVoxel);
			const double closestDetailEntryDistanceSqr = GetDirectionsDetailEntryDistanceSqr(closestDetailEntry, playerWorldVoxel);
			if (currentDetailEntryDistanceSqr < closestDetailEntryDistanceSqr)
			{
				closestIndex = i;
			}
		}

		return closestIndex;
	}

	std::string GetSubstitutedTextForDirectionsEntry(const DialogueDirectionsDetailEntry *detailEntry, WorldInt3 playerWorldVoxel, DialogueManager &dialogueManager, bool *outIsEntryValidForAutomap)
	{
		*outIsEntryValidForAutomap = false;

		const TextAssetLibrary &textAssetLibrary = TextAssetLibrary::getInstance();
		const ArenaTemplateDat &templateDat = textAssetLibrary.templateDat;

		double detailEntryDistanceSqr = Constants::Infinity;
		if (detailEntry != nullptr)
		{
			detailEntryDistanceSqr = GetDirectionsDetailEntryDistanceSqr(*detailEntry, playerWorldVoxel);
		}

		int entryKey = -1;
		if (detailEntryDistanceSqr >= DIRECTIONS_DETAIL_ENTRY_MAX_DISTANCE_SQR)
		{
			// Too far away to know direction.
			entryKey = 259;
		}
		else if (detailEntryDistanceSqr <= DIRECTIONS_DETAIL_ENTRY_MARK_ON_MAP_DISTANCE_SQR)
		{
			// Close enough to mark on player's map.
			entryKey = 261;
			*outIsEntryValidForAutomap = true;
		}
		else
		{
			// Worth giving directions for.
			entryKey = 260;
		}

		const std::string entryValue = dialogueManager.getRandomTemplateDatEntryValue(entryKey);
		if (detailEntry != nullptr)
		{
			const Double2 direction = GetDirectionsDetailEntryDirection(*detailEntry, playerWorldVoxel);
			const CardinalDirectionName cardinalDirectionName = GetDirectionsDetailEntryCardinalDirection(direction);
			dialogueManager.dialogueDirection = cardinalDirectionName;
		}

		return dialogueManager.getSubstitutedText(entryValue.c_str());
	}
}

GameWorldLootUiItemMapping::GameWorldLootUiItemMapping()
{
	this->inventoryItemIndex = -1;
	this->listBoxItemIndex = -1;
}

void GameWorldUiInitInfo::init(const std::string &textPopUpMessage)
{
	this->textPopUpMessage = textPopUpMessage;
}

GameWorldUiState::GameWorldUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->textPopUpContextInstID = -1;
	this->lootPopUpContextInstID = -1;
	this->campModalContextInstID = -1;
	this->campManualHoursModalContextInstID = -1;
	this->statusBarsTextureID = -1;
	this->playerHurtTextureID = -1;
	this->modernModeReticleTextureID = -1;
	this->currentHealth = 0.0;
	this->maxHealth = 0.0;
	this->currentStamina = 0.0;
	this->maxStamina = 0.0;
	this->currentSpellPoints = 0.0;
	this->maxSpellPoints = 0.0;
	this->interactionType = GameWorldInteractionType::Default;
	this->triggerTextRemainingSeconds = 0.0;
	this->actionTextRemainingSeconds = 0.0;
	this->effectTextRemainingSeconds = 0.0;
	this->playerHurtRemainingSeconds = 0.0;
}

void GameWorldUiState::init(Game &game)
{
	const Player &player = game.player;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	this->game = &game;

	this->statusBarsTextureID = GameWorldUiView::allocStatusBarsTexture(textureManager, renderer);

	const int keyTextureCount = GameWorldUiView::getKeyTextureCount(textureManager);
	this->keyTextureIDs.init(keyTextureCount);
	for (int i = 0; i < keyTextureCount; i++)
	{
		const UiTextureID keyTextureID = GameWorldUiView::allocKeyTexture(i, textureManager, renderer);
		this->keyTextureIDs.set(i, keyTextureID);
	}

	this->arrowCursorTextureIDs.init(GameWorldUiView::ArrowCursorRegionCount);
	for (int i = 0; i < GameWorldUiView::ArrowCursorRegionCount; i++)
	{
		const UiTextureID arrowTextureID = GameWorldUiView::allocArrowCursorTexture(i, textureManager, renderer);
		this->arrowCursorTextureIDs.set(i, arrowTextureID);
	}

	const Window &window = game.window;
	this->playerHurtTextureID = GameWorldUiView::allocPlayerHurtTexture(window.getSceneViewAspectRatio(), window.fullGameWindow, renderer);

	this->modernModeReticleTextureID = GameWorldUiView::allocModernModeReticleTexture(textureManager, renderer);

	const Int2 windowDims = window.getPixelDimensions();
	this->updateNativeCursorRegions(windowDims.x, windowDims.y);

	this->currentHealth = player.currentHealth;
	this->maxHealth = player.maxHealth;
	this->currentStamina = player.currentStamina;
	this->maxStamina = player.maxStamina;
	this->currentSpellPoints = player.currentSpellPoints;
	this->maxSpellPoints = player.maxSpellPoints;
	this->interactionType = GameWorldInteractionType::Default;
	this->dialogueStartPlayerEffectsState.clear();
}

void GameWorldUiState::freeTextures(Renderer &renderer)
{
	if (this->statusBarsTextureID >= 0)
	{
		renderer.freeUiTexture(this->statusBarsTextureID);
		this->statusBarsTextureID = -1;
	}

	if (this->keyTextureIDs.isValid())
	{
		for (const UiTextureID textureID : this->keyTextureIDs)
		{
			renderer.freeUiTexture(textureID);
		}

		this->keyTextureIDs.clear();
	}

	if (this->arrowCursorTextureIDs.isValid())
	{
		for (const UiTextureID textureID : this->arrowCursorTextureIDs)
		{
			renderer.freeUiTexture(textureID);
		}

		this->arrowCursorTextureIDs.clear();
	}

	if (this->playerHurtTextureID >= 0)
	{
		renderer.freeUiTexture(this->playerHurtTextureID);
		this->playerHurtTextureID = -1;
	}

	if (this->modernModeReticleTextureID >= 0)
	{
		renderer.freeUiTexture(this->modernModeReticleTextureID);
		this->modernModeReticleTextureID = -1;
	}
}

void GameWorldUiState::updateNativeCursorRegions(int windowWidth, int windowHeight)
{
	// @todo: maybe the classic rects should be converted to vector space then scaled by the ratio of aspect ratios?
	const double xScale = static_cast<double>(windowWidth) / ArenaRenderUtils::SCREEN_WIDTH_REAL;
	const double yScale = static_cast<double>(windowHeight) / ArenaRenderUtils::SCREEN_HEIGHT_REAL;

	for (int i = 0; i < static_cast<int>(std::size(this->nativeCursorRegions)); i++)
	{
		this->nativeCursorRegions[i] = GameWorldUiView::scaleClassicCursorRectToNative(i, xScale, yScale);
	}
}

void GameWorldUI::create(Game &game)
{
	GameWorldUiState &state = GameWorldUI::state;
	state.init(game);

	const GameState &gameState = game.gameState;
	const Options &options = game.options;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(GameWorldUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	UiContextInitInfo textPopUpContextInitInfo;
	textPopUpContextInitInfo.name = ContextName_TextPopUp;
	textPopUpContextInitInfo.drawOrder = 1;
	state.textPopUpContextInstID = uiManager.createContext(textPopUpContextInitInfo);
	uiManager.setContextEnabled(state.textPopUpContextInstID, false);

	UiContextInitInfo lootPopUpContextInitInfo;
	lootPopUpContextInitInfo.name = ContextName_LootPopUp;
	lootPopUpContextInitInfo.drawOrder = 1;
	state.lootPopUpContextInstID = uiManager.createContext(lootPopUpContextInitInfo);
	uiManager.setContextEnabled(state.lootPopUpContextInstID, false);

	UiContextInitInfo campModalContextInitInfo;
	campModalContextInitInfo.name = ContextName_CampModal;
	campModalContextInitInfo.drawOrder = 1;
	state.campModalContextInstID = uiManager.createContext(campModalContextInitInfo);
	uiManager.setContextEnabled(state.campModalContextInstID, false);

	UiContextInitInfo campManualHoursModalContextInitInfo;
	campManualHoursModalContextInitInfo.name = ContextName_CampManualHoursModal;
	campManualHoursModalContextInitInfo.drawOrder = 1;
	state.campManualHoursModalContextInstID = uiManager.createContext(campManualHoursModalContextInitInfo);
	uiManager.setContextEnabled(state.campManualHoursModalContextInstID, false);

	UiContextInitInfo conversationModalContextInitInfo;
	conversationModalContextInitInfo.name = ContextName_ConversationModal;
	conversationModalContextInitInfo.drawOrder = 2;
	state.conversationModalContextInstID = uiManager.createContext(conversationModalContextInitInfo);
	uiManager.setContextEnabled(state.conversationModalContextInstID, false);

	UiContextInitInfo shopkeeperBackgroundContextInitInfo;
	shopkeeperBackgroundContextInitInfo.name = ContextName_ShopkeeperBackground;
	shopkeeperBackgroundContextInitInfo.drawOrder = 1;
	state.shopkeeperBgContextInstID = uiManager.createContext(shopkeeperBackgroundContextInitInfo);
	uiManager.setContextEnabled(state.shopkeeperBgContextInstID, false);

	const bool isModernInterface = options.getGraphics_ModernInterface();
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();

	UiElementInitInfo weaponAnimImageElementInitInfo;
	weaponAnimImageElementInitInfo.name = WeaponImageElementName;
	weaponAnimImageElementInitInfo.sizeType = isModernInterface ? UiTransformSizeType::Manual : UiTransformSizeType::Content;
	weaponAnimImageElementInitInfo.drawOrder = 0;
	weaponAnimImageElementInitInfo.renderSpace = isModernInterface ? UiRenderSpace::Native : UiRenderSpace::Classic;

	const WeaponAnimationDefinition &dummyWeaponAnimDef = WeaponAnimationLibrary::getInstance().getDefinition(ArenaItemUtils::FistsWeaponID);
	const TextureAsset &dummyWeaponAnimFrameTextureAsset = dummyWeaponAnimDef.frames[0].textureAsset;
	const UiTextureID dummyWeaponAnimImageTextureID = uiManager.getOrAddTexture(dummyWeaponAnimFrameTextureAsset, paletteTextureAsset, textureManager, renderer);
	uiManager.createImage(weaponAnimImageElementInitInfo, dummyWeaponAnimImageTextureID, state.contextInstID, renderer);

	for (int i = 0; i < state.keyTextureIDs.getCount(); i++)
	{
		UiElementInitInfo keyImageElementInitInfo;
		keyImageElementInitInfo.name = GetKeyImageElementName(i);
		keyImageElementInitInfo.position = GameWorldUiView::getKeyPosition(i);
		keyImageElementInitInfo.drawOrder = 0;

		const UiTextureID keyImageTextureID = state.keyTextureIDs[i];
		uiManager.createImage(keyImageElementInitInfo, keyImageTextureID, state.contextInstID, renderer);
	}

	GameWorldUI::updateDoorKeys();

	UiElementInitInfo statusBarsImageElementInitInfo;
	statusBarsImageElementInitInfo.name = StatusBarsImageElementName;
	statusBarsImageElementInitInfo.position = GameWorldUiView::HealthBarRect.getBottomLeft();

	if (isModernInterface)
	{
		statusBarsImageElementInitInfo.position = GetStatusBarsModernModePosition(game.window);
	}

	statusBarsImageElementInitInfo.pivotType = GameWorldUiView::StatusBarPivotType;
	statusBarsImageElementInitInfo.drawOrder = 5;
	uiManager.createImage(statusBarsImageElementInitInfo, state.statusBarsTextureID, state.contextInstID, renderer);

	UiElementInitInfo modernModeReticleImageElementInitInfo;
	modernModeReticleImageElementInitInfo.name = ModernModeReticleImageElementName;
	modernModeReticleImageElementInitInfo.position = GameWorldUiView::getInterfaceCenter(game);
	modernModeReticleImageElementInitInfo.pivotType = UiPivotType::Middle;
	modernModeReticleImageElementInitInfo.drawOrder = 1;
	const UiElementInstanceID modernModeReticleImageElementInstID = uiManager.createImage(modernModeReticleImageElementInitInfo, state.modernModeReticleTextureID, state.contextInstID, renderer);

	UiElementInitInfo playerHurtImageElementInitInfo;
	playerHurtImageElementInitInfo.name = PlayerHurtImageElementName;
	playerHurtImageElementInitInfo.sizeType = UiTransformSizeType::Manual;
	playerHurtImageElementInitInfo.size = game.window.getSceneViewDimensions();
	playerHurtImageElementInitInfo.renderSpace = UiRenderSpace::Native;
	playerHurtImageElementInitInfo.drawOrder = 3;
	const UiElementInstanceID playerHurtImageElementInstID = uiManager.createImage(playerHurtImageElementInitInfo, state.playerHurtTextureID, state.contextInstID, renderer);
	uiManager.setElementActive(playerHurtImageElementInstID, false);

	const Player &player = game.player;
	const TextureAsset playerPortraitTextureAsset = GameWorldUiView::getPlayerPortraitTextureAsset(player.male, player.raceID, player.portraitID);
	const UiTextureID playerPortraitTextureID = uiManager.getOrAddTexture(playerPortraitTextureAsset, paletteTextureAsset, textureManager, renderer);
	const UiElementInstanceID playerPortraitImageElementInstID = uiManager.getElementByName(PlayerPortraitImageElementName);
	uiManager.setImageTexture(playerPortraitImageElementInstID, playerPortraitTextureID);

	const UiElementInstanceID playerNameTextBoxElementInstID = uiManager.getElementByName(PlayerNameTextBoxElementName);
	const std::string playerNameText = GameWorldUiModel::getPlayerNameText(game);
	uiManager.setTextBoxText(playerNameTextBoxElementInstID, playerNameText.c_str());

	const UiElementInstanceID triggerTextBoxElementInstID = uiManager.getElementByName(TriggerTextBoxElementName);
	const Int2 triggerTextBoxPosition = GameWorldUiView::getTriggerTextPosition(game, ArenaRenderUtils::SCENE_UI_HEIGHT);
	uiManager.setTransformPosition(triggerTextBoxElementInstID, triggerTextBoxPosition);

	const UiElementInstanceID actionTextBoxElementInstID = uiManager.getElementByName(ActionTextBoxElementName);
	const Int2 actionTextBoxPosition = GameWorldUiView::getActionTextPosition();
	uiManager.setTransformPosition(actionTextBoxElementInstID, actionTextBoxPosition);

	const UiElementInstanceID effectTextBoxElementInstID = uiManager.getElementByName(EffectTextBoxElementName);
	const Int2 effectTextBoxPosition = GameWorldUiView::getEffectTextPosition(game, ArenaRenderUtils::SCENE_UI_HEIGHT);
	uiManager.setTransformPosition(effectTextBoxElementInstID, effectTextBoxPosition);

	if (isModernInterface)
	{
		const UiElementInstanceID interfaceImageElementInstID = uiManager.getElementByName(InterfaceImageElementName);
		uiManager.setElementActive(interfaceImageElementInstID, false);

		const UiElementInstanceID noMagicImageElementInstID = uiManager.getElementByName(NoMagicImageElementName);
		uiManager.setElementActive(noMagicImageElementInstID, false);

		const UiElementInstanceID statusGradientImageElementInstID = uiManager.getElementByName(StatusGradientImageElementName);
		uiManager.setElementActive(statusGradientImageElementInstID, false);

		uiManager.setElementActive(playerPortraitImageElementInstID, false);
		uiManager.setElementActive(playerNameTextBoxElementInstID, false);

		for (const char *buttonElementName : ButtonElementNames)
		{
			const UiElementInstanceID buttonElementInstID = uiManager.getElementByName(buttonElementName);
			uiManager.setElementActive(buttonElementInstID, false);
		}

		uiManager.setElementActive(game.cursorImageElementInstID, false);

		GameWorldUiModel::setFreeLookActive(game, true);
	}
	else
	{
		uiManager.setElementActive(modernModeReticleImageElementInstID, false);
	}

	GameWorldUiView::updateStatusBarsTexture(state.statusBarsTextureID, game.player, renderer);

	const bool isCompassVisible = options.getMisc_ShowCompass() && !gameState.isCamping();
	GameWorldUI::setCompassVisible(isCompassVisible);

	uiManager.addMouseButtonChangedListener(GameWorldUI::onMouseButtonChanged, GameWorldUI::ContextName, inputManager);
	uiManager.addMouseButtonHeldListener(GameWorldUI::onMouseButtonHeld, GameWorldUI::ContextName, inputManager);
	uiManager.addWindowResizedListener(GameWorldUI::onWindowResized, GameWorldUI::ContextName, inputManager);

	game.shouldSimulateScene = true;
	game.shouldRenderScene = true;

	inputManager.setInputActionMapActive(InputActionMapName::GameWorld, true);

	if (!state.initInfo.textPopUpMessage.empty())
	{
		GameWorldUI::showTextPopUp(state.initInfo.textPopUpMessage.c_str(), GameWorldUiView::StatusPopUpFontName, GameWorldUiView::StatusPopUpTextAlignment);
	}
}

void GameWorldUI::destroy()
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	DialogueManager &dialogueManager = game.dialogueManager;
	Renderer &renderer = game.renderer;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, renderer);
		state.contextInstID = -1;
	}

	if (state.textPopUpContextInstID >= 0)
	{
		uiManager.freeContext(state.textPopUpContextInstID, inputManager, renderer);
		state.textPopUpContextInstID = -1;
	}

	if (state.lootPopUpContextInstID >= 0)
	{
		uiManager.freeContext(state.lootPopUpContextInstID, inputManager, renderer);
		state.lootPopUpContextInstID = -1;
	}

	if (state.campModalContextInstID >= 0)
	{
		uiManager.freeContext(state.campModalContextInstID, inputManager, renderer);
		state.campModalContextInstID = -1;
	}

	if (state.campManualHoursModalContextInstID >= 0)
	{
		uiManager.freeContext(state.campManualHoursModalContextInstID, inputManager, renderer);
		state.campManualHoursModalContextInstID = -1;
	}

	if (state.conversationModalContextInstID >= 0)
	{
		uiManager.freeContext(state.conversationModalContextInstID, inputManager, renderer);
		state.conversationModalContextInstID = -1;
	}

	if (state.shopkeeperBgContextInstID >= 0)
	{
		uiManager.freeContext(state.shopkeeperBgContextInstID, inputManager, renderer);
		state.shopkeeperBgContextInstID = -1;
	}

	state.freeTextures(renderer);
	state.initInfo.textPopUpMessage.clear();
	state.currentHealth = 0.0;
	state.maxHealth = 0.0;
	state.currentStamina = 0.0;
	state.maxStamina = 0.0;
	state.currentSpellPoints = 0.0;
	state.maxSpellPoints = 0.0;
	state.playerHurtRemainingSeconds = 0.0;
	state.lootPopUpItemMappings.clear();
	state.dialogueWhereIsDetailEntries.clear();

	dialogueManager.endDialogue();

	inputManager.setInputActionMapActive(InputActionMapName::GameWorld, false);

	const Options &options = game.options;
	if (options.getGraphics_ModernInterface())
	{
		uiManager.setElementActive(game.cursorImageElementInstID, true);
		GameWorldUiModel::setFreeLookActive(game, false);
	}

	game.shouldSimulateScene = false;
	game.shouldRenderScene = false;
}

void GameWorldUI::update(double dt)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	const Options &options = game.options;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	const Window &window = game.window;
	Renderer &renderer = game.renderer;

	const bool isModernInterface = options.getGraphics_ModernInterface();
	const TextureAsset paletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();

	// Compass
	const Player &player = game.player;
	const Double2 playerDirection = player.getGroundDirectionXZ();
	const Int2 compassSliderPosition = GameWorldUiView::getCompassSliderPosition(game, playerDirection);
	const UiElementInstanceID compassSliderImageElementInstID = uiManager.getElementByName(CompassSliderImageElementName);
	const UiElementInstanceID compassFrameImageElementInstID = uiManager.getElementByName(CompassFrameImageElementName);
	uiManager.setTransformPosition(compassSliderImageElementInstID, compassSliderPosition);

	const bool isCompassVisible = options.getMisc_ShowCompass() && !gameState.isCamping();
	GameWorldUI::setCompassVisible(isCompassVisible);

	// Weapon
	const UiElementInstanceID weaponImageElementInstID = uiManager.getElementByName(WeaponImageElementName);
	const bool isWeaponVisible = IsPlayerWeaponVisible(player);
	uiManager.setElementActive(weaponImageElementInstID, isWeaponVisible);
	if (isWeaponVisible)
	{
		const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
		const WeaponAnimationDefinitionID weaponAnimDefID = player.getEquippedWeaponAnimationDefID();
		const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(weaponAnimDefID);
		const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
		const int weaponAnimFrameIndex = WeaponAnimationUtils::getFrameIndex(weaponAnimInst, weaponAnimDef);
		DebugAssertIndex(weaponAnimDef.frames, weaponAnimFrameIndex);
		const WeaponAnimationDefinitionFrame &weaponAnimFrame = weaponAnimDef.frames[weaponAnimFrameIndex];
		const UiTextureID weaponAnimTextureID = uiManager.getOrAddTexture(weaponAnimFrame.textureAsset, paletteTextureAsset, textureManager, renderer);
		const std::optional<Int2> weaponTextureDims = renderer.tryGetUiTextureDims(weaponAnimTextureID);
		DebugAssert(weaponTextureDims.has_value());

		Int2 weaponPosition;
		Int2 weaponSize;
		if (isModernInterface)
		{
			constexpr int classicViewHeight = ArenaRenderUtils::SCENE_VIEW_HEIGHT;
			const Int2 windowDims = window.getPixelDimensions();
			const Double2 windowDimsReal(
				static_cast<double>(windowDims.x),
				static_cast<double>(windowDims.y));
			const Double2 weaponOffsetPercents(
				static_cast<double>(weaponAnimFrame.xOffset) / ArenaRenderUtils::SCREEN_WIDTH_REAL,
				static_cast<double>(weaponAnimFrame.yOffset) / static_cast<double>(classicViewHeight));
			const Double2 weaponTextureScreenSizePercents(
				static_cast<double>(weaponTextureDims->x) / ArenaRenderUtils::SCREEN_WIDTH_REAL,
				static_cast<double>(weaponTextureDims->y) / static_cast<double>(classicViewHeight));

			weaponPosition = Int2(
				static_cast<int>(std::round(weaponOffsetPercents.x * windowDimsReal.x)),
				static_cast<int>(std::round(weaponOffsetPercents.y * windowDimsReal.y)));
			weaponSize = Int2(
				static_cast<int>(std::round(weaponTextureScreenSizePercents.x * windowDimsReal.x)),
				static_cast<int>(std::round(weaponTextureScreenSizePercents.y * windowDimsReal.y)));
		}
		else
		{
			weaponPosition = Int2(weaponAnimFrame.xOffset, weaponAnimFrame.yOffset);
			weaponSize = *weaponTextureDims;
		}

		uiManager.setTransformPosition(weaponImageElementInstID, weaponPosition);
		uiManager.setTransformSize(weaponImageElementInstID, weaponSize);
		uiManager.setImageTexture(weaponImageElementInstID, weaponAnimTextureID);
	}

	// Status bars
	if (isModernInterface)
	{
		const UiElementInstanceID statusBarsImageElementInstID = uiManager.getElementByName(StatusBarsImageElementName);
		const Int2 statusBarsModernModePosition = GetStatusBarsModernModePosition(game.window);
		uiManager.setTransformPosition(statusBarsImageElementInstID, statusBarsModernModePosition);
	}

	const int previousHealthBarHeight = GameWorldUiView::getStatusBarCurrentPixelHeight(state.currentHealth, state.maxHealth);
	const int previousStaminaBarHeight = GameWorldUiView::getStatusBarCurrentPixelHeight(state.currentStamina, state.maxStamina);
	const int previousSpellPointsBarHeight = GameWorldUiView::getStatusBarCurrentPixelHeight(state.currentSpellPoints, state.maxSpellPoints);
	const int currentHealthBarHeight = GameWorldUiView::getStatusBarCurrentPixelHeight(player.currentHealth, player.maxHealth);
	const int currentStaminaBarHeight = GameWorldUiView::getStatusBarCurrentPixelHeight(player.currentStamina, player.maxStamina);
	const int currentSpellPointsBarHeight = GameWorldUiView::getStatusBarCurrentPixelHeight(player.currentSpellPoints, player.maxSpellPoints);

	const bool isStatusBarsTextureDirty = (previousHealthBarHeight != currentHealthBarHeight) ||
		(previousStaminaBarHeight != currentStaminaBarHeight) ||
		(previousSpellPointsBarHeight != currentSpellPointsBarHeight);

	if (isStatusBarsTextureDirty)
	{
		state.currentHealth = player.currentHealth;
		state.maxHealth = player.maxHealth;
		state.currentStamina = player.currentStamina;
		state.maxStamina = player.maxStamina;
		state.currentSpellPoints = player.currentSpellPoints;
		state.maxSpellPoints = player.maxSpellPoints;
		GameWorldUiView::updateStatusBarsTexture(state.statusBarsTextureID, player, renderer);
	}

	// Trigger/action/effect text
	const bool isTriggerTextVisible = GameWorldUI::isTriggerTextVisible();
	const UiElementInstanceID triggerTextBoxElementInstID = uiManager.getElementByName(TriggerTextBoxElementName);
	uiManager.setElementActive(triggerTextBoxElementInstID, isTriggerTextVisible);

	const bool isActionTextVisible = GameWorldUI::isActionTextVisible();
	const UiElementInstanceID actionTextBoxElementInstID = uiManager.getElementByName(ActionTextBoxElementName);
	uiManager.setElementActive(actionTextBoxElementInstID, isActionTextVisible);

	const bool isEffectTextVisible = GameWorldUI::isEffectTextVisible();
	const UiElementInstanceID effectTextBoxElementInstID = uiManager.getElementByName(EffectTextBoxElementName);
	uiManager.setElementActive(effectTextBoxElementInstID, isEffectTextVisible);

	const bool isCampingHoursTextVisible = GameWorldUI::isCampingHoursTextVisible();
	const UiElementInstanceID campingHoursTextBoxElementInstID = uiManager.getElementByName(CampingHoursTextBoxElementName);
	uiManager.setElementActive(campingHoursTextBoxElementInstID, isCampingHoursTextVisible);

	if (!isModernInterface)
	{
		const PlayerStatusGradientType statusGradientType = GameWorldUiModel::getCurrentPlayerStatusGradientType(player);
		const TextureAsset statusGradientTextureAsset = GameWorldUiView::getStatusGradientTextureAsset(statusGradientType);
		const UiTextureID statusGradientTextureID = uiManager.getOrAddTexture(statusGradientTextureAsset, paletteTextureAsset, textureManager, renderer);
		const UiElementInstanceID statusGradientElementInstID = uiManager.getElementByName(StatusGradientImageElementName);
		uiManager.setImageTexture(statusGradientElementInstID, statusGradientTextureID);

		const InputManager &inputManager = game.inputManager;
		const Int2 cursorPosition = inputManager.getMousePosition();

		int arrowCursorRegionIndex = -1;
		for (int i = 0; i < GameWorldUiView::ArrowCursorRegionCount; i++)
		{
			const Rect &nativeCursorRegion = state.nativeCursorRegions[i];
			if (nativeCursorRegion.contains(cursorPosition))
			{
				arrowCursorRegionIndex = i;
				break;
			}
		}

		if (arrowCursorRegionIndex >= 0)
		{
			const UiTextureID cursorTextureID = state.arrowCursorTextureIDs[arrowCursorRegionIndex];

			Span<const UiPivotType> arrowCursorPivotTypes = GameWorldUiView::ArrowCursorPivotTypes;
			const UiPivotType cursorPivotType = arrowCursorPivotTypes[arrowCursorRegionIndex];
			game.setCursorOverride(UiCursorOverrideState(cursorTextureID, cursorPivotType));
		}
		else
		{
			game.setCursorOverride(std::nullopt);
		}
	}

	// Player hurt.
	const bool isPlayerHurtVisible = state.playerHurtRemainingSeconds > 0.0;
	const UiElementInstanceID playerHurtImageElementInstID = uiManager.getElementByName(PlayerHurtImageElementName);
	uiManager.setElementActive(playerHurtImageElementInstID, isPlayerHurtVisible);

	if (isPlayerHurtVisible)
	{
		state.playerHurtRemainingSeconds = std::max(state.playerHurtRemainingSeconds - dt, 0.0);
	}
}

void GameWorldUI::onScreenToWorldInteraction(Int2 windowPoint, bool isPrimaryInteraction)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	const InputManager &inputManager = game.inputManager;
	const bool debugFadeVoxel = isPrimaryInteraction && inputManager.keyIsDown(SDL_SCANCODE_G);
	PlayerLogic::handleScreenToWorldInteraction(game, windowPoint, isPrimaryInteraction, debugFadeVoxel);
}

void GameWorldUI::updateDoorKeys()
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	const Player &player = game.player;
	UiManager &uiManager = game.uiManager;
	const Renderer &renderer = game.renderer;

	const Span<const int> keyInventory = player.keyInventory;
	for (int i = 0; i < state.keyTextureIDs.getCount(); i++)
	{
		const std::string keyImageElementName = GetKeyImageElementName(i);
		const UiElementInstanceID keyImageElementInstID = uiManager.getElementByName(keyImageElementName.c_str());

		const int keyID = keyInventory[i];
		const bool isKeyVisible = game.shouldSimulateScene && (keyID != ArenaItemUtils::InvalidDoorKeyID);
		uiManager.setElementActive(keyImageElementInstID, isKeyVisible);

		if (isKeyVisible)
		{
			const UiTextureID keyTextureID = state.keyTextureIDs[keyID];
			const std::optional<Int2> keyDimensions = renderer.tryGetUiTextureDims(keyTextureID);
			DebugAssert(keyDimensions.has_value());

			uiManager.setTransformSize(keyImageElementInstID, *keyDimensions);
			uiManager.setImageTexture(keyImageElementInstID, keyTextureID);
		}
	}
}

void GameWorldUI::setCompassVisible(bool visible)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID compassSliderImageElementInstID = uiManager.getElementByName(CompassSliderImageElementName);
	const UiElementInstanceID compassFrameImageElementInstID = uiManager.getElementByName(CompassFrameImageElementName);
	uiManager.setElementActive(compassSliderImageElementInstID, visible);
	uiManager.setElementActive(compassFrameImageElementInstID, visible);
}

void GameWorldUI::setInteractionType(GameWorldInteractionType type)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

	state.interactionType = type;

	switch (type)
	{
	case GameWorldInteractionType::Default:
	{
		GameWorldUI::setActionText("");
		state.actionTextRemainingSeconds = 0.0;
		break;
	}
	case GameWorldInteractionType::Thieving:
	{
		GameWorldUI::setActionText(exeData.thieving.thievingInteractionType.c_str());
		state.actionTextRemainingSeconds = Constants::Infinity;
		break;
	}
	default:
		DebugNotImplementedMsg(std::to_string(static_cast<int>(type)));
		break;
	}
}

void GameWorldUI::onPauseChanged(bool paused)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	const GameState &gameState = game.gameState;
	const Player &player = game.player;
	UiManager &uiManager = game.uiManager;
	const Options &options = game.options;
	const bool isModernInterface = options.getGraphics_ModernInterface();

	game.shouldSimulateScene = !paused;

	const bool isWeaponVisible = !paused && IsPlayerWeaponVisible(player);
	const UiElementInstanceID weaponImageElementInstID = uiManager.getElementByName(WeaponImageElementName);
	uiManager.setElementActive(weaponImageElementInstID, isWeaponVisible);

	const bool isCompassVisible = !paused && options.getMisc_ShowCompass() && !gameState.isCamping();
	GameWorldUI::setCompassVisible(isCompassVisible);

	GameWorldUI::updateDoorKeys();

	const bool isModernModeReticleVisible = !paused && isModernInterface;
	const UiElementInstanceID modernModeReticleImageElementInstID = uiManager.getElementByName(ModernModeReticleImageElementName);
	uiManager.setElementActive(modernModeReticleImageElementInstID, isModernModeReticleVisible);

	if (paused)
	{
		const UiElementInstanceID triggerTextBoxElementInstID = uiManager.getElementByName(TriggerTextBoxElementName);
		uiManager.setElementActive(triggerTextBoxElementInstID, false);

		const UiElementInstanceID actionTextBoxElementInstID = uiManager.getElementByName(ActionTextBoxElementName);
		uiManager.setElementActive(actionTextBoxElementInstID, false);

		const UiElementInstanceID effectTextBoxElementInstID = uiManager.getElementByName(EffectTextBoxElementName);
		uiManager.setElementActive(effectTextBoxElementInstID, false);

		game.setCursorOverride(std::nullopt);
	}

	if (isModernInterface)
	{
		uiManager.setElementActive(game.cursorImageElementInstID, paused);
		GameWorldUiModel::setFreeLookActive(game, !paused);
	}
}

void GameWorldUI::showTextPopUp(const char *str, const std::string &fontName, TextAlignment alignment, const GameWorldPopUpClosedCallback &callback)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	uiManager.clearContextElements(state.textPopUpContextInstID, inputManager, renderer);

	UiElementInitInfo textPopUpTextBoxElementInitInfo;
	textPopUpTextBoxElementInitInfo.name = "GameWorldTextPopUpTextBox";
	textPopUpTextBoxElementInitInfo.position = GameWorldUiView::getStatusPopUpTextCenterPoint(game);
	textPopUpTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	textPopUpTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo textPopUpTextBoxInitInfo;
	textPopUpTextBoxInitInfo.text = str;
	textPopUpTextBoxInitInfo.fontName = fontName;
	textPopUpTextBoxInitInfo.defaultColor = GameWorldUiView::StatusPopUpTextColor;
	textPopUpTextBoxInitInfo.alignment = alignment;
	textPopUpTextBoxInitInfo.lineSpacing = GameWorldUiView::StatusPopUpTextLineSpacing;
	const UiElementInstanceID textPopUpTextBoxElementInstID = uiManager.createTextBox(textPopUpTextBoxElementInitInfo, textPopUpTextBoxInitInfo, state.textPopUpContextInstID, renderer);
	const Rect textPopUpTextBoxRect = uiManager.getTransformGlobalRect(textPopUpTextBoxElementInstID);

	UiElementInitInfo textPopUpImageElementInitInfo;
	textPopUpImageElementInitInfo.name = "GameWorldTextPopUpImage";
	textPopUpImageElementInitInfo.position = GameWorldUiView::getStatusPopUpTextCenterPoint(game);
	textPopUpImageElementInitInfo.pivotType = UiPivotType::Middle;
	textPopUpImageElementInitInfo.drawOrder = 0;

	const int textPopUpImageTextureWidth = GameWorldUiView::getStatusPopUpTextureWidth(textPopUpTextBoxRect.width);
	const int textPopUpImageTextureHeight = GameWorldUiView::getStatusPopUpTextureHeight(textPopUpTextBoxRect.height);
	const UiTextureID textPopUpImageTextureID = uiManager.getOrAddTexture(GameWorldUiView::StatusPopUpTexturePatternType, textPopUpImageTextureWidth, textPopUpImageTextureHeight, textureManager, renderer);
	uiManager.createImage(textPopUpImageElementInitInfo, textPopUpImageTextureID, state.textPopUpContextInstID, renderer);

	UiElementInitInfo textPopUpBackButtonElementInitInfo;
	textPopUpBackButtonElementInitInfo.name = "GameWorldTextPopUpBackButton";
	textPopUpBackButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	textPopUpBackButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	textPopUpBackButtonElementInitInfo.drawOrder = 2;

	auto popUpButtonCallback = [callback](MouseButtonType)
	{
		callback();
	};

	UiButtonInitInfo textPopUpBackButtonInitInfo;
	textPopUpBackButtonInitInfo.mouseButtonFlags = PopUpMouseButtonTypeFlags;
	textPopUpBackButtonInitInfo.callback = popUpButtonCallback;
	uiManager.createButton(textPopUpBackButtonElementInitInfo, textPopUpBackButtonInitInfo, state.textPopUpContextInstID);

	auto inputActionCallback = [popUpButtonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			popUpButtonCallback(MouseButtonType::Left);
		}
	};

	uiManager.addInputActionListener(InputActionName::Back, inputActionCallback, ContextName_TextPopUp, inputManager);
	uiManager.setContextEnabled(state.textPopUpContextInstID, true);

	GameWorldUI::onPauseChanged(true);
}

void GameWorldUI::showTextPopUp(const char *str, const std::string &fontName, TextAlignment alignment)
{
	auto callback = []()
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		GameWorldUiController::onPopUpSelected(game);
	};

	GameWorldUI::showTextPopUp(str, fontName, alignment, callback);
}

void GameWorldUI::showLootPopUp(ItemInventory &itemInventory, const GameWorldPopUpClosedCallback &callback)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	uiManager.clearContextElements(state.lootPopUpContextInstID, inputManager, renderer);

	UiElementInitInfo lootPopUpImageElementInitInfo;
	lootPopUpImageElementInitInfo.name = "GameWorldLootPopUpImage";
	lootPopUpImageElementInitInfo.position = Int2(56, 10);

	const TextureAsset lootPopUpTextureAsset = GameWorldUiView::getContainerInventoryTextureAsset();
	const TextureAsset lootPopUpPaletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();
	const UiTextureID lootPopUpTextureID = uiManager.getOrAddTexture(lootPopUpTextureAsset, lootPopUpPaletteTextureAsset, textureManager, renderer);
	uiManager.createImage(lootPopUpImageElementInitInfo, lootPopUpTextureID, state.lootPopUpContextInstID, renderer);

	UiElementInitInfo lootPopUpListBoxElementInitInfo;
	lootPopUpListBoxElementInitInfo.name = "GameWorldLootPopUpListBox";
	lootPopUpListBoxElementInitInfo.position = Int2(85, 34);
	lootPopUpListBoxElementInitInfo.drawOrder = 1;

	const UiListBoxInitInfo lootPopUpListBoxInitInfo = GameWorldUiView::getLootListBoxProperties();
	const UiElementInstanceID listBoxElementInstID = uiManager.createListBox(lootPopUpListBoxElementInitInfo, lootPopUpListBoxInitInfo, state.lootPopUpContextInstID, renderer);

	UiElementInitInfo lootPopUpListBoxUpButtonElementInitInfo;
	lootPopUpListBoxUpButtonElementInitInfo.name = "GameWorldLootPopUpListBoxUpButton";
	lootPopUpListBoxUpButtonElementInitInfo.position = Int2(65, 19);
	lootPopUpListBoxUpButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	lootPopUpListBoxUpButtonElementInitInfo.size = Int2(9, 9);

	UiButtonInitInfo lootPopUpListBoxUpButtonInitInfo;
	lootPopUpListBoxUpButtonInitInfo.callback = [&uiManager, listBoxElementInstID](MouseButtonType) { uiManager.scrollListBoxUp(listBoxElementInstID); };
	uiManager.createButton(lootPopUpListBoxUpButtonElementInitInfo, lootPopUpListBoxUpButtonInitInfo, state.lootPopUpContextInstID);

	UiElementInitInfo lootPopUpListBoxDownButtonElementInitInfo;
	lootPopUpListBoxDownButtonElementInitInfo.name = "GameWorldLootPopUpListBoxDownButton";
	lootPopUpListBoxDownButtonElementInitInfo.position = Int2(65, 92);
	lootPopUpListBoxDownButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	lootPopUpListBoxDownButtonElementInitInfo.size = Int2(9, 9);

	UiButtonInitInfo lootPopUpListBoxDownButtonInitInfo;
	lootPopUpListBoxDownButtonInitInfo.callback = [&uiManager, listBoxElementInstID](MouseButtonType) { uiManager.scrollListBoxDown(listBoxElementInstID); };
	uiManager.createButton(lootPopUpListBoxDownButtonElementInitInfo, lootPopUpListBoxDownButtonInitInfo, state.lootPopUpContextInstID);

	UiElementInitInfo lootPopUpBackButtonElementInitInfo;
	lootPopUpBackButtonElementInitInfo.name = "GameWorldLootPopUpBackButton";
	lootPopUpBackButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	lootPopUpBackButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);

	auto lootPopUpBackButtonCallback = [&state, callback](MouseButtonType)
	{
		callback();
		GameWorldUI::onPauseChanged(false);
	};

	UiButtonInitInfo lootPopUpBackButtonInitInfo;
	lootPopUpBackButtonInitInfo.mouseButtonFlags = MouseButtonTypeFlags(MouseButtonType::Right);
	lootPopUpBackButtonInitInfo.callback = lootPopUpBackButtonCallback;
	uiManager.createButton(lootPopUpBackButtonElementInitInfo, lootPopUpBackButtonInitInfo, state.lootPopUpContextInstID);

	auto lootPopUpBackInputActionCallback = [lootPopUpBackButtonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			lootPopUpBackButtonCallback(MouseButtonType::Right);
		}
	};

	uiManager.addInputActionListener(InputActionName::Back, lootPopUpBackInputActionCallback, ContextName_LootPopUp, inputManager);

	auto lootPopUpMouseWheelScrollChangedCallback = [&uiManager, listBoxElementInstID](Game &game, MouseWheelScrollType type, const Int2 &position)
	{
		if (type == MouseWheelScrollType::Down)
		{
			uiManager.scrollListBoxDown(listBoxElementInstID);
		}
		else if (type == MouseWheelScrollType::Up)
		{
			uiManager.scrollListBoxUp(listBoxElementInstID);
		}
	};

	uiManager.addMouseScrollChangedListener(lootPopUpMouseWheelScrollChangedCallback, ContextName_LootPopUp, inputManager);

	state.lootPopUpItemMappings.clear();
	for (int i = 0; i < itemInventory.getTotalSlotCount(); i++)
	{
		const ItemInstance &itemInst = itemInventory.getSlot(i);
		if (!itemInst.isValid())
		{
			continue;
		}

		const int listBoxItemIndex = uiManager.getListBoxItemCount(listBoxElementInstID);

		std::vector<GameWorldLootUiItemMapping> &itemMappings = state.lootPopUpItemMappings;
		GameWorldLootUiItemMapping itemMapping;
		itemMapping.inventoryItemIndex = i;
		itemMapping.listBoxItemIndex = listBoxItemIndex;
		itemMappings.emplace_back(itemMapping);

		const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
		const ItemDefinition &itemDef = itemLibrary.getDefinition(itemInst.defID);
		std::string itemDisplayName = GetLootItemDisplayNameWithQty(itemDef, itemInst.stackAmount);

		auto listBoxItemCallback = [&game, &itemInventory, &uiManager, &itemLibrary, listBoxElementInstID, lootPopUpBackButtonCallback, listBoxItemIndex, &itemMappings](MouseButtonType)
		{
			// Find which inventory item slot this list box item points to.
			int itemMappingsIndex = -1;
			for (int curItemMappingsIndex = 0; curItemMappingsIndex < static_cast<int>(itemMappings.size()); curItemMappingsIndex++)
			{
				if (itemMappings[curItemMappingsIndex].listBoxItemIndex == listBoxItemIndex)
				{
					itemMappingsIndex = curItemMappingsIndex;
					break;
				}
			}

			DebugAssert(itemMappingsIndex >= 0);
			const int mappedInventoryItemIndex = itemMappings[itemMappingsIndex].inventoryItemIndex;
			if (mappedInventoryItemIndex < 0)
			{
				// This list box item was emptied previously.
				return;
			}

			ItemInstance &selectedItemInst = itemInventory.getSlot(mappedInventoryItemIndex);
			const ItemDefinitionID selectedItemDefID = selectedItemInst.defID;
			DebugAssert(selectedItemDefID >= 0);
			const ItemDefinition &selectedItemDef = itemLibrary.getDefinition(selectedItemDefID);

			Player &player = game.player;
			if (selectedItemDef.type == ItemType::Gold)
			{
				player.gold += selectedItemInst.stackAmount;
			}
			else
			{
				ItemInventory &playerInventory = player.inventory;
				playerInventory.insert(selectedItemDefID, selectedItemInst.stackAmount);
			}

			selectedItemInst.defID = -1;

			if (itemInventory.getOccupiedSlotCount() == 0)
			{
				lootPopUpBackButtonCallback(MouseButtonType::Right);
			}

			// Shift mappings forward by one.
			for (int curItemMappingsIndex = itemMappingsIndex; curItemMappingsIndex < static_cast<int>(itemMappings.size()); curItemMappingsIndex++)
			{
				GameWorldLootUiItemMapping &curItemMapping = itemMappings[curItemMappingsIndex];

				const int nextItemMappingsIndex = curItemMappingsIndex + 1;
				if (nextItemMappingsIndex < static_cast<int>(itemMappings.size()))
				{
					GameWorldLootUiItemMapping &nextItemMapping = itemMappings[nextItemMappingsIndex];
					curItemMapping.inventoryItemIndex = nextItemMapping.inventoryItemIndex;
				}
				else
				{
					curItemMapping.inventoryItemIndex = -1;
				}

				std::string newListBoxItemText;
				if (curItemMapping.inventoryItemIndex >= 0)
				{
					const ItemInstance &curItemInst = itemInventory.getSlot(curItemMapping.inventoryItemIndex);
					const ItemDefinition &curItemDef = itemLibrary.getDefinition(curItemInst.defID);
					newListBoxItemText = GetLootItemDisplayNameWithQty(curItemDef, curItemInst.stackAmount);
				}

				uiManager.setListBoxItemText(listBoxElementInstID, curItemMapping.listBoxItemIndex, newListBoxItemText.c_str());
			}
		};

		UiListBoxItem listBoxItem;
		listBoxItem.init(itemDisplayName, std::nullopt, listBoxItemCallback);
		uiManager.insertBackListBoxItem(listBoxElementInstID, std::move(listBoxItem));
	}

	// @todo sword cursor

	uiManager.setContextEnabled(state.lootPopUpContextInstID, true);

	GameWorldUI::onPauseChanged(true);
}

void GameWorldUI::showCampModal()
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	uiManager.clearContextElements(state.campModalContextInstID, inputManager, renderer);

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

	constexpr int campModalImageTextureWidth = 156;
	constexpr int campModalImageTextureHeight = 24;
	const UiTextureID campModalImageTextureID = uiManager.getOrAddTexture(UiTexturePatternType::Dark, campModalImageTextureWidth, campModalImageTextureHeight, textureManager, renderer);

	constexpr Color campModalTextColor(215, 158, 4);

	UiElementInitInfo campModalTitleImageElementInitInfo;
	campModalTitleImageElementInitInfo.name = "GameWorldCampModalTitleImage";
	campModalTitleImageElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, 80);
	campModalTitleImageElementInitInfo.pivotType = UiPivotType::Middle;
	uiManager.createImage(campModalTitleImageElementInitInfo, campModalImageTextureID, state.campModalContextInstID, renderer);

	UiElementInitInfo campModalTitleTextBoxElementInitInfo;
	campModalTitleTextBoxElementInitInfo.name = "GameWorldCampModalTitleTextBox";
	campModalTitleTextBoxElementInitInfo.position = campModalTitleImageElementInitInfo.position;
	campModalTitleTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	campModalTitleTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo campModalTitleTextBoxInitInfo;
	campModalTitleTextBoxInitInfo.text = GameWorldUiModel::getCampModalTitleText(exeData);
	campModalTitleTextBoxInitInfo.fontName = ArenaFontName::A;
	campModalTitleTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
	campModalTitleTextBoxInitInfo.defaultColor = campModalTextColor;
	campModalTitleTextBoxInitInfo.tabColorPaletteID = GameWorldUiView::getCampModalTextBoxPaletteID(textureManager);
	uiManager.createTextBox(campModalTitleTextBoxElementInitInfo, campModalTitleTextBoxInitInfo, state.campModalContextInstID, renderer);

	UiElementInitInfo campModalManualHoursImageElementInitInfo;
	campModalManualHoursImageElementInitInfo.name = "GameWorldCampModalManualHoursImage";
	campModalManualHoursImageElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, campModalTitleImageElementInitInfo.position.y + campModalImageTextureHeight);
	campModalManualHoursImageElementInitInfo.pivotType = UiPivotType::Middle;
	uiManager.createImage(campModalManualHoursImageElementInitInfo, campModalImageTextureID, state.campModalContextInstID, renderer);

	UiElementInitInfo campModalManualHoursTextBoxElementInitInfo;
	campModalManualHoursTextBoxElementInitInfo.name = "GameWorldCampModalManualHoursTextBox";
	campModalManualHoursTextBoxElementInitInfo.position = campModalManualHoursImageElementInitInfo.position;
	campModalManualHoursTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	campModalManualHoursTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo campModalManualHoursTextBoxInitInfo;
	campModalManualHoursTextBoxInitInfo.text = GameWorldUiModel::getCampModalManualHoursText(exeData);
	campModalManualHoursTextBoxInitInfo.fontName = ArenaFontName::A;
	campModalManualHoursTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
	campModalManualHoursTextBoxInitInfo.defaultColor = campModalTextColor;
	campModalManualHoursTextBoxInitInfo.tabColorPaletteID = campModalTitleTextBoxInitInfo.tabColorPaletteID;
	uiManager.createTextBox(campModalManualHoursTextBoxElementInitInfo, campModalManualHoursTextBoxInitInfo, state.campModalContextInstID, renderer);

	UiElementInitInfo campModalManualHoursButtonElementInitInfo;
	campModalManualHoursButtonElementInitInfo.name = "GameWorldCampModalManualHoursButton";
	campModalManualHoursButtonElementInitInfo.position = campModalManualHoursImageElementInitInfo.position;
	campModalManualHoursButtonElementInitInfo.pivotType = UiPivotType::Middle;

	auto campModalManualHoursButtonCallback = [](MouseButtonType)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		game.uiManager.disableTopMostContext();
		game.inputManager.setInputActionMapActive(InputActionMapName::Camping, false);
		GameWorldUI::showCampManualHoursModal();
	};

	UiButtonInitInfo campModalManualHoursButtonInitInfo;
	campModalManualHoursButtonInitInfo.callback = campModalManualHoursButtonCallback;
	campModalManualHoursButtonInitInfo.contentElementName = campModalManualHoursImageElementInitInfo.name;
	uiManager.createButton(campModalManualHoursButtonElementInitInfo, campModalManualHoursButtonInitInfo, state.campModalContextInstID);

	UiElementInitInfo campModalUntilHealedImageElementInitInfo;
	campModalUntilHealedImageElementInitInfo.name = "GameWorldCampModalUntilHealedImage";
	campModalUntilHealedImageElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, campModalManualHoursImageElementInitInfo.position.y + campModalImageTextureHeight);
	campModalUntilHealedImageElementInitInfo.pivotType = UiPivotType::Middle;
	uiManager.createImage(campModalUntilHealedImageElementInitInfo, campModalImageTextureID, state.campModalContextInstID, renderer);

	UiElementInitInfo campModalUntilHealedTextBoxElementInitInfo;
	campModalUntilHealedTextBoxElementInitInfo.name = "GameWorldCampModalUntilHealedTextBox";
	campModalUntilHealedTextBoxElementInitInfo.position = campModalUntilHealedImageElementInitInfo.position;
	campModalUntilHealedTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	campModalUntilHealedTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo campModalUntilHealedTextBoxInitInfo;
	campModalUntilHealedTextBoxInitInfo.text = GameWorldUiModel::getCampModalUntilHealedText(exeData);
	campModalUntilHealedTextBoxInitInfo.fontName = ArenaFontName::A;
	campModalUntilHealedTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
	campModalUntilHealedTextBoxInitInfo.defaultColor = campModalTextColor;
	campModalUntilHealedTextBoxInitInfo.tabColorPaletteID = campModalManualHoursTextBoxInitInfo.tabColorPaletteID;
	uiManager.createTextBox(campModalUntilHealedTextBoxElementInitInfo, campModalUntilHealedTextBoxInitInfo, state.campModalContextInstID, renderer);

	UiElementInitInfo campModalUntilHealedButtonElementInitInfo;
	campModalUntilHealedButtonElementInitInfo.name = "GameWorldCampModalUntilHealedButton";
	campModalUntilHealedButtonElementInitInfo.position = campModalUntilHealedImageElementInitInfo.position;
	campModalUntilHealedButtonElementInitInfo.pivotType = UiPivotType::Middle;

	auto campModalUntilHealedButtonCallback = [&exeData](MouseButtonType)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		game.uiManager.disableTopMostContext();
		game.inputManager.setInputActionMapActive(InputActionMapName::Camping, false);

		Player &player = game.player;
		if (!player.canRestUntilHealed())
		{
			GameWorldUI::showTextPopUp(exeData.camping.alreadyFullyRested.c_str(), ArenaFontName::A, GameWorldUiView::StatusPopUpTextAlignment);
			return;
		}

		GameState &gameState = game.gameState;
		gameState.setCampingUntilHealed();

		GameWorldUI::onPauseChanged(false);
	};

	UiButtonInitInfo campModalUntilHealedButtonInitInfo;
	campModalUntilHealedButtonInitInfo.callback = campModalUntilHealedButtonCallback;
	campModalUntilHealedButtonInitInfo.contentElementName = campModalUntilHealedImageElementInitInfo.name;
	uiManager.createButton(campModalUntilHealedButtonElementInitInfo, campModalUntilHealedButtonInitInfo, state.campModalContextInstID);

	UiElementInitInfo campModalBackButtonElementInitInfo;
	campModalBackButtonElementInitInfo.name = "GameWorldCampModalBackButton";
	campModalBackButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	campModalBackButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);

	auto campModalBackButtonCallback = [&state](MouseButtonType)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		game.uiManager.disableTopMostContext();
		game.inputManager.setInputActionMapActive(InputActionMapName::Camping, false);
		GameWorldUI::onPauseChanged(false);
	};

	UiButtonInitInfo campModalBackButtonInitInfo;
	campModalBackButtonInitInfo.mouseButtonFlags = MouseButtonTypeFlags(MouseButtonType::Right);
	campModalBackButtonInitInfo.callback = campModalBackButtonCallback;
	uiManager.createButton(campModalBackButtonElementInitInfo, campModalBackButtonInitInfo, state.campModalContextInstID);

	auto campModalBackInputActionCallback = [campModalBackButtonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			campModalBackButtonCallback(MouseButtonType::Right);
		}
	};

	auto campModalManualHoursInputActionCallback = [campModalManualHoursButtonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			campModalManualHoursButtonCallback(MouseButtonType::Right);
		}
	};

	auto campModalUntilHealedInputActionCallback = [campModalUntilHealedButtonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			campModalUntilHealedButtonCallback(MouseButtonType::Right);
		}
	};

	uiManager.addInputActionListener(InputActionName::Back, campModalBackInputActionCallback, ContextName_CampModal, inputManager);
	uiManager.addInputActionListener(InputActionName::CampManualHours, campModalManualHoursInputActionCallback, ContextName_CampModal, inputManager);
	uiManager.addInputActionListener(InputActionName::CampUntilHealed, campModalUntilHealedInputActionCallback, ContextName_CampModal, inputManager);

	uiManager.setContextEnabled(state.campModalContextInstID, true);

	inputManager.setInputActionMapActive(InputActionMapName::Camping, true);

	GameWorldUI::onPauseChanged(true);
}

void GameWorldUI::showCampManualHoursModal()
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	uiManager.clearContextElements(state.campManualHoursModalContextInstID, inputManager, renderer);
	state.campManualHoursInputText.clear();

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

	constexpr int imageTextureWidth = 231;
	constexpr int imageTextureHeight = 30;
	const UiTextureID imageTextureID = uiManager.getOrAddTexture(UiTexturePatternType::Dark, imageTextureWidth, imageTextureHeight, textureManager, renderer);

	const int imageY = game.options.getGraphics_ModernInterface() ? (ArenaRenderUtils::SCREEN_HEIGHT / 2) : (ArenaRenderUtils::SCENE_VIEW_HEIGHT / 2);

	UiElementInitInfo imageElementInitInfo;
	imageElementInitInfo.name = "GameWorldCampManualHoursModalImage";
	imageElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, imageY);
	imageElementInitInfo.pivotType = UiPivotType::Middle;
	const UiElementInstanceID imageElementInstID = uiManager.createImage(imageElementInitInfo, imageTextureID, state.campManualHoursModalContextInstID, renderer);
	const Rect imageGlobalRect = uiManager.getTransformGlobalRect(imageElementInstID);

	UiElementInitInfo textBoxElementInitInfo;
	textBoxElementInitInfo.name = "GameWorldCampManualHoursModalTextBox";
	textBoxElementInitInfo.position = imageGlobalRect.getTopLeft() + Int2(13, 11);
	textBoxElementInitInfo.pivotType = UiPivotType::TopLeft;
	textBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo textBoxInitInfo;
	textBoxInitInfo.text = GameWorldUiModel::getCampManualHoursModalText(exeData);
	textBoxInitInfo.fontName = ArenaFontName::Arena;
	textBoxInitInfo.defaultColor = Color(231, 215, 0);
	const UiElementInstanceID textBoxElementInstID = uiManager.createTextBox(textBoxElementInitInfo, textBoxInitInfo, state.campManualHoursModalContextInstID, renderer);
	const Rect textBoxGlobalRect = uiManager.getTransformGlobalRect(textBoxElementInstID);

	UiElementInitInfo inputTextBoxElementInitInfo;
	inputTextBoxElementInitInfo.name = "GameWorldCampManualHoursModalInputTextBox";
	inputTextBoxElementInitInfo.position = textBoxGlobalRect.getTopRight();
	inputTextBoxElementInitInfo.pivotType = UiPivotType::TopLeft;
	inputTextBoxElementInitInfo.drawOrder = 2;

	UiTextBoxInitInfo inputTextBoxInitInfo;
	inputTextBoxInitInfo.worstCaseText = TextRenderUtils::makeWorstCaseText(3);
	inputTextBoxInitInfo.fontName = ArenaFontName::Arena;
	inputTextBoxInitInfo.defaultColor = textBoxInitInfo.defaultColor;
	const UiElementInstanceID inputTextBoxElementInstID = uiManager.createTextBox(inputTextBoxElementInitInfo, inputTextBoxInitInfo, state.campManualHoursModalContextInstID, renderer);

	UiElementInitInfo backButtonElementInitInfo;
	backButtonElementInitInfo.name = "GameWorldCampManualHoursModalBackButton";
	backButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	backButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);

	auto backButtonCallback = [&state](MouseButtonType)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		game.uiManager.disableTopMostContext();
		game.inputManager.setTextInputMode(false);
		GameWorldUI::onPauseChanged(false);
	};

	UiButtonInitInfo backButtonInitInfo;
	backButtonInitInfo.mouseButtonFlags = MouseButtonTypeFlags(MouseButtonType::Right);
	backButtonInitInfo.callback = backButtonCallback;
	uiManager.createButton(backButtonElementInitInfo, backButtonInitInfo, state.campManualHoursModalContextInstID);

	auto backInputActionCallback = [backButtonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			backButtonCallback(MouseButtonType::Right);
		}
	};

	auto acceptInputActionCallback = [backButtonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			backButtonCallback(MouseButtonType::Right);

			GameWorldUiState &state = GameWorldUI::state;
			Game &game = *state.game;
			const std::string &inputText = state.campManualHoursInputText;
			if (inputText.empty())
			{
				return;
			}

			int hoursCount = 0;
			try
			{
				size_t index = 0;
				hoursCount = std::stoi(inputText, &index);
				if (index != inputText.size())
				{
					return;
				}
			}
			catch (std::exception)
			{
				return;
			}

			if (hoursCount > 0)
			{
				GameState &gameState = game.gameState;
				gameState.setCampingManualHours(hoursCount);
			}
		}
	};

	auto textInputCallback = [inputTextBoxElementInstID](const std::string_view text)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		UiManager &uiManager = game.uiManager;

		std::string &inputText = state.campManualHoursInputText;
		auto isCharAllowed = [](char c)
		{
			return (c >= '0') && (c <= '9'); // Numbers only
		};

		constexpr int maxCampHourDigits = 3;
		if (TextEntry::append(inputText, text, isCharAllowed, maxCampHourDigits))
		{
			uiManager.setTextBoxText(inputTextBoxElementInstID, inputText.c_str());
		}
	};

	auto textBackspaceCallback = [inputTextBoxElementInstID](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			GameWorldUiState &state = GameWorldUI::state;
			Game &game = *state.game;
			UiManager &uiManager = game.uiManager;

			std::string &inputText = state.campManualHoursInputText;
			if (TextEntry::backspace(inputText))
			{
				uiManager.setTextBoxText(inputTextBoxElementInstID, inputText.c_str());
			}
		}
	};

	uiManager.addInputActionListener(InputActionName::Back, backInputActionCallback, ContextName_CampManualHoursModal, inputManager);
	uiManager.addInputActionListener(InputActionName::Accept, acceptInputActionCallback, ContextName_CampManualHoursModal, inputManager);
	uiManager.addTextInputListener(textInputCallback, ContextName_CampManualHoursModal, inputManager);
	uiManager.addInputActionListener(InputActionName::Backspace, textBackspaceCallback, ContextName_CampManualHoursModal, inputManager);

	uiManager.setContextEnabled(state.campManualHoursModalContextInstID, true);

	inputManager.setTextInputMode(true);
}

void GameWorldUI::showPlayerHurt()
{
	GameWorldUiState &state = GameWorldUI::state;
	state.playerHurtRemainingSeconds = 1.0 / ArenaRenderUtils::FRAMES_PER_SECOND;
}

void GameWorldUI::setConversationMessageBoxInputActionMapActive(const char *mapName)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;

	constexpr const char *npcMapNames[] =
	{
		InputActionMapName::EquipmentStore,
		InputActionMapName::EquipmentStoreBuy,
		InputActionMapName::MagesGuild,
		InputActionMapName::MagesGuildBuy,
		InputActionMapName::MagesGuildSteal,
		InputActionMapName::NpcGeneral,
		InputActionMapName::NpcRumors,
		InputActionMapName::Tavern,
		InputActionMapName::TavernRumors,
		InputActionMapName::Temple
	};

	for (const char *npcMapName : npcMapNames)
	{
		const bool shouldSetActive = !String::isNullOrEmpty(mapName) && StringView::equals(npcMapName, mapName);
		inputManager.setInputActionMapActive(npcMapName, shouldSetActive);
	}
}

void GameWorldUI::addConversationMessageBoxInputActionListeners(const char *mapName)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;

	const char *contextName = ContextName_ConversationModal;

	const GameState &gameState = game.gameState;
	const Player &player = game.player;
	const CharacterClassDefinition &charClassDef = CharacterClassLibrary::getInstance().getDefinition(player.charClassDefID);

	if (StringView::equals(mapName, InputActionMapName::EquipmentStore))
	{
		uiManager.addInputActionListener(InputActionName::EquipmentStoreBuy, GameWorldUI::onEquipmentStoreBuyInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::EquipmentStoreSell, GameWorldUI::onEquipmentStoreSellInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::EquipmentStoreRepair, GameWorldUI::onEquipmentStoreRepairInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::EquipmentStoreSteal, GameWorldUI::onEquipmentStoreStealInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::EquipmentStoreExit, GameWorldUI::onEquipmentStoreExitInputAction, contextName, inputManager);
	}
	else if (StringView::equals(mapName, InputActionMapName::EquipmentStoreBuy))
	{
		uiManager.addInputActionListener(InputActionName::EquipmentStoreBuyWeapon, GameWorldUI::onEquipmentStoreBuyWeaponInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::EquipmentStoreBuyArmor, GameWorldUI::onEquipmentStoreBuyArmorInputAction, contextName, inputManager);
	}
	else if (StringView::equals(mapName, InputActionMapName::MagesGuild))
	{
		uiManager.addInputActionListener(InputActionName::MagesGuildBuy, GameWorldUI::onMagesGuildBuyInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::MagesGuildDetectMagic, GameWorldUI::onMagesGuildDetectMagicInputAction, contextName, inputManager);

		if (charClassDef.castsMagic)
		{
			uiManager.addInputActionListener(InputActionName::MagesGuildSpellmaker, GameWorldUI::onMagesGuildSpellmakerInputAction, contextName, inputManager);
		}
		
		uiManager.addInputActionListener(InputActionName::MagesGuildSteal, GameWorldUI::onMagesGuildStealInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::MagesGuildExit, GameWorldUI::onMagesGuildExitInputAction, contextName, inputManager);
	}
	else if (StringView::equals(mapName, InputActionMapName::MagesGuildBuy))
	{
		uiManager.addInputActionListener(InputActionName::MagesGuildBuyPotions, GameWorldUI::onMagesGuildBuyPotionsInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::MagesGuildBuyMagicItems, GameWorldUI::onMagesGuildBuyMagicItemsInputAction, contextName, inputManager);

		if (charClassDef.castsMagic)
		{
			uiManager.addInputActionListener(InputActionName::MagesGuildBuySpells, GameWorldUI::onMagesGuildBuySpellsInputAction, contextName, inputManager);
		}
	}
	else if (StringView::equals(mapName, InputActionMapName::MagesGuildSteal))
	{
		uiManager.addInputActionListener(InputActionName::MagesGuildStealPotions, GameWorldUI::onMagesGuildStealPotionsInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::MagesGuildStealMagicItems, GameWorldUI::onMagesGuildStealMagicItemsInputAction, contextName, inputManager);
	}
	else if (StringView::equals(mapName, InputActionMapName::NpcGeneral))
	{
		uiManager.addInputActionListener(InputActionName::NpcWhoAreYou, GameWorldUI::onNpcWhoAreYouInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::NpcWhereIs, GameWorldUI::onNpcWhereIsInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::NpcRumors, GameWorldUI::onNpcRumorsInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::NpcExit, GameWorldUI::onNpcExitInputAction, contextName, inputManager);
	}
	else if (StringView::equals(mapName, InputActionMapName::NpcRumors))
	{
		uiManager.addInputActionListener(InputActionName::NpcRumorsGeneral, GameWorldUI::onNpcRumorsGeneralInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::NpcRumorsWork, GameWorldUI::onNpcRumorsWorkInputAction, contextName, inputManager);
	}
	else if (StringView::equals(mapName, InputActionMapName::Tavern))
	{
		uiManager.addInputActionListener(InputActionName::TavernBuyDrinks, GameWorldUI::onTavernBuyDrinksInputAction, contextName, inputManager);

		if (!gameState.canUseTavernRentedRoomForCamping())
		{
			uiManager.addInputActionListener(InputActionName::TavernGetRoom, GameWorldUI::onTavernGetRoomInputAction, contextName, inputManager);
			uiManager.addInputActionListener(InputActionName::TavernSneakIntoRoom, GameWorldUI::onTavernSneakIntoRoomInputAction, contextName, inputManager);
		}
		
		uiManager.addInputActionListener(InputActionName::TavernRumors, GameWorldUI::onTavernRumorsInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::TavernExit, GameWorldUI::onTavernExitInputAction, contextName, inputManager);
	}
	else if (StringView::equals(mapName, InputActionMapName::TavernRumors))
	{
		uiManager.addInputActionListener(InputActionName::TavernRumorsGeneral, GameWorldUI::onTavernRumorsGeneralInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::TavernRumorsWork, GameWorldUI::onTavernRumorsWorkInputAction, contextName, inputManager);
	}
	else if (StringView::equals(mapName, InputActionMapName::Temple))
	{
		uiManager.addInputActionListener(InputActionName::TempleBless, GameWorldUI::onTempleBlessInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::TempleCure, GameWorldUI::onTempleCureInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::TempleHeal, GameWorldUI::onTempleHealInputAction, contextName, inputManager);
		uiManager.addInputActionListener(InputActionName::TempleExit, GameWorldUI::onTempleExitInputAction, contextName, inputManager);
	}
}

void GameWorldUI::showConversationMessageBox(ConversationMessageBoxType messageBoxType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	uiManager.clearContextElements(state.conversationModalContextInstID, inputManager, renderer);

	const GameState &gameState = game.gameState;
	const Player &player = game.player;
	const CharacterClassDefinition &playerCharClassDef = CharacterClassLibrary::getInstance().getDefinition(player.charClassDefID);
	const bool canCastMagic = playerCharClassDef.castsMagic;
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

	const bool isPaused = !game.shouldSimulateScene;
	if (!isPaused)
	{
		state.dialogueStartPlayerEffectsState = player.effectsState;
	}	

	std::string messageBoxTitleText;
	int messageBoxButtonCount = 0;
	constexpr int messageBoxMaxButtonCount = 5;
	UiButtonCallback messageBoxButtonCallbacks[messageBoxMaxButtonCount];
	std::string messageBoxButtonTexts[messageBoxMaxButtonCount];
	const char *inputActionMapName = nullptr;

	switch (messageBoxType)
	{
	case ConversationMessageBoxType::Citizen:
		messageBoxTitleText = exeData.services.citizenModalTitle;
		messageBoxButtonCount = 4;
		messageBoxButtonCallbacks[0] = GameWorldUI::onNpcWhoAreYouButtonSelected;
		messageBoxButtonCallbacks[1] = GameWorldUI::onNpcWhereIsButtonSelected;
		messageBoxButtonCallbacks[2] = GameWorldUI::onNpcRumorsButtonSelected;
		messageBoxButtonCallbacks[3] = GameWorldUI::onCloseConversationButtonSelected;
		messageBoxButtonTexts[0] = exeData.services.citizenModalWhoAreYou;
		messageBoxButtonTexts[1] = exeData.services.citizenModalWhereIs;
		messageBoxButtonTexts[2] = exeData.services.citizenModalRumors;
		messageBoxButtonTexts[3] = exeData.services.citizenModalExit;
		inputActionMapName = InputActionMapName::NpcGeneral;
		break;
	case ConversationMessageBoxType::CitizenRumors:
		messageBoxTitleText = exeData.services.citizenRumorsModalTitle;
		messageBoxButtonCount = 2;
		messageBoxButtonCallbacks[0] = GameWorldUI::onNpcRumorsGeneralButtonSelected;
		messageBoxButtonCallbacks[1] = GameWorldUI::onNpcRumorsWorkButtonSelected;
		messageBoxButtonTexts[0] = exeData.services.citizenRumorsModalGeneral;
		messageBoxButtonTexts[1] = exeData.services.citizenRumorsModalWork;
		inputActionMapName = InputActionMapName::NpcRumors;
		break;
	case ConversationMessageBoxType::Equipment:
		messageBoxTitleText = exeData.services.equipmentModalTitle;
		messageBoxButtonCount = 5;
		messageBoxButtonCallbacks[0] = GameWorldUI::onNpcEquipmentBuyButtonSelected;
		messageBoxButtonCallbacks[1] = GameWorldUI::onNpcEquipmentSellButtonSelected;
		messageBoxButtonCallbacks[2] = GameWorldUI::onNpcEquipmentRepairButtonSelected;
		messageBoxButtonCallbacks[3] = GameWorldUI::onNpcEquipmentStealButtonSelected;
		messageBoxButtonCallbacks[4] = GameWorldUI::onCloseConversationButtonSelected;
		messageBoxButtonTexts[0] = exeData.services.equipmentModalBuy;
		messageBoxButtonTexts[1] = exeData.services.equipmentModalSell;
		messageBoxButtonTexts[2] = exeData.services.equipmentModalRepair;
		messageBoxButtonTexts[3] = exeData.services.equipmentModalSteal;
		messageBoxButtonTexts[4] = exeData.services.equipmentModalExit;
		inputActionMapName = InputActionMapName::EquipmentStore;
		break;
	case ConversationMessageBoxType::EquipmentBuyItem:
		messageBoxTitleText = exeData.services.equipmentBuyModalTitle;
		messageBoxButtonCount = 2;
		messageBoxButtonCallbacks[0] = GameWorldUI::onNpcEquipmentBuyWeaponsButtonSelected;
		messageBoxButtonCallbacks[1] = GameWorldUI::onNpcEquipmentBuyArmorButtonSelected;
		messageBoxButtonTexts[0] = exeData.services.equipmentBuyModalWeapons;
		messageBoxButtonTexts[1] = exeData.services.equipmentBuyModalArmor;
		inputActionMapName = InputActionMapName::EquipmentStoreBuy;
		break;
	case ConversationMessageBoxType::MagesGuild:
		messageBoxTitleText = exeData.services.magesGuildModalTitle;

		if (canCastMagic)
		{
			messageBoxButtonCount = 5;
			messageBoxButtonCallbacks[0] = GameWorldUI::onNpcMagesGuildBuyButtonSelected;
			messageBoxButtonCallbacks[1] = GameWorldUI::onNpcMagesGuildDetectMagicButtonSelected;
			messageBoxButtonCallbacks[2] = GameWorldUI::onNpcMagesGuildSpellmakerButtonSelected;
			messageBoxButtonCallbacks[3] = GameWorldUI::onNpcMagesGuildStealButtonSelected;
			messageBoxButtonCallbacks[4] = GameWorldUI::onCloseConversationButtonSelected;
			messageBoxButtonTexts[0] = exeData.services.magesGuildModalBuy;
			messageBoxButtonTexts[1] = exeData.services.magesGuildModalDetectMagic;
			messageBoxButtonTexts[2] = exeData.services.magesGuildModalSpellmaker;
			messageBoxButtonTexts[3] = exeData.services.magesGuildModalSteal;
			messageBoxButtonTexts[4] = exeData.services.magesGuildModalExit;
		}
		else
		{
			messageBoxButtonCount = 4;
			messageBoxButtonCallbacks[0] = GameWorldUI::onNpcMagesGuildBuyButtonSelected;
			messageBoxButtonCallbacks[1] = GameWorldUI::onNpcMagesGuildDetectMagicButtonSelected;
			messageBoxButtonCallbacks[2] = GameWorldUI::onNpcMagesGuildStealButtonSelected;
			messageBoxButtonCallbacks[3] = GameWorldUI::onCloseConversationButtonSelected;
			messageBoxButtonTexts[0] = exeData.services.magesGuildModalBuy;
			messageBoxButtonTexts[1] = exeData.services.magesGuildModalDetectMagic;
			messageBoxButtonTexts[2] = exeData.services.magesGuildModalSteal;
			messageBoxButtonTexts[3] = exeData.services.magesGuildModalExit;
		}

		inputActionMapName = InputActionMapName::MagesGuild;

		break;
	case ConversationMessageBoxType::MagesGuildBuyItem:
		messageBoxTitleText = exeData.services.magesGuildPickItemModalTitle;

		if (canCastMagic)
		{
			messageBoxButtonCount = 3;
			messageBoxButtonCallbacks[0] = GameWorldUI::onNpcMagesGuildBuyPotionsButtonSelected;
			messageBoxButtonCallbacks[1] = GameWorldUI::onNpcMagesGuildBuyMagicItemsButtonSelected;
			messageBoxButtonCallbacks[2] = GameWorldUI::onNpcMagesGuildBuySpellsButtonSelected;
			messageBoxButtonTexts[0] = exeData.services.magesGuildPickItemModalPotions;
			messageBoxButtonTexts[1] = exeData.services.magesGuildPickItemModalMagicItems;
			messageBoxButtonTexts[2] = exeData.services.magesGuildPickItemModalSpells;
		}
		else
		{
			messageBoxButtonCount = 2;
			messageBoxButtonCallbacks[0] = GameWorldUI::onNpcMagesGuildBuyPotionsButtonSelected;
			messageBoxButtonCallbacks[1] = GameWorldUI::onNpcMagesGuildBuyMagicItemsButtonSelected;
			messageBoxButtonTexts[0] = exeData.services.magesGuildPickItemModalPotions;
			messageBoxButtonTexts[1] = exeData.services.magesGuildPickItemModalMagicItems;
		}
		
		inputActionMapName = InputActionMapName::MagesGuildBuy;
		break;
	case ConversationMessageBoxType::MagesGuildSteal:
		messageBoxTitleText = exeData.services.magesGuildPickItemModalTitle;
		messageBoxButtonCount = 2;
		messageBoxButtonCallbacks[0] = GameWorldUI::onNpcMagesGuildStealPotionsButtonSelected;
		messageBoxButtonCallbacks[1] = GameWorldUI::onNpcMagesGuildStealMagicItemsButtonSelected;
		messageBoxButtonTexts[0] = exeData.services.magesGuildPickItemModalPotions;
		messageBoxButtonTexts[1] = exeData.services.magesGuildPickItemModalMagicItems;
		inputActionMapName = InputActionMapName::MagesGuildSteal;
		break;
	case ConversationMessageBoxType::Tavern:
	{
		messageBoxTitleText = exeData.services.tavernModalTitle;

		if (gameState.canUseTavernRentedRoomForCamping())
		{
			messageBoxButtonCount = 3;
			messageBoxButtonCallbacks[0] = GameWorldUI::onNpcTavernBuyDrinksButtonSelected;
			messageBoxButtonCallbacks[1] = GameWorldUI::onNpcTavernRumorsButtonSelected;
			messageBoxButtonCallbacks[2] = GameWorldUI::onCloseConversationButtonSelected;
			messageBoxButtonTexts[0] = exeData.services.tavernModalBuyDrinks;
			messageBoxButtonTexts[1] = exeData.services.tavernModalRumors;
			messageBoxButtonTexts[2] = exeData.services.tavernModalExit;
		}
		else
		{
			messageBoxButtonCount = 5;
			messageBoxButtonCallbacks[0] = GameWorldUI::onNpcTavernBuyDrinksButtonSelected;
			messageBoxButtonCallbacks[1] = GameWorldUI::onNpcTavernGetARoomButtonSelected;
			messageBoxButtonCallbacks[2] = GameWorldUI::onNpcTavernSneakIntoARoomButtonSelected;
			messageBoxButtonCallbacks[3] = GameWorldUI::onNpcTavernRumorsButtonSelected;
			messageBoxButtonCallbacks[4] = GameWorldUI::onCloseConversationButtonSelected;
			messageBoxButtonTexts[0] = exeData.services.tavernModalBuyDrinks;
			messageBoxButtonTexts[1] = exeData.services.tavernModalGetARoom;
			messageBoxButtonTexts[2] = exeData.services.tavernModalSneakIntoARoom;
			messageBoxButtonTexts[3] = exeData.services.tavernModalRumors;
			messageBoxButtonTexts[4] = exeData.services.tavernModalExit;
		}

		inputActionMapName = InputActionMapName::Tavern;

		break;
	}
	case ConversationMessageBoxType::TavernRumors:
		messageBoxTitleText = exeData.services.citizenRumorsModalTitle;
		messageBoxButtonCount = 2;
		messageBoxButtonCallbacks[0] = GameWorldUI::onNpcTavernRumorsGeneralButtonSelected;
		messageBoxButtonCallbacks[1] = GameWorldUI::onNpcTavernRumorsWorkButtonSelected;
		messageBoxButtonTexts[0] = exeData.services.citizenRumorsModalGeneral;
		messageBoxButtonTexts[1] = exeData.services.citizenRumorsModalWork;
		inputActionMapName = InputActionMapName::TavernRumors;
		break;
	case ConversationMessageBoxType::Temple:
		messageBoxTitleText = exeData.services.templeModalTitle;
		messageBoxButtonCount = 4;
		messageBoxButtonCallbacks[0] = GameWorldUI::onNpcTempleBlessButtonSelected;
		messageBoxButtonCallbacks[1] = GameWorldUI::onNpcTempleCureButtonSelected;
		messageBoxButtonCallbacks[2] = GameWorldUI::onNpcTempleHealButtonSelected;
		messageBoxButtonCallbacks[3] = GameWorldUI::onCloseConversationButtonSelected;
		messageBoxButtonTexts[0] = exeData.services.templeModalBless;
		messageBoxButtonTexts[1] = exeData.services.templeModalCure;
		messageBoxButtonTexts[2] = exeData.services.templeModalHeal;
		messageBoxButtonTexts[3] = exeData.services.templeModalExit;
		inputActionMapName = InputActionMapName::Temple;
		break;
	default:
		DebugNotImplemented();
		break;
	}

	const int titleImageTextureHeight = 24;
	const int totalMessageBoxHeight = titleImageTextureHeight * (messageBoxButtonCount + 1);
	const int titleTextBoxPositionY = (ArenaRenderUtils::SCREEN_HEIGHT / 2) - (totalMessageBoxHeight / 2) + (titleImageTextureHeight / 2);
	const PaletteID paletteID = *textureManager.tryGetPaletteID(GameWorldUiView::getPaletteTextureAsset());

	UiElementInitInfo titleTextBoxElementInitInfo;
	titleTextBoxElementInitInfo.name = "GameWorldConversationModalTitleTextBox";
	titleTextBoxElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, titleTextBoxPositionY);
	titleTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	titleTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo titleTextBoxInitInfo;
	titleTextBoxInitInfo.text = messageBoxTitleText;
	titleTextBoxInitInfo.fontName = ArenaFontName::A;
	titleTextBoxInitInfo.tabColorPaletteID = paletteID;
	titleTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
	const UiElementInstanceID titleTextBoxElementInstID = uiManager.createTextBox(titleTextBoxElementInitInfo, titleTextBoxInitInfo, state.conversationModalContextInstID, renderer);
	const Rect titleTextBoxGlobalRect = uiManager.getTransformGlobalRect(titleTextBoxElementInstID);

	int widestButtonTextBoxWidth = titleTextBoxGlobalRect.width;
	for (int i = 0; i < messageBoxButtonCount; i++)
	{
		UiElementInitInfo messageBoxButtonTextBoxElementInitInfo;
		messageBoxButtonTextBoxElementInitInfo.name = "GameWorldConversationModalButton" + std::to_string(i) + "TextBox";
		messageBoxButtonTextBoxElementInitInfo.position = titleTextBoxElementInitInfo.position + Int2(0, titleImageTextureHeight * (i + 1));
		messageBoxButtonTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
		messageBoxButtonTextBoxElementInitInfo.drawOrder = 1;

		UiTextBoxInitInfo messageBoxButtonTextBoxInitInfo;
		messageBoxButtonTextBoxInitInfo.text = messageBoxButtonTexts[i];
		messageBoxButtonTextBoxInitInfo.fontName = ArenaFontName::A;
		messageBoxButtonTextBoxInitInfo.tabColorPaletteID = paletteID;
		messageBoxButtonTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
		const UiElementInstanceID messageBoxButtonTextBoxElementInstID = uiManager.createTextBox(messageBoxButtonTextBoxElementInitInfo, messageBoxButtonTextBoxInitInfo, state.conversationModalContextInstID, renderer);
		const Rect messageBoxButtonTextBoxGlobalRect = uiManager.getTransformGlobalRect(messageBoxButtonTextBoxElementInstID);
		widestButtonTextBoxWidth = std::max(widestButtonTextBoxWidth, messageBoxButtonTextBoxGlobalRect.width);
	}

	UiElementInitInfo titleImageElementInitInfo;
	titleImageElementInitInfo.name = "GameWorldConversationModalTitleImage";
	titleImageElementInitInfo.position = titleTextBoxElementInitInfo.position;
	titleImageElementInitInfo.sizeType = UiTransformSizeType::Manual;
	titleImageElementInitInfo.size = Int2(widestButtonTextBoxWidth + 14, titleImageTextureHeight);
	titleImageElementInitInfo.pivotType = UiPivotType::Middle;

	const UiTextureID titleImageTextureID = uiManager.getOrAddTexture(UiTexturePatternType::Dark, titleImageElementInitInfo.size.x, titleImageElementInitInfo.size.y, textureManager, renderer);
	uiManager.createImage(titleImageElementInitInfo, titleImageTextureID, state.conversationModalContextInstID, renderer);

	for (int i = 0; i < messageBoxButtonCount; i++)
	{
		UiElementInitInfo messageBoxButtonImageElementInitInfo;
		messageBoxButtonImageElementInitInfo.name = "GameWorldConversationModalButton" + std::to_string(i) + "Image";
		messageBoxButtonImageElementInitInfo.position = titleTextBoxElementInitInfo.position + Int2(0, titleImageTextureHeight * (i + 1));
		messageBoxButtonImageElementInitInfo.sizeType = UiTransformSizeType::Manual;
		messageBoxButtonImageElementInitInfo.size = titleImageElementInitInfo.size;
		messageBoxButtonImageElementInitInfo.pivotType = UiPivotType::Middle;
		uiManager.createImage(messageBoxButtonImageElementInitInfo, titleImageTextureID, state.conversationModalContextInstID, renderer);

		UiElementInitInfo messageBoxButtonElementInitInfo;
		messageBoxButtonElementInitInfo.name = "GameWorldConversationModalButton" + std::to_string(i);
		messageBoxButtonElementInitInfo.position = messageBoxButtonImageElementInitInfo.position;
		messageBoxButtonElementInitInfo.pivotType = UiPivotType::Middle;

		UiButtonInitInfo messageBoxButtonInitInfo;
		messageBoxButtonInitInfo.callback = messageBoxButtonCallbacks[i];
		messageBoxButtonInitInfo.contentElementName = messageBoxButtonImageElementInitInfo.name;
		uiManager.createButton(messageBoxButtonElementInitInfo, messageBoxButtonInitInfo, state.conversationModalContextInstID);
	}

	UiElementInitInfo backButtonElementInitInfo;
	backButtonElementInitInfo.name = "GameWorldConversationModalBackButton";
	backButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	backButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);

	UiButtonInitInfo backButtonInitInfo;
	backButtonInitInfo.mouseButtonFlags = MouseButtonTypeFlags(MouseButtonType::Right);
	backButtonInitInfo.callback = GameWorldUI::onCloseConversationButtonSelected;
	uiManager.createButton(backButtonElementInitInfo, backButtonInitInfo, state.conversationModalContextInstID);

	auto backInputActionCallback = [](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			GameWorldUI::onCloseConversationButtonSelected(MouseButtonType::Right);
		}
	};

	uiManager.addInputActionListener(InputActionName::Back, backInputActionCallback, ContextName_ConversationModal, inputManager);

	GameWorldUI::setShopkeeperPlayerGoldVisible(false);
	GameWorldUI::setConversationMessageBoxInputActionMapActive(inputActionMapName);

	if (!String::isNullOrEmpty(inputActionMapName))
	{
		GameWorldUI::addConversationMessageBoxInputActionListeners(inputActionMapName);
	}

	uiManager.setContextEnabled(state.conversationModalContextInstID, true);

	if (!isPaused)
	{
		GameWorldUI::onPauseChanged(true);
	}
}

void GameWorldUI::showConversationListBox(ConversationListBoxType listBoxType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	DialogueManager &dialogueManager = game.dialogueManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	uiManager.clearContextElements(state.conversationModalContextInstID, inputManager, renderer);

	const GameState &gameState = game.gameState;
	const MapType mapType = gameState.getActiveMapType();
	const Player &player = game.player;
	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const ArenaTypes::Spellsg &standardSpells = binaryAssetLibrary.getStandardSpells();
	const ExeData &exeData = binaryAssetLibrary.getExeData();
	const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
	const VoxelChunkManager &voxelChunkManager = game.sceneManager.voxelChunkManager;

	const GameWorldPopUpClosedCallback returnToCitizenMessageBoxCallback = GameWorldUI::makeReturnToMessageBoxCallback(ConversationMessageBoxType::Citizen);
	const GameWorldPopUpClosedCallback returnToEquipmentStoreMessageBoxCallback = GameWorldUI::makeReturnToMessageBoxCallback(ConversationMessageBoxType::Equipment);
	const GameWorldPopUpClosedCallback returnToMagesGuildMessageBoxCallback = GameWorldUI::makeReturnToMessageBoxCallback(ConversationMessageBoxType::MagesGuild);
	const GameWorldPopUpClosedCallback returnToTavernMessageBoxCallback = GameWorldUI::makeReturnToMessageBoxCallback(ConversationMessageBoxType::Tavern);
	const GameWorldPopUpClosedCallback returnToTempleMessageBoxCallback = GameWorldUI::makeReturnToMessageBoxCallback(ConversationMessageBoxType::Temple);

	constexpr int sceneViewCenterY = ArenaRenderUtils::SCENE_VIEW_HEIGHT / 2;
	constexpr int barterViewCenterY = 122 / 2;

	int listBoxImageY = barterViewCenterY;
	UiPivotType listBoxImagePivotType = UiPivotType::Middle;
	std::string listBoxTextureName;
	Int2 listBoxPositionOffset;
	std::string listBoxFontName;
	int listBoxTextureWidth = 0;
	int listBoxTextureHeight = 0;
	std::vector<int> listBoxColumnPixelXOffsets = { 0 };
	Int2 listBoxButtonUpPositionOffset;
	Int2 listBoxButtonDownPositionOffset;
	std::vector<std::vector<std::string>> listBoxItemTextColumns;
	std::vector<UiListBoxItemCallback> listBoxItemCallbacks;
	bool shouldShowPlayerGold = true;

	switch (listBoxType)
	{
	case ConversationListBoxType::CitizenWhereIs:
	{
		listBoxImageY = 0;
		listBoxImagePivotType = UiPivotType::Top;
		listBoxTextureName = ArenaTextureName::PopUp11;
		listBoxPositionOffset = Int2(28, 20);
		listBoxFontName = ArenaFontName::Arena;
		listBoxTextureWidth = 185;
		listBoxTextureHeight = 81;
		listBoxButtonUpPositionOffset = Int2(9, 7);
		listBoxButtonDownPositionOffset = Int2(9, 102);
		shouldShowPlayerGold = false;

		const double ceilingScale = gameState.getActiveCeilingScale();
		const WorldInt3 playerWorldVoxel = VoxelUtils::pointToVoxel(player.getEyePosition(), ceilingScale);

		if (state.dialogueWhereIsDetailEntries.empty())
		{
			std::vector<DialogueDirectionsEntry> directionsEntries;
			if (mapType == MapType::City)
			{
				directionsEntries = dialogueManager.cityDirectionsEntries;
			}
			else if (mapType == MapType::Wilderness)
			{
				directionsEntries = dialogueManager.wildernessDirectionsEntries;
			}

			for (int i = 0; i < static_cast<int>(directionsEntries.size()); i++)
			{
				const DialogueDirectionsEntry &directionsEntry = directionsEntries[i];
				listBoxItemTextColumns.emplace_back(std::vector<std::string> { directionsEntry.displayString });
				listBoxItemCallbacks.emplace_back(GameWorldUI::makeDirectionsEntryCallback(directionsEntry));
			}
		}
		else
		{
			for (const DialogueDirectionsDetailEntry &detailEntry : state.dialogueWhereIsDetailEntries)
			{
				listBoxItemTextColumns.emplace_back(std::vector<std::string> { detailEntry.buildingName });
				listBoxItemCallbacks.emplace_back(GameWorldUI::makeDirectionsDetailEntryCallback(detailEntry));
			}

			state.dialogueWhereIsDetailEntries.clear();
		}

		break;
	}
	case ConversationListBoxType::EquipmentWeapons:
	{
		listBoxTextureName = ArenaTextureName::PopUp3;
		listBoxPositionOffset = Int2(30, 28);
		listBoxFontName = ArenaFontName::Teeny;
		listBoxTextureWidth = 275;
		listBoxTextureHeight = 72;
		listBoxColumnPixelXOffsets = { 0, 170, 197, 218, 253 };
		listBoxButtonUpPositionOffset = Int2(9, 9);
		listBoxButtonDownPositionOffset = Int2(9, 104);

		const std::vector<ItemDefinitionID> sourceItemDefIDs = itemLibrary.getDefinitionIDsIf(
			[](const ItemDefinition &itemDef)
		{
			return itemDef.type == ItemType::Weapon;
		});

		for (const ItemDefinitionID sourceItemDefID : sourceItemDefIDs)
		{
			const ItemDefinition &sourceItemDef = itemLibrary.getDefinition(sourceItemDefID);
			const WeaponItemDefinition &sourceWeaponDef = sourceItemDef.weapon;
			const std::string weaponDisplayName = sourceItemDef.getDisplayNameWithoutQty();
			const int weaponHandedness = sourceWeaponDef.handCount;
			const double weaponWeightKgs = sourceItemDef.getWeight();
			const int weaponPrice = sourceItemDef.getGoldValue();

			const std::string weaponHandednessString = String::format("%-8d", weaponHandedness);
			const std::string weaponWeightString = String::format("%-.1f", weaponWeightKgs);
			const std::string weaponPriceString = String::format("%10d", weaponPrice);
			const std::string weaponPriceUnitString = "gp";
			std::vector<std::string> weaponTextColumns = { weaponDisplayName, weaponHandednessString, weaponWeightString, weaponPriceString, weaponPriceUnitString };
			listBoxItemTextColumns.emplace_back(std::move(weaponTextColumns));

			listBoxItemCallbacks.emplace_back(GameWorldUI::makeShopkeeperItemPurchaseCallback(sourceItemDefID, weaponPrice, ConversationMessageBoxType::Equipment));
		}

		break;
	}
	case ConversationListBoxType::EquipmentArmor:
	{
		listBoxTextureName = ArenaTextureName::PopUp4;
		listBoxPositionOffset = Int2(30, 28);
		listBoxFontName = ArenaFontName::Teeny;
		listBoxTextureWidth = 275;
		listBoxTextureHeight = 72;
		listBoxColumnPixelXOffsets = { 0, 154, 203, 218, 253 };
		listBoxButtonUpPositionOffset = Int2(9, 9);
		listBoxButtonDownPositionOffset = Int2(9, 104);

		const std::vector<ItemDefinitionID> sourceItemDefIDs = itemLibrary.getDefinitionIDsIf(
			[](const ItemDefinition &itemDef)
		{
			return (itemDef.type == ItemType::Armor) || (itemDef.type == ItemType::Shield);
		});

		for (const ItemDefinitionID sourceItemDefID : sourceItemDefIDs)
		{
			const ItemDefinition &sourceItemDef = itemLibrary.getDefinition(sourceItemDefID);
			const ArmorItemDefinition &sourceArmorDef = sourceItemDef.armor;
			const std::string armorDisplayName = sourceItemDef.getDisplayNameWithoutQty();
			const double armorWeightKgs = sourceItemDef.getWeight();
			const int armorPrice = sourceItemDef.getGoldValue();

			const std::string armorProtectedSlotString = "TODO";
			const std::string armorWeightString = String::format("%-.1f", armorWeightKgs);
			const std::string armorPriceString = String::format("%10d", armorPrice);
			const std::string armorPriceUnitString = "gp";
			std::vector<std::string> armorTextColumns = { armorDisplayName, armorProtectedSlotString, armorWeightString, armorPriceString, armorPriceUnitString };
			listBoxItemTextColumns.emplace_back(std::move(armorTextColumns));

			listBoxItemCallbacks.emplace_back(GameWorldUI::makeShopkeeperItemPurchaseCallback(sourceItemDefID, armorPrice, ConversationMessageBoxType::Equipment));
		}

		break;
	}
	case ConversationListBoxType::EquipmentSell:
	{
		listBoxTextureName = ArenaTextureName::ContainerInventory;
		listBoxPositionOffset = Int2(29, 24);
		listBoxFontName = ArenaFontName::Teeny;
		listBoxTextureWidth = 155;
		listBoxTextureHeight = 56;
		listBoxButtonUpPositionOffset = Int2(9, 9);
		listBoxButtonDownPositionOffset = Int2(9, 82);

		std::vector<int> playerInventorySlotIndices;
		for (int i = 0; i < player.inventory.getTotalSlotCount(); i++)
		{
			const ItemInstance &itemInst = player.inventory.getSlot(i);
			if (itemInst.isValid())
			{
				playerInventorySlotIndices.emplace_back(i);
			}
		}

		for (const int inventorySlotIndex : playerInventorySlotIndices)
		{
			const ItemInstance &sourceItemInst = player.inventory.getSlot(inventorySlotIndex);
			const ItemDefinition &sourceItemDef = itemLibrary.getDefinition(sourceItemInst.defID);
			listBoxItemTextColumns.emplace_back(std::vector<std::string> { sourceItemDef.getDisplayNameWithoutQty() });
			listBoxItemCallbacks.emplace_back(GameWorldUI::makeShopkeeperItemSellCallback(inventorySlotIndex));
		}

		break;
	}
	case ConversationListBoxType::EquipmentRepair:
	{
		listBoxTextureName = ArenaTextureName::ContainerInventory;
		listBoxPositionOffset = Int2(29, 24);
		listBoxFontName = ArenaFontName::Teeny;
		listBoxTextureWidth = 155;
		listBoxTextureHeight = 56;
		listBoxButtonUpPositionOffset = Int2(9, 9);
		listBoxButtonDownPositionOffset = Int2(9, 82);

		std::vector<int> playerInventorySlotIndices;
		for (int i = 0; i < player.inventory.getTotalSlotCount(); i++)
		{
			const ItemInstance &itemInst = player.inventory.getSlot(i);
			if (itemInst.isValid())
			{
				const ItemDefinition &itemDef = itemLibrary.getDefinition(itemInst.defID);
				const bool isValidItemForRepair = ItemTypeFlags(itemDef.type).any(ItemType::Weapon | ItemType::Armor | ItemType::Shield);
				if (isValidItemForRepair)
				{
					playerInventorySlotIndices.emplace_back(i);
				}
			}
		}

		for (const int inventorySlotIndex : playerInventorySlotIndices)
		{
			const ItemInstance &sourceItemInst = player.inventory.getSlot(inventorySlotIndex);
			const ItemDefinition &sourceItemDef = itemLibrary.getDefinition(sourceItemInst.defID);
			listBoxItemTextColumns.emplace_back(std::vector<std::string> { sourceItemDef.getDisplayNameWithoutQty() });
			listBoxItemCallbacks.emplace_back(GameWorldUI::makeShopkeeperItemRepairCallback(inventorySlotIndex));
		}

		break;
	}
	case ConversationListBoxType::MagesGuildPotions:
	{
		listBoxTextureName = ArenaTextureName::ContainerInventory;
		listBoxPositionOffset = Int2(29, 24);
		listBoxFontName = ArenaFontName::Teeny;
		listBoxTextureWidth = 150;
		listBoxTextureHeight = 56;
		listBoxColumnPixelXOffsets = { 0, 123 };
		listBoxButtonUpPositionOffset = Int2(9, 9);
		listBoxButtonDownPositionOffset = Int2(9, 82);

		const std::vector<ItemDefinitionID> sourceItemDefIDs = itemLibrary.getDefinitionIDsIf(
			[](const ItemDefinition &itemDef)
		{
			return itemDef.type == ItemType::Consumable;
		});

		for (const ItemDefinitionID sourceItemDefID : sourceItemDefIDs)
		{
			const ItemDefinition &sourceItemDef = itemLibrary.getDefinition(sourceItemDefID);
			const std::string consumableDisplayName = String::format("%.28s", sourceItemDef.getDisplayNameWithoutQty().c_str());
			const int consumableGoldPrice = sourceItemDef.getGoldValue();
			const std::string consumableGoldPriceString = String::format("%d gp", consumableGoldPrice);
			listBoxItemTextColumns.emplace_back(std::vector<std::string> { consumableDisplayName, consumableGoldPriceString });
			listBoxItemCallbacks.emplace_back(GameWorldUI::makeShopkeeperItemPurchaseCallback(sourceItemDefID, consumableGoldPrice, ConversationMessageBoxType::MagesGuild));
		}

		break;
	}
	case ConversationListBoxType::MagesGuildMagicItems:
	{
		listBoxTextureName = ArenaTextureName::PopUp7;
		listBoxPositionOffset = Int2(30, 28);
		listBoxFontName = ArenaFontName::Teeny;
		listBoxTextureWidth = 270;
		listBoxTextureHeight = 72;
		listBoxColumnPixelXOffsets = { 0, 180 };
		listBoxButtonUpPositionOffset = Int2(9, 9);
		listBoxButtonDownPositionOffset = Int2(9, 104);

		const std::vector<ItemDefinitionID> sourceItemDefIDs = itemLibrary.getDefinitionIDsIf(
			[](const ItemDefinition &itemDef)
		{
			return (itemDef.type == ItemType::Accessory) || (itemDef.type == ItemType::Trinket);
		});

		for (const ItemDefinitionID sourceItemDefID : sourceItemDefIDs)
		{
			const ItemDefinition &sourceItemDef = itemLibrary.getDefinition(sourceItemDefID);
			const std::string sourceItemDisplayName = String::format("%.30s", sourceItemDef.getDisplayNameWithoutQty().c_str());
			const int sourceItemGoldPrice = sourceItemDef.getGoldValue();

			const std::string sourceItemGoldPriceString = String::format("%d gp", sourceItemGoldPrice);
			listBoxItemTextColumns.emplace_back(std::vector<std::string> { sourceItemDisplayName, sourceItemGoldPriceString });
			listBoxItemCallbacks.emplace_back(GameWorldUI::makeShopkeeperItemPurchaseCallback(sourceItemDefID, sourceItemGoldPrice, ConversationMessageBoxType::MagesGuild));
		}

		break;
	}
	case ConversationListBoxType::MagesGuildSpells:
	{
		listBoxTextureName = ArenaTextureName::ContainerInventory;
		listBoxPositionOffset = Int2(29, 24);
		listBoxFontName = ArenaFontName::Teeny;
		listBoxTextureWidth = 150;
		listBoxTextureHeight = 56;
		listBoxButtonUpPositionOffset = Int2(9, 9);
		listBoxButtonDownPositionOffset = Int2(9, 82);

		std::vector<std::string> spellNames;
		for (const ArenaTypes::SpellData &spellData : standardSpells)
		{
			std::string spellName(spellData.name.data());
			if (!spellName.empty())
			{
				spellNames.emplace_back(std::move(spellName));
			}
		}

		for (int i = 0; i < static_cast<int>(spellNames.size()); i++)
		{
			const std::string &spellName = spellNames[i];
			listBoxItemTextColumns.emplace_back(std::vector<std::string> { spellName });
			listBoxItemCallbacks.emplace_back(GameWorldUI::makeMagesGuildSpellPurchaseCallback(spellName));
		}

		break;
	}
	case ConversationListBoxType::TavernDrinks:
	{
		listBoxTextureName = ArenaTextureName::PopUp5;
		listBoxPositionOffset = Int2(29, 24);
		listBoxFontName = ArenaFontName::Arena;
		listBoxTextureWidth = 155;
		listBoxTextureHeight = 54;
		listBoxColumnPixelXOffsets = { 0, 120 };
		listBoxButtonUpPositionOffset = Int2(9, 9);
		listBoxButtonDownPositionOffset = Int2(9, 82);

		Span<const std::string> drinkNames = exeData.services.tavernDrinks;
		Span<const uint8_t> drinkGoldPrices = exeData.services.tavernDrinkGoldPrices;

		const LocationDefinition &locationDef = gameState.getLocationDefinition();
		const LocationCityDefinition &locationCityDef = locationDef.getCityDefinition();
		ArenaRandom tempRandom(locationCityDef.citySeed); // @todo use interior seed from the entrance XY

		constexpr int maxAvailableDrinkCount = 10;
		std::vector<int> availableDrinkIndices(drinkNames.getCount());
		std::iota(availableDrinkIndices.begin(), availableDrinkIndices.end(), 0);
		RandomUtils::shuffle<int>(availableDrinkIndices, tempRandom);
		availableDrinkIndices.resize(maxAvailableDrinkCount);

		for (int i = 0; i < static_cast<int>(availableDrinkIndices.size()); i++)
		{
			const int drinkIndex = availableDrinkIndices[i];
			const std::string &drinkName = drinkNames[drinkIndex];
			const int drinkGoldPrice = drinkGoldPrices[drinkIndex];
			const std::string drinkNameTruncated = String::format("%.22s", drinkName.c_str());
			const std::string drinkGoldPriceString = String::format("%d gp", drinkGoldPrice);
			listBoxItemTextColumns.emplace_back(std::vector<std::string> { drinkNameTruncated, drinkGoldPriceString });
			listBoxItemCallbacks.emplace_back(GameWorldUI::makeTavernDrinkPurchaseCallback(drinkName, drinkGoldPrice));
		}

		break;
	}
	case ConversationListBoxType::TavernRooms:
	{
		listBoxTextureName = ArenaTextureName::ContainerInventory;
		listBoxPositionOffset = Int2(29, 36);
		listBoxFontName = ArenaFontName::A;
		listBoxTextureWidth = 170;
		listBoxTextureHeight = 44;
		listBoxColumnPixelXOffsets = { 0, 112 };
		listBoxButtonUpPositionOffset = Int2(9, 9);
		listBoxButtonDownPositionOffset = Int2(9, 82);

		Span<const std::string> roomTypeNames = exeData.services.tavernRoomTypes;
		constexpr int roomGoldPricesPerDay[] = { 10, 20, 35, 50, 75 }; // @todo get from ExeData

		for (int i = 0; i < roomTypeNames.getCount(); i++)
		{
			const int roomType = i;
			const std::string roomTypeName = String::format("%.13s", roomTypeNames[i].c_str());
			const int baseRoomGoldPrice = roomGoldPricesPerDay[i];
			const int roomRentDayCount = 1;
			const int calculatedRoomGoldPrice = baseRoomGoldPrice * roomRentDayCount;
			const std::string roomGoldPriceString = String::format("%d gp", calculatedRoomGoldPrice);
			listBoxItemTextColumns.emplace_back(std::vector<std::string> { roomTypeName, roomGoldPriceString });
			listBoxItemCallbacks.emplace_back(GameWorldUI::makeTavernRoomPurchaseCallback(roomType, calculatedRoomGoldPrice));
		}

		break;
	}
	case ConversationListBoxType::TempleCuring:
	{
		listBoxTextureName = ArenaTextureName::ContainerInventory;
		listBoxPositionOffset = Int2(29, 24);
		listBoxFontName = ArenaFontName::Arena;
		listBoxTextureWidth = 155;
		listBoxTextureHeight = 65;
		listBoxButtonUpPositionOffset = Int2(9, 9);
		listBoxButtonDownPositionOffset = Int2(9, 82);

		if (player.effectsState.isDiseased())
		{
			listBoxItemTextColumns.emplace_back(std::vector<std::string> { exeData.status.effectNames[0] });
			listBoxItemCallbacks.emplace_back(GameWorldUI::makeTempleCurePurchaseCallback());
		}

		break;
	}
	default:
		DebugNotImplemented();
		break;
	}

	const TextureAsset listBoxTextureAsset(listBoxTextureName);
	const TextureAsset listBoxPaletteAsset = GameWorldUiView::getPaletteTextureAsset();
	const UiTextureID listBoxTextureID = uiManager.getOrAddTexture(listBoxTextureAsset, listBoxPaletteAsset, textureManager, renderer);

	UiElementInitInfo listBoxImageElementInitInfo;
	listBoxImageElementInitInfo.name = "GameWorldConversationModalListBoxImage";
	listBoxImageElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, listBoxImageY);
	listBoxImageElementInitInfo.pivotType = listBoxImagePivotType;
	const UiElementInstanceID listBoxImageElementInstID = uiManager.createImage(listBoxImageElementInitInfo, listBoxTextureID, state.conversationModalContextInstID, renderer);
	const Rect listBoxImageGlobalRect = uiManager.getTransformGlobalRect(listBoxImageElementInstID);

	UiElementInitInfo listBoxElementInitInfo;
	listBoxElementInitInfo.name = ConversationModalListBoxElementName;
	listBoxElementInitInfo.position = listBoxImageGlobalRect.getTopLeft() + listBoxPositionOffset;
	listBoxElementInitInfo.pivotType = UiPivotType::TopLeft;
	listBoxElementInitInfo.drawOrder = 1;

	UiListBoxInitInfo listBoxInitInfo;
	listBoxInitInfo.textureWidth = listBoxTextureWidth;
	listBoxInitInfo.textureHeight = listBoxTextureHeight;
	listBoxInitInfo.columnPixelXOffsets = listBoxColumnPixelXOffsets;
	listBoxInitInfo.itemPixelSpacing = 0;
	listBoxInitInfo.fontName = listBoxFontName;
	listBoxInitInfo.defaultTextColor = Color(190, 113, 0);
	const UiElementInstanceID listBoxElementInstID = uiManager.createListBox(listBoxElementInitInfo, listBoxInitInfo, state.conversationModalContextInstID, renderer);

	for (int i = 0; i < static_cast<int>(listBoxItemTextColumns.size()); i++)
	{
		UiListBoxItem listBoxItem;
		listBoxItem.textColumns = listBoxItemTextColumns[i];
		listBoxItem.callback = listBoxItemCallbacks[i];
		uiManager.insertBackListBoxItem(listBoxElementInstID, std::move(listBoxItem));
	}

	UiButtonCallback listBoxUpButtonCallback = [&uiManager, listBoxElementInstID](MouseButtonType)
	{
		uiManager.scrollListBoxUp(listBoxElementInstID);
	};

	UiButtonCallback listBoxDownButtonCallback = [&uiManager, listBoxElementInstID](MouseButtonType)
	{
		uiManager.scrollListBoxDown(listBoxElementInstID);
	};

	UiElementInitInfo listBoxUpButtonElementInitInfo;
	listBoxUpButtonElementInitInfo.name = "GameWorldConversationModalListBoxUpButton";
	listBoxUpButtonElementInitInfo.position = listBoxImageGlobalRect.getTopLeft() + listBoxButtonUpPositionOffset;
	listBoxUpButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	listBoxUpButtonElementInitInfo.size = Int2(9, 9);

	UiButtonInitInfo listBoxUpButtonInitInfo;
	listBoxUpButtonInitInfo.callback = listBoxUpButtonCallback;
	uiManager.createButton(listBoxUpButtonElementInitInfo, listBoxUpButtonInitInfo, state.conversationModalContextInstID);

	UiElementInitInfo listBoxDownButtonElementInitInfo;
	listBoxDownButtonElementInitInfo.name = "GameWorldConversationModalListBoxDownButton";
	listBoxDownButtonElementInitInfo.position = listBoxImageGlobalRect.getTopLeft() + listBoxButtonDownPositionOffset;
	listBoxDownButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	listBoxDownButtonElementInitInfo.size = listBoxUpButtonElementInitInfo.size;

	UiButtonInitInfo listBoxDownButtonInitInfo;
	listBoxDownButtonInitInfo.callback = listBoxDownButtonCallback;
	uiManager.createButton(listBoxDownButtonElementInitInfo, listBoxDownButtonInitInfo, state.conversationModalContextInstID);

	UiElementInitInfo backButtonElementInitInfo;
	backButtonElementInitInfo.name = "GameWorldConversationModalBackButton";
	backButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	backButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);

	UiButtonInitInfo backButtonInitInfo;
	backButtonInitInfo.mouseButtonFlags = MouseButtonTypeFlags(MouseButtonType::Right);
	backButtonInitInfo.callback = GameWorldUI::onCloseConversationButtonSelected;
	uiManager.createButton(backButtonElementInitInfo, backButtonInitInfo, state.conversationModalContextInstID);

	InputActionCallback backInputActionCallback = [](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			GameWorldUI::onCloseConversationButtonSelected(MouseButtonType::Right);
		}
	};

	uiManager.addInputActionListener(InputActionName::Back, backInputActionCallback, ContextName_ConversationModal, inputManager);

	MouseScrollChangedCallback mouseScrollChangedListener = [&uiManager, listBoxElementInstID](Game &game, MouseWheelScrollType type, const Int2 &position)
	{
		if (type == MouseWheelScrollType::Down)
		{
			uiManager.scrollListBoxDown(listBoxElementInstID);
		}
		else if (type == MouseWheelScrollType::Up)
		{
			uiManager.scrollListBoxUp(listBoxElementInstID);
		}
	};

	uiManager.addMouseScrollChangedListener(mouseScrollChangedListener, ContextName_ConversationModal, inputManager);

	GameWorldUI::setShopkeeperPlayerGoldVisible(shouldShowPlayerGold);

	uiManager.setContextEnabled(state.conversationModalContextInstID, true);
}

void GameWorldUI::showShopkeeperBackground(const char *titleText)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	InputManager &inputManager = game.inputManager;
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	uiManager.clearContextElements(state.shopkeeperBgContextInstID, inputManager, renderer);

	const bool shouldUseSmallerFont = std::strlen(titleText) >= 32;
	std::string fontName = ArenaFontName::C;
	if (shouldUseSmallerFont)
	{
		fontName = ArenaFontName::Arena;
	}

	UiElementInitInfo titleTextBoxElementInitInfo;
	titleTextBoxElementInitInfo.name = "GameWorldShopkeeperBackgroundTitleTextBox";
	titleTextBoxElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, 10);
	titleTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	titleTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo titleTextBoxInitInfo;
	titleTextBoxInitInfo.text = titleText;
	titleTextBoxInitInfo.fontName = fontName;
	titleTextBoxInitInfo.defaultColor = Color(12, 12, 24);
	titleTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
	const UiElementInstanceID titleTextBoxElementInstID = uiManager.createTextBox(titleTextBoxElementInitInfo, titleTextBoxInitInfo, state.shopkeeperBgContextInstID, renderer);
	const Rect titleTextBoxGlobalRect = uiManager.getTransformGlobalRect(titleTextBoxElementInstID);

	const int titleImageTextureWidth = titleTextBoxGlobalRect.width + 24;
	constexpr int titleImageTextureHeight = 20;
	const UiTextureID titleImageTextureID = uiManager.getOrAddTexture(UiTexturePatternType::ShopkeeperTitle, titleImageTextureWidth, titleImageTextureHeight, textureManager, renderer);

	UiElementInitInfo titleImageElementInitInfo;
	titleImageElementInitInfo.name = "GameWorldShopkeeperBackgroundTitleImage";
	titleImageElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, 0);
	titleImageElementInitInfo.pivotType = UiPivotType::Top;
	titleImageElementInitInfo.drawOrder = 0;
	uiManager.createImage(titleImageElementInitInfo, titleImageTextureID, state.shopkeeperBgContextInstID, renderer);

	const TextureAsset barterBgTextureAsset(ArenaTextureName::BarterBackground);
	const TextureAsset barterBgPaletteTextureAsset = GameWorldUiView::getPaletteTextureAsset();
	const UiTextureID barterBgImageTextureID = uiManager.getOrAddTexture(barterBgTextureAsset, barterBgPaletteTextureAsset, textureManager, renderer);

	UiElementInitInfo barterBgImageElementInitInfo;
	barterBgImageElementInitInfo.name = "GameWorldShopkeeperBackgroundBarterImage";
	barterBgImageElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, ArenaRenderUtils::SCREEN_HEIGHT);
	barterBgImageElementInitInfo.pivotType = UiPivotType::Bottom;
	uiManager.createImage(barterBgImageElementInitInfo, barterBgImageTextureID, state.shopkeeperBgContextInstID, renderer);

	uiManager.setContextEnabled(state.shopkeeperBgContextInstID, true);
}

GameWorldPopUpClosedCallback GameWorldUI::makeReturnToMessageBoxCallback(ConversationMessageBoxType messageBoxType)
{
	return [messageBoxType]()
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		UiManager &uiManager = game.uiManager;
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(messageBoxType);
	};
}

void GameWorldUI::setShopkeeperPlayerGoldVisible(bool visible)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	Renderer &renderer = game.renderer;

	UiElementInstanceID textBoxElementInstID = uiManager.getElementByName(PlayerGoldTextBoxElementName);
	if (textBoxElementInstID < 0)
	{
		UiElementInitInfo textBoxElementInitInfo;
		textBoxElementInitInfo.name = PlayerGoldTextBoxElementName;
		textBoxElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, 160);
		textBoxElementInitInfo.pivotType = UiPivotType::Middle;
		textBoxElementInitInfo.drawOrder = 1;

		UiTextBoxInitInfo textBoxInitInfo;
		textBoxInitInfo.worstCaseText = TextRenderUtils::makeWorstCaseText(15);
		textBoxInitInfo.fontName = ArenaFontName::A;
		textBoxInitInfo.defaultColor = Color(28, 89, 125);
		textBoxInitInfo.alignment = TextAlignment::MiddleCenter;
		textBoxElementInstID = uiManager.createTextBox(textBoxElementInitInfo, textBoxInitInfo, state.shopkeeperBgContextInstID, renderer);
	}

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const Player &player = game.player;
	const std::string text = String::format("%s%d", exeData.services.playerGoldRemaining.c_str(), player.gold);
	uiManager.setTextBoxText(textBoxElementInstID, text.c_str());
	uiManager.setElementActive(textBoxElementInstID, visible);
}

UiButtonCallback GameWorldUI::makeDirectionsEntryCallback(const DialogueDirectionsEntry &entry)
{
	return [entry](MouseButtonType)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		UiManager &uiManager = game.uiManager;
		uiManager.disableTopMostContext();

		GameState &gameState = game.gameState;
		const double ceilingScale = gameState.getActiveCeilingScale();
		const Player &player = game.player;
		const WorldInt3 playerWorldVoxel = VoxelUtils::pointToVoxel(player.getEyePosition(), ceilingScale);
		const MapType mapType = gameState.getActiveMapType();
		DialogueManager &dialogueManager = game.dialogueManager;
		const VoxelChunkManager &voxelChunkManager = game.sceneManager.voxelChunkManager;
		const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

		const GameWorldPopUpClosedCallback popUpClosedCallback = GameWorldUI::makeReturnToMessageBoxCallback(ConversationMessageBoxType::Citizen);

		if (entry.showsDetailList)
		{
			if (mapType == MapType::Wilderness)
			{
				GameWorldUI::showTextPopUp(exeData.services.citizenRumorsModalWorkAskInTown.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, popUpClosedCallback);
			}
			else
			{
				state.dialogueWhereIsDetailEntries = GetDirectionsDetailEntries(entry.menuType, voxelChunkManager);
				GameWorldUI::showConversationListBox(ConversationListBoxType::CitizenWhereIs);
			}
		}
		else
		{
			std::string text;
			if ((mapType == MapType::Wilderness) && entry.isCityOnly())
			{
				text = exeData.dialogue.directionIsCityOnly;
			}
			else
			{
				const std::vector<DialogueDirectionsDetailEntry> detailEntries = GetDirectionsDetailEntries(entry.menuType, voxelChunkManager);
				const DialogueDirectionsDetailEntry *nearestDetailEntry = nullptr;
				const int nearestDetailEntryIndex = GetNearestDirectionsDetailEntryIndex(detailEntries, playerWorldVoxel, voxelChunkManager);
				if (nearestDetailEntryIndex >= 0)
				{
					nearestDetailEntry = &detailEntries[nearestDetailEntryIndex];
				}

				bool isEntryValidForAutomap;
				text = GetSubstitutedTextForDirectionsEntry(nearestDetailEntry, playerWorldVoxel, dialogueManager, &isEntryValidForAutomap);

				if (isEntryValidForAutomap)
				{
					DebugAssert(nearestDetailEntry != nullptr);
					gameState.addAutomapDirectionsDetailEntry(nearestDetailEntry->buildingName, nearestDetailEntry->entranceWorldVoxel);
				}
			}

			GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, popUpClosedCallback);
		}
	};
}

UiButtonCallback GameWorldUI::makeDirectionsDetailEntryCallback(const DialogueDirectionsDetailEntry &detailEntry)
{
	return [detailEntry](MouseButtonType)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		UiManager &uiManager = game.uiManager;
		uiManager.disableTopMostContext();

		GameState &gameState = game.gameState;
		const double ceilingScale = gameState.getActiveCeilingScale();
		const Player &player = game.player;
		const WorldInt3 playerWorldVoxel = VoxelUtils::pointToVoxel(player.getEyePosition(), ceilingScale);
		DialogueManager &dialogueManager = game.dialogueManager;
		const VoxelChunkManager &voxelChunkManager = game.sceneManager.voxelChunkManager;

		const Span<const DialogueDirectionsDetailEntry> detailEntries(&detailEntry, 1);
		const DialogueDirectionsDetailEntry *nearestDetailEntry = nullptr;
		const int nearestDetailEntryIndex = GetNearestDirectionsDetailEntryIndex(detailEntries, playerWorldVoxel, voxelChunkManager);
		if (nearestDetailEntryIndex >= 0)
		{
			nearestDetailEntry = &detailEntry;
		}

		bool isEntryValidForAutomap;
		const std::string text = GetSubstitutedTextForDirectionsEntry(nearestDetailEntry, playerWorldVoxel, dialogueManager, &isEntryValidForAutomap);
		const GameWorldPopUpClosedCallback popUpClosedCallback = GameWorldUI::makeReturnToMessageBoxCallback(ConversationMessageBoxType::Citizen);
		GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, popUpClosedCallback);

		if (isEntryValidForAutomap)
		{
			DebugAssert(nearestDetailEntry != nullptr);
			gameState.addAutomapDirectionsDetailEntry(nearestDetailEntry->buildingName, nearestDetailEntry->entranceWorldVoxel);
		}
	};
}

UiButtonCallback GameWorldUI::makeShopkeeperItemPurchaseCallback(ItemDefinitionID itemDefID, int itemGoldPrice, ConversationMessageBoxType returnMessageBoxType)
{
	return [itemDefID, itemGoldPrice, returnMessageBoxType](MouseButtonType)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		UiManager &uiManager = game.uiManager;
		uiManager.disableTopMostContext();

		const GameWorldPopUpClosedCallback popUpClosedCallback = GameWorldUI::makeReturnToMessageBoxCallback(returnMessageBoxType);

		const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
		const ItemDefinition &selectedItemDef = itemLibrary.getDefinition(itemDefID);
		const std::string selectedItemDisplayName = selectedItemDef.getDisplayNameWithoutQty();

		Player &player = game.player;
		ItemInventory &playerInventory = player.inventory;
		const bool canPlayerAffordPurchase = player.gold >= itemGoldPrice;
		if (canPlayerAffordPurchase)
		{
			playerInventory.insert(itemDefID);
			player.gold -= itemGoldPrice;

			const std::string text = String::format("Bought %s for %d gold.", selectedItemDisplayName.c_str(), itemGoldPrice);
			GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, GameWorldUiView::StatusPopUpTextAlignment, popUpClosedCallback);
		}
		else
		{
			const std::string text = String::format("%s is too expensive.", selectedItemDisplayName.c_str());
			GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, GameWorldUiView::StatusPopUpTextAlignment, popUpClosedCallback);
		}
	};
}

UiButtonCallback GameWorldUI::makeShopkeeperItemSellCallback(int playerInventorySlotIndex)
{
	return [playerInventorySlotIndex](MouseButtonType)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		UiManager &uiManager = game.uiManager;
		uiManager.disableTopMostContext();

		const GameWorldPopUpClosedCallback popUpClosedCallback = GameWorldUI::makeReturnToMessageBoxCallback(ConversationMessageBoxType::Equipment);

		Player &player = game.player;
		ItemInventory &playerInventory = player.inventory;
		ItemInstance &selectedItemInst = playerInventory.getSlot(playerInventorySlotIndex);
		const bool isSelectedItemEquipped = selectedItemInst.isEquipped;

		const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
		const ItemDefinition &selectedItemDef = itemLibrary.getDefinition(selectedItemInst.defID);
		const std::string selectedItemDisplayName = selectedItemDef.getDisplayNameWithoutQty();
		const int selectedItemGoldValue = selectedItemDef.getGoldValue();

		player.gold += selectedItemGoldValue;
		selectedItemInst.clear();
		playerInventory.compact();

		if (isSelectedItemEquipped)
		{
			if (selectedItemDef.type == ItemType::Weapon)
			{
				// Reset weapon animation.
				player.setWeaponAnimationFromItem(player.getEquippedWeaponItemDefID());
			}
		}

		const std::string text = String::format("Sold %s for %d gold.", selectedItemDisplayName.c_str(), selectedItemGoldValue);
		GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, GameWorldUiView::StatusPopUpTextAlignment, popUpClosedCallback);
	};
}

UiButtonCallback GameWorldUI::makeShopkeeperItemRepairCallback(int playerInventorySlotIndex)
{
	return [playerInventorySlotIndex](MouseButtonType)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		UiManager &uiManager = game.uiManager;
		uiManager.disableTopMostContext();

		const std::string text = "Repairing not implemented.";
		const GameWorldPopUpClosedCallback popUpClosedCallback = GameWorldUI::makeReturnToMessageBoxCallback(ConversationMessageBoxType::Equipment);
		GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, GameWorldUiView::StatusPopUpTextAlignment, popUpClosedCallback);
	};
}

UiButtonCallback GameWorldUI::makeMagesGuildSpellPurchaseCallback(const std::string &spellName)
{
	return [spellName](MouseButtonType)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		UiManager &uiManager = game.uiManager;
		uiManager.disableTopMostContext();

		const std::string text = "Spells not implemented.";
		const GameWorldPopUpClosedCallback popUpClosedCallback = GameWorldUI::makeReturnToMessageBoxCallback(ConversationMessageBoxType::MagesGuild);
		GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, GameWorldUiView::StatusPopUpTextAlignment, popUpClosedCallback);
	};
}

UiButtonCallback GameWorldUI::makeTavernDrinkPurchaseCallback(const std::string &drinkName, int drinkGoldPrice)
{
	return [drinkName, drinkGoldPrice](MouseButtonType)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		UiManager &uiManager = game.uiManager;
		uiManager.disableTopMostContext();

		const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
		const GameWorldPopUpClosedCallback returnToTavernMessageBoxCallback = GameWorldUI::makeReturnToMessageBoxCallback(ConversationMessageBoxType::Tavern);

		Player &player = game.player;

		std::string text;
		GameWorldPopUpClosedCallback popUpClosedCallback;
		if (player.gold >= drinkGoldPrice)
		{
			player.gold -= drinkGoldPrice;

			std::string consumeDrinkText = String::replace(exeData.services.tavernConsumeDrink, "%s", drinkName);
			text = String::distributeNewlines(consumeDrinkText, 60);

			popUpClosedCallback = [&game, returnToTavernMessageBoxCallback]()
			{
				Player &player = game.player;
				player.effectsState.applyDrink();

				returnToTavernMessageBoxCallback();
			};
		}
		else
		{
			text = String::format("%s is too expensive.", drinkName.c_str());
			popUpClosedCallback = returnToTavernMessageBoxCallback;
		}

		GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, popUpClosedCallback);
	};
}

UiButtonCallback GameWorldUI::makeTavernRoomPurchaseCallback(int roomType, int goldPrice)
{
	return [roomType, goldPrice](MouseButtonType)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		GameState &gameState = game.gameState;
		UiManager &uiManager = game.uiManager;
		uiManager.disableTopMostContext();

		const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
		const std::string &roomTypeName = exeData.services.tavernRoomTypes[roomType];

		const GameWorldPopUpClosedCallback popUpClosedCallback = GameWorldUI::makeReturnToMessageBoxCallback(ConversationMessageBoxType::Tavern);

		Player &player = game.player;

		std::string text;
		if (player.gold >= goldPrice)
		{
			player.gold -= goldPrice;

			const int rentedRoomHours = 24;
			gameState.setTavernRentedRoom(roomType, rentedRoomHours);

			text = String::format(exeData.services.tavernRoomRented.c_str(), roomTypeName.c_str());
		}
		else
		{
			text = String::format("%s is too expensive.", roomTypeName.c_str());
		}
		
		GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, GameWorldUiView::StatusPopUpTextAlignment, popUpClosedCallback);
	};
}

UiButtonCallback GameWorldUI::makeTempleCurePurchaseCallback()
{
	return [](MouseButtonType)
	{
		GameWorldUiState &state = GameWorldUI::state;
		Game &game = *state.game;
		UiManager &uiManager = game.uiManager;
		uiManager.disableTopMostContext();

		Player &player = game.player;
		player.effectsState.cureDisease();

		const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
		const std::string text = String::format(exeData.services.templeReceiveCuring.c_str(), player.firstName.c_str());
		const GameWorldPopUpClosedCallback popUpClosedCallback = GameWorldUI::makeReturnToMessageBoxCallback(ConversationMessageBoxType::Temple);
		GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, GameWorldUiView::StatusPopUpTextAlignment, popUpClosedCallback);
	};
}

void GameWorldUI::onPlayerStealItemSuccess(const ItemLibraryPredicate &stealableItemsPredicate, ConversationMessageBoxType mainMessageBoxType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
	const std::vector<ItemDefinitionID> stealableItemDefIDs = itemLibrary.getDefinitionIDsIf(stealableItemsPredicate);

	Random &random = game.random;
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

	const int stolenItemDefIdIndex = random.next(static_cast<int>(stealableItemDefIDs.size()));
	const ItemDefinitionID stolenItemDefID = stealableItemDefIDs[stolenItemDefIdIndex];
	const ItemDefinition &stolenItemDef = itemLibrary.getDefinition(stolenItemDefID);
	const std::string itemName = stolenItemDef.getDisplayNameWithoutQty();
	const std::string text = String::replace(exeData.services.equipmentStealSuccess, "%s", itemName.c_str());

	Player &player = game.player;
	player.inventory.insert(stolenItemDefID);

	GameWorldPopUpClosedCallback callback = [mainMessageBoxType, &uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(mainMessageBoxType);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, GameWorldUiView::StatusPopUpTextAlignment, callback);
}

void GameWorldUI::onPlayerStealItemFailure()
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const std::string &text = exeData.services.tavernSneakIntoRoomUnsuccessful;
	GameWorldPopUpClosedCallback callback = [&game, &uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::onCloseConversationButtonSelected(MouseButtonType::Left);

		GameState &gameState = game.gameState;
		gameState.queueCityGuardEncounter(game);
	};

	GameWorldUI::showTextPopUp(text.c_str(), ArenaFontName::A, GameWorldUiView::StatusPopUpTextAlignment, callback);
}

bool GameWorldUI::isTriggerTextVisible()
{
	const GameWorldUiState &state = GameWorldUI::state;
	return state.triggerTextRemainingSeconds > 0.0;
}

bool GameWorldUI::isActionTextVisible()
{
	const GameWorldUiState &state = GameWorldUI::state;
	return state.actionTextRemainingSeconds > 0.0;
}

bool GameWorldUI::isEffectTextVisible()
{
	const GameWorldUiState &state = GameWorldUI::state;
	return state.effectTextRemainingSeconds > 0.0;
}

bool GameWorldUI::isCampingHoursTextVisible()
{
	const GameWorldUiState &state = GameWorldUI::state;
	const Game &game = *state.game;
	return game.gameState.isCamping();
}

void GameWorldUI::setTriggerText(const char *str)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID textBoxElementInstID = uiManager.getElementByName(TriggerTextBoxElementName);
	uiManager.setTextBoxText(textBoxElementInstID, str);

	GameWorldUI::setTriggerTextDuration(str);
}

void GameWorldUI::setActionText(const char *str)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID textBoxElementInstID = uiManager.getElementByName(ActionTextBoxElementName);
	uiManager.setTextBoxText(textBoxElementInstID, str);

	GameWorldUI::setActionTextDuration(str);
}

void GameWorldUI::setEffectText(const char *str)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID textBoxElementInstID = uiManager.getElementByName(EffectTextBoxElementName);
	uiManager.setTextBoxText(textBoxElementInstID, str);

	GameWorldUI::setEffectTextDuration(str);
}

void GameWorldUI::setCampingHoursText(const char *str)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const UiElementInstanceID textBoxElementInstID = uiManager.getElementByName(CampingHoursTextBoxElementName);
	uiManager.setTextBoxText(textBoxElementInstID, str);
}

void GameWorldUI::setTriggerTextDuration(const std::string_view text)
{
	GameWorldUiState &state = GameWorldUI::state;
	state.triggerTextRemainingSeconds = GameWorldUiView::getTriggerTextSeconds(text);
}

void GameWorldUI::setActionTextDuration(const std::string_view text)
{
	GameWorldUiState &state = GameWorldUI::state;
	state.actionTextRemainingSeconds = GameWorldUiView::getActionTextSeconds(text);
}

void GameWorldUI::setEffectTextDuration(const std::string_view text)
{
	GameWorldUiState &state = GameWorldUI::state;
	state.effectTextRemainingSeconds = GameWorldUiView::getEffectTextSeconds(text);
}

void GameWorldUI::resetTriggerTextDuration()
{
	GameWorldUiState &state = GameWorldUI::state;
	state.triggerTextRemainingSeconds = 0.0;
}

void GameWorldUI::resetActionTextDuration()
{
	GameWorldUiState &state = GameWorldUI::state;
	state.actionTextRemainingSeconds = 0.0;
}

void GameWorldUI::resetEffectTextDuration()
{
	GameWorldUiState &state = GameWorldUI::state;
	state.effectTextRemainingSeconds = 0.0;
}

void GameWorldUI::onMouseButtonChanged(Game &game, MouseButtonType type, const Int2 &position, bool pressed)
{
	const GameWorldUiState &state = GameWorldUI::state;
	const Rect &centerCursorRegion = state.nativeCursorRegions[GameWorldUiView::CursorMiddleIndex];

	if (pressed)
	{
		GameState &gameState = game.gameState;
		if (gameState.isCamping())
		{
			gameState.clearCampingState();
		}

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
				const WeaponAnimationDefinitionID weaponAnimDefID = player.getEquippedWeaponAnimationDefID();
				const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(weaponAnimDefID);
				DebugAssertIndex(weaponAnimDef.states, weaponAnimInst.currentStateIndex);
				const WeaponAnimationDefinitionState &weaponAnimDefState = weaponAnimDef.states[weaponAnimInst.currentStateIndex];

				const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
				ItemDefinitionID weaponItemDefID = player.getEquippedWeaponItemDefID();
				bool isRangedWeapon = false;
				if (weaponItemDefID >= 0)
				{
					const ItemDefinition &weaponItemDef = itemLibrary.getDefinition(weaponItemDefID);
					isRangedWeapon = weaponItemDef.weapon.isRanged;
				}

				if (WeaponAnimationUtils::isIdle(weaponAnimDefState) && !isRangedWeapon)
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
					GameWorldUI::onScreenToWorldInteraction(position, true);
				}
				else if (isRightClick)
				{
					GameWorldUI::onScreenToWorldInteraction(position, false);
				}
			}
		}
	}
}

void GameWorldUI::onMouseButtonHeld(Game &game, MouseButtonType type, const Int2 &position, double dt)
{
	const GameWorldUiState &state = GameWorldUI::state;
	const Options &options = game.options;
	const Rect &centerCursorRegion = state.nativeCursorRegions[GameWorldUiView::CursorMiddleIndex];
	if (!options.getGraphics_ModernInterface() && !centerCursorRegion.contains(position))
	{
		if (type == MouseButtonType::Left)
		{
			// @todo: move out of PlayerLogicController::handlePlayerTurning() and handlePlayerAttack()
		}
	}
}

void GameWorldUI::onWindowResized(int width, int height)
{
	GameWorldUiState &state = GameWorldUI::state;
	state.updateNativeCursorRegions(width, height);

	Game &game = *state.game;
	Renderer &renderer = game.renderer;
	DebugAssert(state.playerHurtTextureID >= 0);
	renderer.freeUiTexture(state.playerHurtTextureID);

	UiManager &uiManager = game.uiManager;
	const Window &window = game.window;
	const UiElementInstanceID playerHurtImageElementInstID = uiManager.getElementByName(PlayerHurtImageElementName);
	uiManager.setTransformSize(playerHurtImageElementInstID, window.getSceneViewDimensions());

	state.playerHurtTextureID = GameWorldUiView::allocPlayerHurtTexture(window.getSceneViewAspectRatio(), window.fullGameWindow, renderer);
	uiManager.setImageTexture(playerHurtImageElementInstID, state.playerHurtTextureID);
}

void GameWorldUI::onCharacterSheetButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	game.setNextContext(CharacterUI::ContextName);
}

void GameWorldUI::onWeaponToggleButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	if (!game.canPlayerMoveAndTurn())
	{
		return;
	}

	Player &player = game.player;
	WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
	const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
	const WeaponAnimationDefinitionID weaponAnimDefID = player.getEquippedWeaponAnimationDefID();
	const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(weaponAnimDefID);
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

void GameWorldUI::onMapButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	if (mouseButtonType == MouseButtonType::Left)
	{
		game.setNextContext(AutomapUI::ContextName);
	}
	else if (mouseButtonType == MouseButtonType::Right)
	{
		const MapType mapType = gameState.getActiveMapType();
		const EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
		const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

		const Player &player = game.player;
		const WorldDouble3 playerTravelPosition = player.getEyePosition();
		const bool isPlayerSafeToTravel = !entityChunkManager.anyEnemiesNearby(playerTravelPosition);
		const bool isPlayerInBoat = false; // @todo vehicle support
		const bool isPlayerAllowedToTravel = mapType != MapType::Interior;

		std::string text;
		if (!isPlayerSafeToTravel)
		{
			text = exeData.travel.notSafeToTravel;
		}
		else if (isPlayerInBoat)
		{
			text = exeData.travel.notAllowedToTravelInBoat;
		}
		else if (!isPlayerAllowedToTravel)
		{
			text = exeData.travel.notAllowedToTravel;
		}

		if (!text.empty())
		{
			GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, GameWorldUiView::StatusPopUpTextAlignment);
		}
		else
		{
			game.setNextContext(WorldMapUI::ContextName);
		}
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(mouseButtonType)));
	}
}

void GameWorldUI::onStealButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	GameWorldUI::setInteractionType(GameWorldInteractionType::Thieving);
}

void GameWorldUI::onStatusButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	const std::string text = GameWorldUiModel::getStatusButtonText(game);
	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, GameWorldUiView::StatusPopUpTextAlignment);
}

void GameWorldUI::onMagicButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	DebugLog("Magic.");
}

void GameWorldUI::onLogbookButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	game.setNextContext(LogbookUI::ContextName);
}

void GameWorldUI::onUseItemButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		return;
	}

	DebugLog("Use item.");
}

void GameWorldUI::onCampButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	GameState &gameState = game.gameState;
	if (gameState.isCamping())
	{
		gameState.clearCampingState();
		return;
	}

	const MapType mapType = gameState.getActiveMapType();
	const MapSubDefinition &mapSubDef = gameState.getActiveMapDef().getSubDefinition();
	const EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

	const Player &player = game.player;
	const WorldDouble3 playerRestPosition = player.getEyePosition();
	const bool isPlayerSafeForResting = !entityChunkManager.anyEnemiesNearby(playerRestPosition);
	const bool isPlayerAllowedToRest = (mapType != MapType::City) && player.groundState.onGround && !player.groundState.isSwimming;
	const bool isPlayerAttemptingRestInTavern = (mapType == MapType::Interior) && (mapSubDef.interior.interiorType == ArenaInteriorType::Tavern);

	std::string text;
	if (!isPlayerSafeForResting)
	{
		text = exeData.camping.enemiesNearbyBeforeResting;
	}
	else if (!isPlayerAllowedToRest)
	{
		text = exeData.camping.campingNotAllowed;
	}
	else if (isPlayerAttemptingRestInTavern)
	{
		if (!gameState.canUseTavernRentedRoomForCamping())
		{
			text = exeData.camping.tavernBedNotRented;
		}
	}

	if (!text.empty())
	{
		GameWorldUI::showTextPopUp(text.c_str(), ArenaFontName::A, GameWorldUiView::StatusPopUpTextAlignment);
	}
	else
	{
		GameWorldUI::showCampModal();
	}
}

void GameWorldUI::onScrollUpButtonSelected(MouseButtonType mouseButtonType)
{
	// Nothing yet.
}

void GameWorldUI::onScrollDownButtonSelected(MouseButtonType mouseButtonType)
{
	// Nothing yet.
}

void GameWorldUI::onCloseConversationButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	DialogueManager &dialogueManager = game.dialogueManager;
	state.dialogueWhereIsDetailEntries.clear();
	
	// Have to check here since the real-time effects check doesn't handle coming out of UI.
	const Player &player = game.player;
	const bool isBecomingDrunk = player.effectsState.isDrunk() && !state.dialogueStartPlayerEffectsState.isDrunk();
	if (isBecomingDrunk)
	{
		const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
		const Span<const std::string> effectNames = exeData.status.effectNames;
		const std::string drunkText = GameWorldUiModel::getEffectTextBoxMessage(effectNames[5], exeData);
		GameWorldUI::setEffectText(drunkText.c_str());
	}

	state.dialogueStartPlayerEffectsState.clear();

	uiManager.setContextEnabled(state.conversationModalContextInstID, false);
	uiManager.setContextEnabled(state.shopkeeperBgContextInstID, false);
	dialogueManager.endDialogue();
	GameWorldUI::setConversationMessageBoxInputActionMapActive(nullptr);
	GameWorldUI::onPauseChanged(false);
}

void GameWorldUI::onNpcWhoAreYouButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	DialogueManager &dialogueManager = game.dialogueManager;
	uiManager.disableTopMostContext();

	const EntityInstance &entityInst = dialogueManager.getEntityInstance();
	EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;
	EntityDialogueState &dialogueState = entityChunkManager.dialogueStates.get(entityInst.dialogueStateID);
	const bool prevHasBeenIntroduced = dialogueState.hasBeenIntroduced;
	if (!prevHasBeenIntroduced)
	{
		dialogueState.hasBeenIntroduced = true;
	}

	const int hasBeenIntroducedEntryOffset = prevHasBeenIntroduced ? 15 : 0;
	const ArenaNpcPersonalityType personalityType = dialogueManager.getEntityPersonalityType();
	const int entryIndex = 100 + hasBeenIntroducedEntryOffset + static_cast<int>(personalityType);
	const std::string &entryValue = dialogueManager.getRandomTemplateDatEntryValue(entryIndex);
	const std::string text = dialogueManager.getSubstitutedText(entryValue.c_str());

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Citizen);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcWhereIsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const GameState &gameState = game.gameState;
	const MapType mapType = gameState.getActiveMapType();
	if (mapType == MapType::Interior)
	{
		if (gameState.isActiveMapNested())
		{
			const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
			const MapType exteriorMapType = gameState.getExteriorMapType();

			std::string text;
			if (exteriorMapType == MapType::City)
			{
				text = exeData.services.citizenRumorsModalWorkAskOutside;
			}
			else
			{
				text = exeData.services.citizenRumorsModalWorkAskInTown;
			}

			GameWorldPopUpClosedCallback callback = [&uiManager]()
			{
				uiManager.disableTopMostContext();
				GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Citizen);
			};

			GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
		}
		else
		{
			// Only in test interiors.
			DebugLogWarning("No exterior map type available for Where is...");
			GameWorldUI::onCloseConversationButtonSelected(MouseButtonType::Right);
		}
	}
	else
	{
		GameWorldUI::showConversationListBox(ConversationListBoxType::CitizenWhereIs);
	}
}

void GameWorldUI::onNpcRumorsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::CitizenRumors);
}

void GameWorldUI::onNpcRumorsGeneralButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	DialogueManager &dialogueManager = game.dialogueManager;
	constexpr int uninterestingRumorEntryKey = 159;
	constexpr int randomRumorEntryKey = 185;

	Random &random = game.random;
	int entryKey = uninterestingRumorEntryKey;
	if (random.nextBool())
	{
		entryKey = randomRumorEntryKey;
	}

	const std::string &entryValue = dialogueManager.getRandomTemplateDatEntryValue(entryKey);
	const std::string text = dialogueManager.getSubstitutedText(entryValue.c_str());

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Citizen);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcRumorsWorkButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const GameState &gameState = game.gameState;
	DialogueManager &dialogueManager = game.dialogueManager;

	std::string dialogueStr;
	if (gameState.getActiveMapType() == MapType::Wilderness || (gameState.isActiveMapNested() && gameState.getExteriorMapType() == MapType::Wilderness))
	{
		const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
		dialogueStr = exeData.services.citizenRumorsModalWorkAskInTown;
	}
	else
	{
		constexpr int noWorkEntryKey = 160;

		// @todo actual work rumors, requires quest support
		const int entryKey = noWorkEntryKey;

		const std::string &entryValue = dialogueManager.getRandomTemplateDatEntryValue(entryKey);
		dialogueStr = entryValue + " (quests not implemented)";
	}

	const std::string text = dialogueManager.getSubstitutedText(dialogueStr.c_str());

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Citizen);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcEquipmentBuyButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::EquipmentBuyItem);
}

void GameWorldUI::onNpcEquipmentSellButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationListBox(ConversationListBoxType::EquipmentSell);
}

void GameWorldUI::onNpcEquipmentRepairButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationListBox(ConversationListBoxType::EquipmentRepair);
}

void GameWorldUI::onNpcEquipmentStealButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	Random &random = game.random;
	const bool isStealSuccessful = random.nextBool();
	if (isStealSuccessful)
	{
		ItemLibraryPredicate stealableItemsPredicate = [](const ItemDefinition &itemDef)
		{
			if (itemDef.type == ItemType::Armor)
			{
				return true;
			}
			else if (itemDef.type == ItemType::Weapon)
			{
				return true;
			}
			else if (itemDef.type == ItemType::Shield)
			{
				return true;
			}
			else
			{
				return false;
			}
		};

		GameWorldUI::onPlayerStealItemSuccess(stealableItemsPredicate, ConversationMessageBoxType::Equipment);
	}
	else
	{
		GameWorldUI::onPlayerStealItemFailure();
	}
}

void GameWorldUI::onNpcEquipmentBuyWeaponsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationListBox(ConversationListBoxType::EquipmentWeapons);
}

void GameWorldUI::onNpcEquipmentBuyArmorButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationListBox(ConversationListBoxType::EquipmentArmor);
}

void GameWorldUI::onNpcMagesGuildBuyButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::MagesGuildBuyItem);
}

void GameWorldUI::onNpcMagesGuildDetectMagicButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "Detect Magic not implemented.";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::MagesGuild);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcMagesGuildSpellmakerButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const std::string text = "Spellmaker not implemented.";

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::MagesGuild);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcMagesGuildStealButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::MagesGuildSteal);
}

void GameWorldUI::onNpcMagesGuildBuyPotionsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationListBox(ConversationListBoxType::MagesGuildPotions);
}

void GameWorldUI::onNpcMagesGuildBuyMagicItemsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationListBox(ConversationListBoxType::MagesGuildMagicItems);
}

void GameWorldUI::onNpcMagesGuildBuySpellsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationListBox(ConversationListBoxType::MagesGuildSpells);
}

void GameWorldUI::onNpcMagesGuildStealPotionsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	Random &random = game.random;
	const bool isStealSuccessful = random.nextBool();
	if (isStealSuccessful)
	{
		ItemLibraryPredicate stealableItemsPredicate = [](const ItemDefinition &itemDef)
		{
			return itemDef.type == ItemType::Consumable;
		};

		GameWorldUI::onPlayerStealItemSuccess(stealableItemsPredicate, ConversationMessageBoxType::MagesGuild);
	}
	else
	{
		GameWorldUI::onPlayerStealItemFailure();
	}
}

void GameWorldUI::onNpcMagesGuildStealMagicItemsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	Random &random = game.random;
	const bool isStealSuccessful = random.nextBool();
	if (isStealSuccessful)
	{
		ItemLibraryPredicate stealableItemsPredicate = [](const ItemDefinition &itemDef)
		{
			return (itemDef.type == ItemType::Accessory) || (itemDef.type == ItemType::Trinket);
		};

		GameWorldUI::onPlayerStealItemSuccess(stealableItemsPredicate, ConversationMessageBoxType::MagesGuild);
	}
	else
	{
		GameWorldUI::onPlayerStealItemFailure();
	}
}

void GameWorldUI::onNpcTavernBuyDrinksButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationListBox(ConversationListBoxType::TavernDrinks);
}

void GameWorldUI::onNpcTavernGetARoomButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationListBox(ConversationListBoxType::TavernRooms);
}

void GameWorldUI::onNpcTavernSneakIntoARoomButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	GameState &gameState = game.gameState;
	Random &random = game.random;
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

	const bool isSneakingSuccessful = random.nextReal() <= 0.70; // Arbitrary

	std::string text;
	std::string fontName;
	GameWorldPopUpClosedCallback callback;
	if (isSneakingSuccessful)
	{
		Span<const std::string> roomTypeNames = exeData.services.tavernRoomTypes;
		const int roomType = random.next(roomTypeNames.getCount());
		const int rentedRoomHours = 24;
		gameState.setTavernRentedRoom(roomType, rentedRoomHours);

		const std::string &roomName = roomTypeNames[roomType];
		text = String::format(exeData.services.tavernSneakIntoRoomSuccessful.c_str(), roomName.c_str());
		text = String::distributeNewlines(text, 60);

		fontName = GameWorldUiView::StatusPopUpFontName;
		callback = [&uiManager]()
		{
			uiManager.disableTopMostContext();
			GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Tavern);
		};
	}
	else
	{
		text = exeData.services.tavernSneakIntoRoomUnsuccessful;
		fontName = ArenaFontName::A;
		callback = [&game, &uiManager, &gameState]()
		{
			uiManager.disableTopMostContext();
			GameWorldUI::onCloseConversationButtonSelected(MouseButtonType::Left);
			gameState.queueCityGuardEncounter(game);
		};
	}

	GameWorldUI::showTextPopUp(text.c_str(), fontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcTavernRumorsButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::TavernRumors);
}

void GameWorldUI::onNpcTavernRumorsGeneralButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	DialogueManager &dialogueManager = game.dialogueManager;
	constexpr int uninterestingRumorEntryKey = 159;
	constexpr int randomRumorEntryKey = 185;

	Random &random = game.random;
	int entryKey = uninterestingRumorEntryKey;
	if (random.nextBool())
	{
		entryKey = randomRumorEntryKey;
	}

	const std::string &entryValue = dialogueManager.getRandomTemplateDatEntryValue(entryKey);
	const std::string text = dialogueManager.getSubstitutedText(entryValue.c_str());

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Tavern);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcTavernRumorsWorkButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	DialogueManager &dialogueManager = game.dialogueManager;
	constexpr int bartenderNoWorkEntryKey = 196;

	// @todo actual work rumors, requires quest support
	const int entryKey = bartenderNoWorkEntryKey;

	const std::string &entryValue = dialogueManager.getRandomTemplateDatEntryValue(entryKey);
	const std::string entryValueAndTBD = entryValue + " (quests not implemented)";
	const std::string text = dialogueManager.getSubstitutedText(entryValueAndTBD.c_str());

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Tavern);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcTempleBlessButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const std::string text = exeData.services.templeReceiveBlessing;

	GameWorldPopUpClosedCallback callback = [&uiManager]()
	{
		uiManager.disableTopMostContext();
		GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Temple);
	};

	GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
}

void GameWorldUI::onNpcTempleCureButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const Player &player = game.player;
	if (player.effectsState.isDiseased())
	{
		GameWorldUI::showConversationListBox(ConversationListBoxType::TempleCuring);
	}
	else
	{
		const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
		const std::string text = String::format(exeData.services.templePlayerIsNotDiseased.c_str(), player.firstName.c_str());

		GameWorldPopUpClosedCallback callback = [&uiManager]()
		{
			uiManager.disableTopMostContext();
			GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Temple);
		};

		GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
	}
}

void GameWorldUI::onNpcTempleHealButtonSelected(MouseButtonType mouseButtonType)
{
	GameWorldUiState &state = GameWorldUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();

	Player &player = game.player;
	if (player.currentHealth == player.maxHealth)
	{
		const std::string text = String::format(exeData.services.templePlayerIsFullHealth.c_str(), player.firstName.c_str());

		GameWorldPopUpClosedCallback callback = [&uiManager]()
		{
			uiManager.disableTopMostContext();
			GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Temple);
		};

		GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
	}
	else
	{
		player.currentHealth = player.maxHealth;

		const std::string text = String::format(exeData.services.templeReceiveHealing.c_str(), player.firstName.c_str());

		GameWorldPopUpClosedCallback callback = [&uiManager]()
		{
			uiManager.disableTopMostContext();
			GameWorldUI::showConversationMessageBox(ConversationMessageBoxType::Temple);
		};

		GameWorldUI::showTextPopUp(text.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft, callback);
	}
}

void GameWorldUI::onActivateInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		const Options &options = game.options;
		if (options.getGraphics_ModernInterface())
		{
			const Int2 screenPoint = GameWorldUiView::getNativeWindowCenter(game.window);
			GameWorldUI::onScreenToWorldInteraction(screenPoint, true);
		}
	}
}

void GameWorldUI::onInspectInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		const Options &options = game.options;
		if (options.getGraphics_ModernInterface())
		{
			const Int2 screenPoint = GameWorldUiView::getNativeWindowCenter(game.window);
			GameWorldUI::onScreenToWorldInteraction(screenPoint, false);
		}
	}
}

void GameWorldUI::onCharacterSheetInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onCharacterSheetButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onToggleWeaponInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onWeaponToggleButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onAutomapInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onMapButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onWorldMapInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onMapButtonSelected(MouseButtonType::Right);
	}
}

void GameWorldUI::onStealInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onStealButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onStatusInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onStatusButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onCastMagicInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onMagicButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onLogbookInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onLogbookButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onUseItemInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onUseItemButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onCampInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onCampButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onToggleCompassInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		Options &options = game.options;
		const bool isCompassVisible = !options.getMisc_ShowCompass();
		options.setMisc_ShowCompass(isCompassVisible);
	}
}

void GameWorldUI::onPlayerPositionInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		const GameState &gameState = game.gameState;
		if (gameState.isCamping())
		{
			return;
		}

		const std::string text = GameWorldUiModel::getPlayerPositionText(game);
		GameWorldUI::setActionText(text.c_str());
	}
}

void GameWorldUI::onPauseMenuInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		GameState &gameState = game.gameState;
		if (gameState.isCamping())
		{
			gameState.clearCampingState();
			return;
		}

		game.setNextContext(PauseMenuUI::ContextName);
	}
}

void GameWorldUI::onEquipmentStoreBuyInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcEquipmentBuyButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onEquipmentStoreSellInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcEquipmentSellButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onEquipmentStoreRepairInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcEquipmentRepairButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onEquipmentStoreStealInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcEquipmentStealButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onEquipmentStoreExitInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onCloseConversationButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onEquipmentStoreBuyWeaponInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcEquipmentBuyWeaponsButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onEquipmentStoreBuyArmorInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcEquipmentBuyArmorButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onMagesGuildBuyInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcMagesGuildBuyButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onMagesGuildDetectMagicInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcMagesGuildDetectMagicButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onMagesGuildSpellmakerInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcMagesGuildSpellmakerButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onMagesGuildStealInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcMagesGuildStealButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onMagesGuildExitInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onCloseConversationButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onMagesGuildBuyPotionsInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcMagesGuildBuyPotionsButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onMagesGuildBuyMagicItemsInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcMagesGuildBuyMagicItemsButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onMagesGuildBuySpellsInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcMagesGuildBuySpellsButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onMagesGuildStealPotionsInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcMagesGuildStealPotionsButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onMagesGuildStealMagicItemsInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcMagesGuildStealMagicItemsButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onNpcWhoAreYouInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcWhoAreYouButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onNpcWhereIsInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcWhereIsButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onNpcRumorsInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcRumorsButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onNpcExitInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onCloseConversationButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onNpcRumorsGeneralInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcRumorsGeneralButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onNpcRumorsWorkInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcRumorsWorkButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onTavernBuyDrinksInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcTavernBuyDrinksButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onTavernGetRoomInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcTavernGetARoomButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onTavernSneakIntoRoomInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcTavernSneakIntoARoomButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onTavernRumorsInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcTavernRumorsButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onTavernExitInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onCloseConversationButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onTavernRumorsGeneralInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcTavernRumorsGeneralButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onTavernRumorsWorkInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcTavernRumorsWorkButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onTempleBlessInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcTempleBlessButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onTempleCureInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcTempleCureButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onTempleHealInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onNpcTempleHealButtonSelected(MouseButtonType::Left);
	}
}

void GameWorldUI::onTempleExitInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		GameWorldUI::onCloseConversationButtonSelected(MouseButtonType::Left);
	}
}
