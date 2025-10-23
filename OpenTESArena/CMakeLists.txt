PROJECT(otesa CXX)

SET(OpenTESArena_VERSION "0.17.0")

FIND_PACKAGE(SDL2 REQUIRED)
FIND_PACKAGE(OpenAL REQUIRED)
FIND_PACKAGE(WildMidi)
FIND_PACKAGE(Vulkan)

SET(EXTERNAL_LIBS ${OPENAL_LIBRARY} ${SDL2_LIBRARY})
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}" ${SDL2_INCLUDE_DIR} ${OPENAL_INCLUDE_DIR} ${JOLT_INCLUDE_DIR})

IF(WILDMIDI_FOUND)
    SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${WILDMIDI_LIBRARIES})
    INCLUDE_DIRECTORIES(${WILDMIDI_INCLUDE_DIRS})
    ADD_DEFINITIONS("-DHAVE_WILDMIDI=1")
ELSE(WILDMIDI_FOUND)
    MESSAGE(STATUS "WildMIDI not found, no MIDI support.")
ENDIF(WILDMIDI_FOUND)

IF(VULKAN_FOUND)
    SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${Vulkan_LIBRARY})
    INCLUDE_DIRECTORIES(${Vulkan_INCLUDE_DIR})
    ADD_DEFINITIONS("-DHAVE_VULKAN=1")
    ADD_DEFINITIONS("-DVULKAN_HPP_NO_EXCEPTIONS=1")
    ADD_DEFINITIONS("-DVULKAN_HPP_NO_SMART_HANDLE=1")
ELSE(VULKAN_FOUND)
    MESSAGE(STATUS "Vulkan not found, graphics backend is limited to SDL renderer.")
ENDIF(VULKAN_FOUND)

SET(SRC_ROOT ${otesa_SOURCE_DIR}/src)

SET(TES_ASSETS
    "${SRC_ROOT}/Assets/ArenaAssetUtils.h"
    "${SRC_ROOT}/Assets/ArenaLevelLibrary.cpp"
    "${SRC_ROOT}/Assets/ArenaLevelLibrary.h"
    "${SRC_ROOT}/Assets/ArenaPaletteName.h"
    "${SRC_ROOT}/Assets/ArenaPortraitUtils.cpp"
    "${SRC_ROOT}/Assets/ArenaPortraitUtils.h"
    "${SRC_ROOT}/Assets/ArenaSave.cpp"
    "${SRC_ROOT}/Assets/ArenaSave.h"
    "${SRC_ROOT}/Assets/ArenaSoundName.h"
    "${SRC_ROOT}/Assets/ArenaTextureName.h"
    "${SRC_ROOT}/Assets/ArenaTypes.cpp"
    "${SRC_ROOT}/Assets/ArenaTypes.h"
    "${SRC_ROOT}/Assets/BinaryAssetLibrary.cpp"
    "${SRC_ROOT}/Assets/BinaryAssetLibrary.h"
    "${SRC_ROOT}/Assets/CFAFile.cpp"
    "${SRC_ROOT}/Assets/CFAFile.h"
    "${SRC_ROOT}/Assets/CIFFile.cpp"
    "${SRC_ROOT}/Assets/CIFFile.h"
    "${SRC_ROOT}/Assets/CityDataFile.cpp"
    "${SRC_ROOT}/Assets/CityDataFile.h"
    "${SRC_ROOT}/Assets/COLFile.cpp"
    "${SRC_ROOT}/Assets/COLFile.h"
    "${SRC_ROOT}/Assets/Compression.cpp"
    "${SRC_ROOT}/Assets/Compression.h"
    "${SRC_ROOT}/Assets/DFAFile.cpp"
    "${SRC_ROOT}/Assets/DFAFile.h"
    "${SRC_ROOT}/Assets/ExeData.cpp"
    "${SRC_ROOT}/Assets/ExeData.h"
    "${SRC_ROOT}/Assets/ExeTypes.cpp"
    "${SRC_ROOT}/Assets/ExeTypes.h"
    "${SRC_ROOT}/Assets/ExeUnpacker.cpp"
    "${SRC_ROOT}/Assets/ExeUnpacker.h"
    "${SRC_ROOT}/Assets/FLCFile.cpp"
    "${SRC_ROOT}/Assets/FLCFile.h"
    "${SRC_ROOT}/Assets/FontFile.cpp"
    "${SRC_ROOT}/Assets/FontFile.h"
    "${SRC_ROOT}/Assets/IMGFile.cpp"
    "${SRC_ROOT}/Assets/IMGFile.h"
    "${SRC_ROOT}/Assets/INFFile.cpp"
    "${SRC_ROOT}/Assets/INFFile.h"
    "${SRC_ROOT}/Assets/LGTFile.cpp"
    "${SRC_ROOT}/Assets/LGTFile.h"
    "${SRC_ROOT}/Assets/MIFFile.cpp"
    "${SRC_ROOT}/Assets/MIFFile.h"
    "${SRC_ROOT}/Assets/MIFUtils.cpp"
    "${SRC_ROOT}/Assets/MIFUtils.h"
    "${SRC_ROOT}/Assets/RCIFile.cpp"
    "${SRC_ROOT}/Assets/RCIFile.h"
    "${SRC_ROOT}/Assets/RMDFile.cpp"
    "${SRC_ROOT}/Assets/RMDFile.h"
    "${SRC_ROOT}/Assets/SETFile.cpp"
    "${SRC_ROOT}/Assets/SETFile.h"
    "${SRC_ROOT}/Assets/TextAssetLibrary.cpp"
    "${SRC_ROOT}/Assets/TextAssetLibrary.h"
    "${SRC_ROOT}/Assets/TextureAsset.cpp"
    "${SRC_ROOT}/Assets/TextureAsset.h"
    "${SRC_ROOT}/Assets/TextureBuilder.cpp"
    "${SRC_ROOT}/Assets/TextureBuilder.h"
    "${SRC_ROOT}/Assets/TextureFileMetadata.cpp"
    "${SRC_ROOT}/Assets/TextureFileMetadata.h"
    "${SRC_ROOT}/Assets/TextureManager.cpp"
    "${SRC_ROOT}/Assets/TextureManager.h"
    "${SRC_ROOT}/Assets/TextureUtils.cpp"
    "${SRC_ROOT}/Assets/TextureUtils.h"
    "${SRC_ROOT}/Assets/TXTFile.cpp"
    "${SRC_ROOT}/Assets/TXTFile.h"
    "${SRC_ROOT}/Assets/VOCFile.cpp"
    "${SRC_ROOT}/Assets/VOCFile.h"
    "${SRC_ROOT}/Assets/WorldMapMask.cpp"
    "${SRC_ROOT}/Assets/WorldMapMask.h")

SET(TES_AUDIO
    "${SRC_ROOT}/Audio/alext.h"
    "${SRC_ROOT}/Audio/AudioManager.cpp"
    "${SRC_ROOT}/Audio/AudioManager.h"
    "${SRC_ROOT}/Audio/efx.h"
    "${SRC_ROOT}/Audio/Midi.h"
    "${SRC_ROOT}/Audio/MusicDefinition.cpp"
    "${SRC_ROOT}/Audio/MusicDefinition.h"
    "${SRC_ROOT}/Audio/MusicLibrary.cpp"
    "${SRC_ROOT}/Audio/MusicLibrary.h"
    "${SRC_ROOT}/Audio/MusicUtils.cpp"
    "${SRC_ROOT}/Audio/MusicUtils.h"
    "${SRC_ROOT}/Audio/SoundLibrary.cpp"
    "${SRC_ROOT}/Audio/SoundLibrary.h"
    "${SRC_ROOT}/Audio/WildMidi.cpp"
    "${SRC_ROOT}/Audio/WildMidi.h")

SET(TES_COLLISION
    "${SRC_ROOT}/Collision/ArenaSelectionUtils.cpp"
    "${SRC_ROOT}/Collision/ArenaSelectionUtils.h"
    "${SRC_ROOT}/Collision/CollisionChunk.cpp"
    "${SRC_ROOT}/Collision/CollisionChunk.h"
    "${SRC_ROOT}/Collision/CollisionChunkManager.cpp"
    "${SRC_ROOT}/Collision/CollisionChunkManager.h"
    "${SRC_ROOT}/Collision/CollisionShapeDefinition.cpp"
    "${SRC_ROOT}/Collision/CollisionShapeDefinition.h"
    "${SRC_ROOT}/Collision/Physics.cpp"
    "${SRC_ROOT}/Collision/Physics.h"
    "${SRC_ROOT}/Collision/PhysicsBodyActivationListener.cpp"
    "${SRC_ROOT}/Collision/PhysicsBodyActivationListener.h"
    "${SRC_ROOT}/Collision/PhysicsContactListener.cpp"
    "${SRC_ROOT}/Collision/PhysicsContactListener.h"
    "${SRC_ROOT}/Collision/PhysicsLayer.cpp"
    "${SRC_ROOT}/Collision/PhysicsLayer.h"
    "${SRC_ROOT}/Collision/RayCastTypes.cpp"
    "${SRC_ROOT}/Collision/RayCastTypes.h")

SET(TES_COMBAT
    "${SRC_ROOT}/Combat/CombatLogic.cpp"
    "${SRC_ROOT}/Combat/CombatLogic.h")

SET(TES_ENTITIES
    "${SRC_ROOT}/Entities/ArenaAnimUtils.cpp"
    "${SRC_ROOT}/Entities/ArenaAnimUtils.h"
    "${SRC_ROOT}/Entities/ArenaCitizenUtils.h"
    "${SRC_ROOT}/Entities/ArenaEntityUtils.cpp"
    "${SRC_ROOT}/Entities/ArenaEntityUtils.h"
    "${SRC_ROOT}/Entities/CitizenUtils.cpp"
    "${SRC_ROOT}/Entities/CitizenUtils.h"
    "${SRC_ROOT}/Entities/EntityAnimationDefinition.cpp"
    "${SRC_ROOT}/Entities/EntityAnimationDefinition.h"
    "${SRC_ROOT}/Entities/EntityAnimationInstance.cpp"
    "${SRC_ROOT}/Entities/EntityAnimationInstance.h"
    "${SRC_ROOT}/Entities/EntityAnimationLibrary.cpp"
    "${SRC_ROOT}/Entities/EntityAnimationLibrary.h"
    "${SRC_ROOT}/Entities/EntityAnimationUtils.h"
    "${SRC_ROOT}/Entities/EntityChunk.cpp"
    "${SRC_ROOT}/Entities/EntityChunk.h"
    "${SRC_ROOT}/Entities/EntityChunkManager.cpp"
    "${SRC_ROOT}/Entities/EntityChunkManager.h"
    "${SRC_ROOT}/Entities/EntityDefinition.cpp"
    "${SRC_ROOT}/Entities/EntityDefinition.h"
    "${SRC_ROOT}/Entities/EntityDefinitionLibrary.cpp"
    "${SRC_ROOT}/Entities/EntityDefinitionLibrary.h"
    "${SRC_ROOT}/Entities/EntityGeneration.cpp"
    "${SRC_ROOT}/Entities/EntityGeneration.h"
    "${SRC_ROOT}/Entities/EntityInstance.cpp"
    "${SRC_ROOT}/Entities/EntityInstance.h"
    "${SRC_ROOT}/Entities/EntityObservedResult.cpp"
    "${SRC_ROOT}/Entities/EntityObservedResult.h"
    "${SRC_ROOT}/Entities/EntityUtils.cpp"
    "${SRC_ROOT}/Entities/EntityUtils.h"
    "${SRC_ROOT}/Entities/EntityVisibilityChunk.cpp"
    "${SRC_ROOT}/Entities/EntityVisibilityChunk.h"
    "${SRC_ROOT}/Entities/EntityVisibilityChunkManager.cpp"
    "${SRC_ROOT}/Entities/EntityVisibilityChunkManager.h")

SET(TES_GAME
    "${SRC_ROOT}/Game/Game.cpp"
    "${SRC_ROOT}/Game/Game.h"
    "${SRC_ROOT}/Game/GameState.cpp"
    "${SRC_ROOT}/Game/GameState.h"
    "${SRC_ROOT}/Game/Options.cpp"
    "${SRC_ROOT}/Game/Options.h")

SET(TES_INPUT
    "${SRC_ROOT}/Input/ApplicationEvents.h"
    "${SRC_ROOT}/Input/InputActionDefinition.cpp"
    "${SRC_ROOT}/Input/InputActionDefinition.h"
    "${SRC_ROOT}/Input/InputActionEvents.cpp"
    "${SRC_ROOT}/Input/InputActionEvents.h"
    "${SRC_ROOT}/Input/InputActionMap.cpp"
    "${SRC_ROOT}/Input/InputActionMap.h"
    "${SRC_ROOT}/Input/InputActionMapName.h"
    "${SRC_ROOT}/Input/InputActionName.h"
    "${SRC_ROOT}/Input/InputActionType.h"
    "${SRC_ROOT}/Input/InputManager.cpp"
    "${SRC_ROOT}/Input/InputManager.h"
    "${SRC_ROOT}/Input/InputStateType.h"
    "${SRC_ROOT}/Input/PointerEvents.h"
    "${SRC_ROOT}/Input/PointerTypes.h"
    "${SRC_ROOT}/Input/TextEvents.h")

SET(TES_INTERFACE
    "${SRC_ROOT}/Interface/AutomapPanel.cpp"
    "${SRC_ROOT}/Interface/AutomapPanel.h"
    "${SRC_ROOT}/Interface/AutomapUiController.cpp"
    "${SRC_ROOT}/Interface/AutomapUiController.h"
    "${SRC_ROOT}/Interface/AutomapUiModel.cpp"
    "${SRC_ROOT}/Interface/AutomapUiModel.h"
    "${SRC_ROOT}/Interface/AutomapUiView.cpp"
    "${SRC_ROOT}/Interface/AutomapUiView.h"
    "${SRC_ROOT}/Interface/CharacterCreationUiController.cpp"
    "${SRC_ROOT}/Interface/CharacterCreationUiController.h"
    "${SRC_ROOT}/Interface/CharacterCreationUiModel.cpp"
    "${SRC_ROOT}/Interface/CharacterCreationUiModel.h"
    "${SRC_ROOT}/Interface/CharacterCreationUiView.cpp"
    "${SRC_ROOT}/Interface/CharacterCreationUiView.h"
    "${SRC_ROOT}/Interface/CharacterEquipmentPanel.cpp"
    "${SRC_ROOT}/Interface/CharacterEquipmentPanel.h"
    "${SRC_ROOT}/Interface/CharacterPanel.cpp"
    "${SRC_ROOT}/Interface/CharacterPanel.h"
    "${SRC_ROOT}/Interface/CharacterSheetUiController.cpp"
    "${SRC_ROOT}/Interface/CharacterSheetUiController.h"
    "${SRC_ROOT}/Interface/CharacterSheetUiModel.cpp"
    "${SRC_ROOT}/Interface/CharacterSheetUiModel.h"
    "${SRC_ROOT}/Interface/CharacterSheetUiView.cpp"
    "${SRC_ROOT}/Interface/CharacterSheetUiView.h"
    "${SRC_ROOT}/Interface/ChooseAttributesPanel.cpp"
    "${SRC_ROOT}/Interface/ChooseAttributesPanel.h"
    "${SRC_ROOT}/Interface/ChooseClassCreationPanel.cpp"
    "${SRC_ROOT}/Interface/ChooseClassCreationPanel.h"
    "${SRC_ROOT}/Interface/ChooseClassPanel.cpp"
    "${SRC_ROOT}/Interface/ChooseClassPanel.h"
    "${SRC_ROOT}/Interface/ChooseGenderPanel.cpp"
    "${SRC_ROOT}/Interface/ChooseGenderPanel.h"
    "${SRC_ROOT}/Interface/ChooseNamePanel.cpp"
    "${SRC_ROOT}/Interface/ChooseNamePanel.h"
    "${SRC_ROOT}/Interface/ChooseRacePanel.cpp"
    "${SRC_ROOT}/Interface/ChooseRacePanel.h"
    "${SRC_ROOT}/Interface/CinematicLibrary.cpp"
    "${SRC_ROOT}/Interface/CinematicLibrary.h"
    "${SRC_ROOT}/Interface/CinematicPanel.cpp"
    "${SRC_ROOT}/Interface/CinematicPanel.h"
    "${SRC_ROOT}/Interface/CommonUiController.cpp"
    "${SRC_ROOT}/Interface/CommonUiController.h"
    "${SRC_ROOT}/Interface/CommonUiView.cpp"
    "${SRC_ROOT}/Interface/CommonUiView.h"
    "${SRC_ROOT}/Interface/FastTravelSubPanel.cpp"
    "${SRC_ROOT}/Interface/FastTravelSubPanel.h"
    "${SRC_ROOT}/Interface/GameWorldPanel.cpp"
    "${SRC_ROOT}/Interface/GameWorldPanel.h"
    "${SRC_ROOT}/Interface/GameWorldUiController.cpp"
    "${SRC_ROOT}/Interface/GameWorldUiController.h"
    "${SRC_ROOT}/Interface/GameWorldUiModel.cpp"
    "${SRC_ROOT}/Interface/GameWorldUiModel.h"
    "${SRC_ROOT}/Interface/GameWorldUiView.cpp"
    "${SRC_ROOT}/Interface/GameWorldUiView.h"
    "${SRC_ROOT}/Interface/ImagePanel.cpp"
    "${SRC_ROOT}/Interface/ImagePanel.h"
    "${SRC_ROOT}/Interface/ImageSequencePanel.cpp"
    "${SRC_ROOT}/Interface/ImageSequencePanel.h"
    "${SRC_ROOT}/Interface/IntroUiController.cpp"
    "${SRC_ROOT}/Interface/IntroUiController.h"
    "${SRC_ROOT}/Interface/IntroUiModel.cpp"
    "${SRC_ROOT}/Interface/IntroUiModel.h"
    "${SRC_ROOT}/Interface/IntroUiView.cpp"
    "${SRC_ROOT}/Interface/IntroUiView.h"
    "${SRC_ROOT}/Interface/InventoryUiModel.cpp"
    "${SRC_ROOT}/Interface/InventoryUiModel.h"
    "${SRC_ROOT}/Interface/InventoryUiView.cpp"
    "${SRC_ROOT}/Interface/InventoryUiView.h"
    "${SRC_ROOT}/Interface/LoadSavePanel.cpp"
    "${SRC_ROOT}/Interface/LoadSavePanel.h"
    "${SRC_ROOT}/Interface/LoadSaveUiController.cpp"
    "${SRC_ROOT}/Interface/LoadSaveUiController.h"
    "${SRC_ROOT}/Interface/LoadSaveUiModel.cpp"
    "${SRC_ROOT}/Interface/LoadSaveUiModel.h"
    "${SRC_ROOT}/Interface/LoadSaveUiView.cpp"
    "${SRC_ROOT}/Interface/LoadSaveUiView.h"
    "${SRC_ROOT}/Interface/LogbookPanel.cpp"
    "${SRC_ROOT}/Interface/LogbookPanel.h"
    "${SRC_ROOT}/Interface/LogbookUiController.cpp"
    "${SRC_ROOT}/Interface/LogbookUiController.h"
    "${SRC_ROOT}/Interface/LogbookUiModel.cpp"
    "${SRC_ROOT}/Interface/LogbookUiModel.h"
    "${SRC_ROOT}/Interface/LogbookUiView.cpp"
    "${SRC_ROOT}/Interface/LogbookUiView.h"
    "${SRC_ROOT}/Interface/LootSubPanel.cpp"
    "${SRC_ROOT}/Interface/LootSubPanel.h"
    "${SRC_ROOT}/Interface/MainMenuPanel.cpp"
    "${SRC_ROOT}/Interface/MainMenuPanel.h"
    "${SRC_ROOT}/Interface/MainMenuUiController.cpp"
    "${SRC_ROOT}/Interface/MainMenuUiController.h"
    "${SRC_ROOT}/Interface/MainMenuUiModel.cpp"
    "${SRC_ROOT}/Interface/MainMenuUiModel.h"
    "${SRC_ROOT}/Interface/MainMenuUiView.cpp"
    "${SRC_ROOT}/Interface/MainMenuUiView.h"
    "${SRC_ROOT}/Interface/MainQuestSplashPanel.cpp"
    "${SRC_ROOT}/Interface/MainQuestSplashPanel.h"
    "${SRC_ROOT}/Interface/MainQuestSplashUiController.cpp"
    "${SRC_ROOT}/Interface/MainQuestSplashUiController.h"
    "${SRC_ROOT}/Interface/MainQuestSplashUiModel.cpp"
    "${SRC_ROOT}/Interface/MainQuestSplashUiModel.h"
    "${SRC_ROOT}/Interface/MainQuestSplashUiView.cpp"
    "${SRC_ROOT}/Interface/MainQuestSplashUiView.h"
    "${SRC_ROOT}/Interface/MessageBoxSubPanel.cpp"
    "${SRC_ROOT}/Interface/MessageBoxSubPanel.h"
    "${SRC_ROOT}/Interface/OptionsPanel.cpp"
    "${SRC_ROOT}/Interface/OptionsPanel.h"
    "${SRC_ROOT}/Interface/OptionsUiController.cpp"
    "${SRC_ROOT}/Interface/OptionsUiController.h"
    "${SRC_ROOT}/Interface/OptionsUiModel.cpp"
    "${SRC_ROOT}/Interface/OptionsUiModel.h"
    "${SRC_ROOT}/Interface/OptionsUiView.cpp"
    "${SRC_ROOT}/Interface/OptionsUiView.h"
    "${SRC_ROOT}/Interface/Panel.cpp"
    "${SRC_ROOT}/Interface/Panel.h"
    "${SRC_ROOT}/Interface/PauseMenuPanel.cpp"
    "${SRC_ROOT}/Interface/PauseMenuPanel.h"
    "${SRC_ROOT}/Interface/PauseMenuUiController.cpp"
    "${SRC_ROOT}/Interface/PauseMenuUiController.h"
    "${SRC_ROOT}/Interface/PauseMenuUiModel.cpp"
    "${SRC_ROOT}/Interface/PauseMenuUiModel.h"
    "${SRC_ROOT}/Interface/PauseMenuUiView.cpp"
    "${SRC_ROOT}/Interface/PauseMenuUiView.h"
    "${SRC_ROOT}/Interface/ProvinceMapPanel.cpp"
    "${SRC_ROOT}/Interface/ProvinceMapPanel.h"
    "${SRC_ROOT}/Interface/ProvinceMapUiController.cpp"
    "${SRC_ROOT}/Interface/ProvinceMapUiController.h"
    "${SRC_ROOT}/Interface/ProvinceMapUiModel.cpp"
    "${SRC_ROOT}/Interface/ProvinceMapUiModel.h"
    "${SRC_ROOT}/Interface/ProvinceMapUiView.cpp"
    "${SRC_ROOT}/Interface/ProvinceMapUiView.h"
    "${SRC_ROOT}/Interface/ProvinceSearchSubPanel.cpp"
    "${SRC_ROOT}/Interface/ProvinceSearchSubPanel.h"
    "${SRC_ROOT}/Interface/TextCinematicDefinition.cpp"
    "${SRC_ROOT}/Interface/TextCinematicDefinition.h"
    "${SRC_ROOT}/Interface/TextCinematicPanel.cpp"
    "${SRC_ROOT}/Interface/TextCinematicPanel.h"
    "${SRC_ROOT}/Interface/TextCinematicUiModel.cpp"
    "${SRC_ROOT}/Interface/TextCinematicUiModel.h"
    "${SRC_ROOT}/Interface/TextCinematicUiView.cpp"
    "${SRC_ROOT}/Interface/TextCinematicUiView.h"
    "${SRC_ROOT}/Interface/TextSubPanel.cpp"
    "${SRC_ROOT}/Interface/TextSubPanel.h"
    "${SRC_ROOT}/Interface/WorldMapPanel.cpp"
    "${SRC_ROOT}/Interface/WorldMapPanel.h"
    "${SRC_ROOT}/Interface/WorldMapUiController.cpp"
    "${SRC_ROOT}/Interface/WorldMapUiController.h"
    "${SRC_ROOT}/Interface/WorldMapUiModel.cpp"
    "${SRC_ROOT}/Interface/WorldMapUiModel.h"
    "${SRC_ROOT}/Interface/WorldMapUiView.cpp"
    "${SRC_ROOT}/Interface/WorldMapUiView.h")

SET(TES_ITEMS
    "${SRC_ROOT}/Items/ArenaItemUtils.cpp"
    "${SRC_ROOT}/Items/ArenaItemUtils.h"
    "${SRC_ROOT}/Items/ItemConditionLibrary.cpp"
    "${SRC_ROOT}/Items/ItemConditionLibrary.h"
    "${SRC_ROOT}/Items/ItemDefinition.cpp"
    "${SRC_ROOT}/Items/ItemDefinition.h"
    "${SRC_ROOT}/Items/ItemInstance.cpp"
    "${SRC_ROOT}/Items/ItemInstance.h"
    "${SRC_ROOT}/Items/ItemInventory.cpp"
    "${SRC_ROOT}/Items/ItemInventory.h"
    "${SRC_ROOT}/Items/ItemLibrary.cpp"
    "${SRC_ROOT}/Items/ItemLibrary.h"
    "${SRC_ROOT}/Items/ItemMaterialLibrary.cpp"
    "${SRC_ROOT}/Items/ItemMaterialLibrary.h")

SET(TES_MATH
    "${SRC_ROOT}/Math/ArenaMathUtils.cpp"
    "${SRC_ROOT}/Math/ArenaMathUtils.h"
    "${SRC_ROOT}/Math/BoundingBox.cpp"
    "${SRC_ROOT}/Math/BoundingBox.h"
    "${SRC_ROOT}/Math/Constants.h"
    "${SRC_ROOT}/Math/MathUtils.cpp"
    "${SRC_ROOT}/Math/MathUtils.h"
    "${SRC_ROOT}/Math/Matrix4.cpp"
    "${SRC_ROOT}/Math/Matrix4.h"
    "${SRC_ROOT}/Math/Quaternion.cpp"
    "${SRC_ROOT}/Math/Quaternion.h"
    "${SRC_ROOT}/Math/Random.cpp"
    "${SRC_ROOT}/Math/Random.h"
    "${SRC_ROOT}/Math/RandomUtils.h"
    "${SRC_ROOT}/Math/Rect.cpp"
    "${SRC_ROOT}/Math/Rect.h"
    "${SRC_ROOT}/Math/Vector2.cpp"
    "${SRC_ROOT}/Math/Vector2.h"
    "${SRC_ROOT}/Math/Vector3.cpp"
    "${SRC_ROOT}/Math/Vector3.h"
    "${SRC_ROOT}/Math/Vector4.cpp"
    "${SRC_ROOT}/Math/Vector4.h")

SET(TES_PLAYER
    "${SRC_ROOT}/Player/ArenaPlayerUtils.cpp"
    "${SRC_ROOT}/Player/ArenaPlayerUtils.h"
    "${SRC_ROOT}/Player/ArenaWeaponUtils.h"
    "${SRC_ROOT}/Player/CharacterClassGeneration.cpp"
    "${SRC_ROOT}/Player/CharacterClassGeneration.h"
    "${SRC_ROOT}/Player/CharacterCreationState.cpp"
    "${SRC_ROOT}/Player/CharacterCreationState.h"
    "${SRC_ROOT}/Player/CharacterQuestion.cpp"
    "${SRC_ROOT}/Player/CharacterQuestion.h"
    "${SRC_ROOT}/Player/Player.cpp"
    "${SRC_ROOT}/Player/Player.h"
    "${SRC_ROOT}/Player/PlayerInterface.h"
    "${SRC_ROOT}/Player/PlayerLogic.cpp"
    "${SRC_ROOT}/Player/PlayerLogic.h"
    "${SRC_ROOT}/Player/WeaponAnimation.cpp"
    "${SRC_ROOT}/Player/WeaponAnimation.h"
    "${SRC_ROOT}/Player/WeaponAnimationLibrary.cpp"
    "${SRC_ROOT}/Player/WeaponAnimationLibrary.h"
    "${SRC_ROOT}/Player/WeaponAnimationUtils.cpp"
    "${SRC_ROOT}/Player/WeaponAnimationUtils.h")

SET(TES_RENDERING
    "${SRC_ROOT}/Rendering/ArenaRenderUtils.cpp"
    "${SRC_ROOT}/Rendering/ArenaRenderUtils.h"
    "${SRC_ROOT}/Rendering/RenderBackend.cpp"
    "${SRC_ROOT}/Rendering/RenderBackend.h"
    "${SRC_ROOT}/Rendering/RenderBuffer.cpp"
    "${SRC_ROOT}/Rendering/RenderBuffer.h"
    "${SRC_ROOT}/Rendering/RenderBackendType.h"
    "${SRC_ROOT}/Rendering/RenderCamera.cpp"
    "${SRC_ROOT}/Rendering/RenderCamera.h"
    "${SRC_ROOT}/Rendering/RenderCommand.cpp"
    "${SRC_ROOT}/Rendering/RenderCommand.h"
    "${SRC_ROOT}/Rendering/RenderDrawCall.cpp"
    "${SRC_ROOT}/Rendering/RenderDrawCall.h"
    "${SRC_ROOT}/Rendering/RenderEntityManager.cpp"
    "${SRC_ROOT}/Rendering/RenderEntityManager.h"
    "${SRC_ROOT}/Rendering/Renderer.cpp"
    "${SRC_ROOT}/Rendering/Renderer.h"
    "${SRC_ROOT}/Rendering/RendererUtils.cpp"
    "${SRC_ROOT}/Rendering/RendererUtils.h"
    "${SRC_ROOT}/Rendering/RenderFrameSettings.cpp"
    "${SRC_ROOT}/Rendering/RenderFrameSettings.h"
    "${SRC_ROOT}/Rendering/RenderInitSettings.cpp"
    "${SRC_ROOT}/Rendering/RenderInitSettings.h"
    "${SRC_ROOT}/Rendering/RenderLightManager.cpp"
    "${SRC_ROOT}/Rendering/RenderLightManager.h"
    "${SRC_ROOT}/Rendering/RenderLightUtils.h"
    "${SRC_ROOT}/Rendering/RenderMaterialUtils.cpp"
    "${SRC_ROOT}/Rendering/RenderMaterialUtils.h"
    "${SRC_ROOT}/Rendering/RenderMeshInstance.cpp"
    "${SRC_ROOT}/Rendering/RenderMeshInstance.h"
    "${SRC_ROOT}/Rendering/RenderMeshUtils.cpp"
    "${SRC_ROOT}/Rendering/RenderMeshUtils.h"
    "${SRC_ROOT}/Rendering/RenderShaderUtils.h"
    "${SRC_ROOT}/Rendering/RenderSkyManager.h"
    "${SRC_ROOT}/Rendering/RenderSkyManager.cpp"
    "${SRC_ROOT}/Rendering/RenderTextureUtils.cpp"
    "${SRC_ROOT}/Rendering/RenderTextureUtils.h"
    "${SRC_ROOT}/Rendering/RenderVoxelChunk.cpp"
    "${SRC_ROOT}/Rendering/RenderVoxelChunk.h"
    "${SRC_ROOT}/Rendering/RenderVoxelChunkManager.cpp"
    "${SRC_ROOT}/Rendering/RenderVoxelChunkManager.h"
    "${SRC_ROOT}/Rendering/RenderWeatherManager.h"
    "${SRC_ROOT}/Rendering/RenderWeatherManager.cpp"
    "${SRC_ROOT}/Rendering/Sdl2DSoft3DRenderBackend.cpp"
    "${SRC_ROOT}/Rendering/Sdl2DSoft3DRenderBackend.h"
    "${SRC_ROOT}/Rendering/SdlUiRenderer.cpp"
    "${SRC_ROOT}/Rendering/SdlUiRenderer.h"
    "${SRC_ROOT}/Rendering/SoftwareRenderer.cpp"
    "${SRC_ROOT}/Rendering/SoftwareRenderer.h"
    "${SRC_ROOT}/Rendering/VisibilityType.h"
    "${SRC_ROOT}/Rendering/VulkanRenderBackend.cpp"
    "${SRC_ROOT}/Rendering/VulkanRenderBackend.h"
    "${SRC_ROOT}/Rendering/Window.cpp"
    "${SRC_ROOT}/Rendering/Window.h")

SET(TES_SKY
    "${SRC_ROOT}/Sky/ArenaSkyUtils.cpp"
    "${SRC_ROOT}/Sky/ArenaSkyUtils.h"
    "${SRC_ROOT}/Sky/SkyAirDefinition.cpp"
    "${SRC_ROOT}/Sky/SkyAirDefinition.h"
    "${SRC_ROOT}/Sky/SkyDefinition.cpp"
    "${SRC_ROOT}/Sky/SkyDefinition.h"
    "${SRC_ROOT}/Sky/SkyGeneration.cpp"
    "${SRC_ROOT}/Sky/SkyGeneration.h"
    "${SRC_ROOT}/Sky/SkyInfoDefinition.cpp"
    "${SRC_ROOT}/Sky/SkyInfoDefinition.h"
    "${SRC_ROOT}/Sky/SkyInstance.cpp"
    "${SRC_ROOT}/Sky/SkyInstance.h"
    "${SRC_ROOT}/Sky/SkyLandDefinition.cpp"
    "${SRC_ROOT}/Sky/SkyLandDefinition.h"
    "${SRC_ROOT}/Sky/SkyLightningDefinition.cpp"
    "${SRC_ROOT}/Sky/SkyLightningDefinition.h"
    "${SRC_ROOT}/Sky/SkyMoonDefinition.cpp"
    "${SRC_ROOT}/Sky/SkyMoonDefinition.h"
    "${SRC_ROOT}/Sky/SkyStarDefinition.cpp"
    "${SRC_ROOT}/Sky/SkyStarDefinition.h"
    "${SRC_ROOT}/Sky/SkySunDefinition.cpp"
    "${SRC_ROOT}/Sky/SkySunDefinition.h"
    "${SRC_ROOT}/Sky/SkyUtils.cpp"
    "${SRC_ROOT}/Sky/SkyUtils.h"
    "${SRC_ROOT}/Sky/SkyVisibilityManager.cpp"
    "${SRC_ROOT}/Sky/SkyVisibilityManager.h")

SET(TES_STATS
    "${SRC_ROOT}/Stats/CharacterClassDefinition.cpp"
    "${SRC_ROOT}/Stats/CharacterClassDefinition.h"
    "${SRC_ROOT}/Stats/CharacterClassLibrary.cpp"
    "${SRC_ROOT}/Stats/CharacterClassLibrary.h"
    "${SRC_ROOT}/Stats/CharacterRaceDefinition.cpp"
    "${SRC_ROOT}/Stats/CharacterRaceDefinition.h"
    "${SRC_ROOT}/Stats/CharacterRaceLibrary.cpp"
    "${SRC_ROOT}/Stats/CharacterRaceLibrary.h"
    "${SRC_ROOT}/Stats/PrimaryAttribute.cpp"
    "${SRC_ROOT}/Stats/PrimaryAttribute.h")

SET(TES_TIME
    "${SRC_ROOT}/Time/ArenaClockUtils.cpp"
    "${SRC_ROOT}/Time/ArenaClockUtils.h"
    "${SRC_ROOT}/Time/ArenaDateUtils.cpp"
    "${SRC_ROOT}/Time/ArenaDateUtils.h"
    "${SRC_ROOT}/Time/Clock.cpp"
    "${SRC_ROOT}/Time/Clock.h"
    "${SRC_ROOT}/Time/ClockLibrary.cpp"
    "${SRC_ROOT}/Time/ClockLibrary.h"
    "${SRC_ROOT}/Time/Date.cpp"
    "${SRC_ROOT}/Time/Date.h")

SET(TES_UI
    "${SRC_ROOT}/UI/AnimationState.cpp"
    "${SRC_ROOT}/UI/AnimationState.h"
    "${SRC_ROOT}/UI/ArenaFontName.h"
    "${SRC_ROOT}/UI/Button.cpp"
    "${SRC_ROOT}/UI/Button.h"
    "${SRC_ROOT}/UI/CursorAlignment.h"
    "${SRC_ROOT}/UI/FontDefinition.cpp"
    "${SRC_ROOT}/UI/FontDefinition.h"
    "${SRC_ROOT}/UI/FontLibrary.cpp"
    "${SRC_ROOT}/UI/FontLibrary.h"
    "${SRC_ROOT}/UI/GuiUtils.cpp"
    "${SRC_ROOT}/UI/GuiUtils.h"
    "${SRC_ROOT}/UI/ListBox.cpp"
    "${SRC_ROOT}/UI/ListBox.h"
    "${SRC_ROOT}/UI/PivotType.h"
    "${SRC_ROOT}/UI/RenderSpace.h"
    "${SRC_ROOT}/UI/ScrollBar.cpp"
    "${SRC_ROOT}/UI/ScrollBar.h"
    "${SRC_ROOT}/UI/Surface.cpp"
    "${SRC_ROOT}/UI/Surface.h"
    "${SRC_ROOT}/UI/TextAlignment.h"
    "${SRC_ROOT}/UI/TextBox.cpp"
    "${SRC_ROOT}/UI/TextBox.h"
    "${SRC_ROOT}/UI/TextEntry.cpp"
    "${SRC_ROOT}/UI/TextEntry.h"
    "${SRC_ROOT}/UI/TextRenderUtils.cpp"
    "${SRC_ROOT}/UI/TextRenderUtils.h"
    "${SRC_ROOT}/UI/ToggleButton.cpp"
    "${SRC_ROOT}/UI/ToggleButton.h"
    "${SRC_ROOT}/UI/UiButton.cpp"
    "${SRC_ROOT}/UI/UiButton.h"
    "${SRC_ROOT}/UI/UiCommand.cpp"
    "${SRC_ROOT}/UI/UiCommand.h"
    "${SRC_ROOT}/UI/UiDrawCall.cpp"
    "${SRC_ROOT}/UI/UiDrawCall.h"
    "${SRC_ROOT}/UI/UiElement.cpp"
    "${SRC_ROOT}/UI/UiElement.h"
    "${SRC_ROOT}/UI/UiImage.cpp"
    "${SRC_ROOT}/UI/UiImage.h"
    "${SRC_ROOT}/UI/UiManager.cpp"
    "${SRC_ROOT}/UI/UiManager.h"
    "${SRC_ROOT}/UI/UiScope.h"
    "${SRC_ROOT}/UI/UiTextBox.cpp"
    "${SRC_ROOT}/UI/UiTextBox.h"
    "${SRC_ROOT}/UI/UiTransform.cpp"
    "${SRC_ROOT}/UI/UiTransform.h")

SET(TES_UTILITIES
    "${SRC_ROOT}/Utilities/Color.cpp"
    "${SRC_ROOT}/Utilities/Color.h"
    "${SRC_ROOT}/Utilities/Endian.h"
    "${SRC_ROOT}/Utilities/Palette.h"
    "${SRC_ROOT}/Utilities/Platform.cpp"
    "${SRC_ROOT}/Utilities/Platform.h"
    "${SRC_ROOT}/Utilities/Timer.cpp"
    "${SRC_ROOT}/Utilities/Timer.h")

SET(TES_VOXELS
    "${SRC_ROOT}/Voxels/ArenaChasmUtils.cpp"
    "${SRC_ROOT}/Voxels/ArenaChasmUtils.h"
    "${SRC_ROOT}/Voxels/ArenaVoxelUtils.cpp"
    "${SRC_ROOT}/Voxels/ArenaVoxelUtils.h"
    "${SRC_ROOT}/Voxels/VoxelBoxCombineChunk.cpp"
    "${SRC_ROOT}/Voxels/VoxelBoxCombineChunk.h"
    "${SRC_ROOT}/Voxels/VoxelBoxCombineChunkManager.cpp"
    "${SRC_ROOT}/Voxels/VoxelBoxCombineChunkManager.h"
    "${SRC_ROOT}/Voxels/VoxelChasmDefinition.cpp"
    "${SRC_ROOT}/Voxels/VoxelChasmDefinition.h"
    "${SRC_ROOT}/Voxels/VoxelChasmWallInstance.cpp"
    "${SRC_ROOT}/Voxels/VoxelChasmWallInstance.h"
    "${SRC_ROOT}/Voxels/VoxelChunk.cpp"
    "${SRC_ROOT}/Voxels/VoxelChunk.h"
    "${SRC_ROOT}/Voxels/VoxelChunkManager.cpp"
    "${SRC_ROOT}/Voxels/VoxelChunkManager.h"
    "${SRC_ROOT}/Voxels/VoxelDirtyType.h"
    "${SRC_ROOT}/Voxels/VoxelDoorAnimationInstance.cpp"
    "${SRC_ROOT}/Voxels/VoxelDoorAnimationInstance.h"
    "${SRC_ROOT}/Voxels/VoxelDoorDefinition.cpp"
    "${SRC_ROOT}/Voxels/VoxelDoorDefinition.h"
    "${SRC_ROOT}/Voxels/VoxelDoorUtils.cpp"
    "${SRC_ROOT}/Voxels/VoxelDoorUtils.h"
    "${SRC_ROOT}/Voxels/VoxelDoorVisibilityInstance.cpp"
    "${SRC_ROOT}/Voxels/VoxelDoorVisibilityInstance.h"
    "${SRC_ROOT}/Voxels/VoxelFaceCombineChunk.h"
    "${SRC_ROOT}/Voxels/VoxelFaceCombineChunk.cpp"
    "${SRC_ROOT}/Voxels/VoxelFaceCombineChunkManager.h"
    "${SRC_ROOT}/Voxels/VoxelFaceCombineChunkManager.cpp"
    "${SRC_ROOT}/Voxels/VoxelFaceEnableChunk.h"
    "${SRC_ROOT}/Voxels/VoxelFaceEnableChunk.cpp"
    "${SRC_ROOT}/Voxels/VoxelFaceEnableChunkManager.h"
    "${SRC_ROOT}/Voxels/VoxelFaceEnableChunkManager.cpp"
    "${SRC_ROOT}/Voxels/VoxelFacing.h"
    "${SRC_ROOT}/Voxels/VoxelFadeAnimationInstance.cpp"
    "${SRC_ROOT}/Voxels/VoxelFadeAnimationInstance.h"
    "${SRC_ROOT}/Voxels/VoxelShadingDefinition.cpp"
    "${SRC_ROOT}/Voxels/VoxelShadingDefinition.h"
    "${SRC_ROOT}/Voxels/VoxelShapeDefinition.cpp"
    "${SRC_ROOT}/Voxels/VoxelShapeDefinition.h"
    "${SRC_ROOT}/Voxels/VoxelTextureDefinition.cpp"
    "${SRC_ROOT}/Voxels/VoxelTextureDefinition.h"
    "${SRC_ROOT}/Voxels/VoxelTraitsDefinition.cpp"
    "${SRC_ROOT}/Voxels/VoxelTraitsDefinition.h"
    "${SRC_ROOT}/Voxels/VoxelTriggerDefinition.cpp"
    "${SRC_ROOT}/Voxels/VoxelTriggerDefinition.h"
    "${SRC_ROOT}/Voxels/VoxelTriggerInstance.cpp"
    "${SRC_ROOT}/Voxels/VoxelTriggerInstance.h"
    "${SRC_ROOT}/Voxels/VoxelUtils.cpp"
    "${SRC_ROOT}/Voxels/VoxelUtils.h"
    "${SRC_ROOT}/Voxels/VoxelFrustumCullingChunk.cpp"
    "${SRC_ROOT}/Voxels/VoxelFrustumCullingChunk.h"
    "${SRC_ROOT}/Voxels/VoxelFrustumCullingChunkManager.cpp"
    "${SRC_ROOT}/Voxels/VoxelFrustumCullingChunkManager.h")

SET(TES_WEATHER
    "${SRC_ROOT}/Weather/ArenaWeatherUtils.cpp"
    "${SRC_ROOT}/Weather/ArenaWeatherUtils.h"
    "${SRC_ROOT}/Weather/WeatherDefinition.cpp"
    "${SRC_ROOT}/Weather/WeatherDefinition.h"
    "${SRC_ROOT}/Weather/WeatherInstance.cpp"
    "${SRC_ROOT}/Weather/WeatherInstance.h"
    "${SRC_ROOT}/Weather/WeatherUtils.cpp"
    "${SRC_ROOT}/Weather/WeatherUtils.h")

SET(TES_WORLD
    "${SRC_ROOT}/World/ArenaCityUtils.cpp"
    "${SRC_ROOT}/World/ArenaCityUtils.h"
    "${SRC_ROOT}/World/ArenaClimateUtils.cpp"
    "${SRC_ROOT}/World/ArenaClimateUtils.h"
    "${SRC_ROOT}/World/ArenaInteriorUtils.cpp"
    "${SRC_ROOT}/World/ArenaInteriorUtils.h"
    "${SRC_ROOT}/World/ArenaLevelUtils.cpp"
    "${SRC_ROOT}/World/ArenaLevelUtils.h"
    "${SRC_ROOT}/World/ArenaMeshUtils.cpp"
    "${SRC_ROOT}/World/ArenaMeshUtils.h"
    "${SRC_ROOT}/World/ArenaWildUtils.cpp"
    "${SRC_ROOT}/World/ArenaWildUtils.h"
    "${SRC_ROOT}/World/CardinalDirection.cpp"
    "${SRC_ROOT}/World/CardinalDirection.h"
    "${SRC_ROOT}/World/CardinalDirectionName.h"
    "${SRC_ROOT}/World/Chunk.cpp"
    "${SRC_ROOT}/World/Chunk.h"
    "${SRC_ROOT}/World/ChunkManager.cpp"
    "${SRC_ROOT}/World/ChunkManager.h"
    "${SRC_ROOT}/World/ChunkUtils.cpp"
    "${SRC_ROOT}/World/ChunkUtils.h"
    "${SRC_ROOT}/World/Coord.cpp"
    "${SRC_ROOT}/World/Coord.h"
    "${SRC_ROOT}/World/LevelDefinition.cpp"
    "${SRC_ROOT}/World/LevelDefinition.h"
    "${SRC_ROOT}/World/LevelInfoDefinition.cpp"
    "${SRC_ROOT}/World/LevelInfoDefinition.h"
    "${SRC_ROOT}/World/LockDefinition.cpp"
    "${SRC_ROOT}/World/LockDefinition.h"
    "${SRC_ROOT}/World/MapDefinition.cpp"
    "${SRC_ROOT}/World/MapDefinition.h"
    "${SRC_ROOT}/World/MapGeneration.cpp"
    "${SRC_ROOT}/World/MapGeneration.h"
    "${SRC_ROOT}/World/MapLogic.cpp"
    "${SRC_ROOT}/World/MapLogic.h"
    "${SRC_ROOT}/World/MapType.h"
    "${SRC_ROOT}/World/MeshLibrary.cpp"
    "${SRC_ROOT}/World/MeshLibrary.h"
    "${SRC_ROOT}/World/MeshUtils.cpp"
    "${SRC_ROOT}/World/MeshUtils.h"
    "${SRC_ROOT}/World/SceneManager.cpp"
    "${SRC_ROOT}/World/SceneManager.h"
    "${SRC_ROOT}/World/SpecializedChunkManager.h"
    "${SRC_ROOT}/World/TransitionDefinition.cpp"
    "${SRC_ROOT}/World/TransitionDefinition.h"
    "${SRC_ROOT}/World/TransitionType.h")

SET(TES_WORLD_MAP
    "${SRC_ROOT}/WorldMap/ArenaLocationUtils.cpp"
    "${SRC_ROOT}/WorldMap/ArenaLocationUtils.h"
    "${SRC_ROOT}/WorldMap/LocationDefinition.cpp"
    "${SRC_ROOT}/WorldMap/LocationDefinition.h"
    "${SRC_ROOT}/WorldMap/LocationInstance.cpp"
    "${SRC_ROOT}/WorldMap/LocationInstance.h"
    "${SRC_ROOT}/WorldMap/ProvinceDefinition.cpp"
    "${SRC_ROOT}/WorldMap/ProvinceDefinition.h"
    "${SRC_ROOT}/WorldMap/ProvinceInstance.cpp"
    "${SRC_ROOT}/WorldMap/ProvinceInstance.h"
    "${SRC_ROOT}/WorldMap/WorldMapDefinition.cpp"
    "${SRC_ROOT}/WorldMap/WorldMapDefinition.h"
    "${SRC_ROOT}/WorldMap/WorldMapInstance.cpp"
    "${SRC_ROOT}/WorldMap/WorldMapInstance.h")

SET(TES_MAIN "${SRC_ROOT}/Main.cpp")

SET(TES_SOURCES 
    ${TES_ASSETS}
    ${TES_AUDIO}
    ${TES_COLLISION}
    ${TES_COMBAT}
    ${TES_ENTITIES}
    ${TES_GAME}
    ${TES_INPUT}
    ${TES_INTERFACE}
    ${TES_ITEMS}
    ${TES_MATH}
    ${TES_PLAYER}
    ${TES_RENDERING}
    ${TES_SKY}
    ${TES_STATS}
    ${TES_TIME}
    ${TES_UI}
    ${TES_UTILITIES}
    ${TES_VOXELS}
    ${TES_WEATHER}
    ${TES_WORLD}
    ${TES_WORLD_MAP}
    ${TES_MAIN})

IF (WIN32)
    SET(TES_WIN32_RESOURCES ${CMAKE_SOURCE_DIR}/windows/opentesarena.rc)
    ADD_DEFINITIONS("-D_SCL_SECURE_NO_WARNINGS=1")
ENDIF()

SET(TES_DATA_FOLDER ${CMAKE_SOURCE_DIR}/data)
SET(TES_OPTIONS_FOLDER ${CMAKE_SOURCE_DIR}/options)

IF (NOT APPLE)
    ADD_EXECUTABLE(otesa ${TES_SOURCES} ${TES_WIN32_RESOURCES})
    SET(TES_EXECUTABLE_RESOURCES_FOLDER $<TARGET_FILE_DIR:otesa>)
ELSEIF (APPLE)
    SET(MACOSX_BUNDLE_LONG_VERSION_STRING ${OpenTESArena_VERSION})
    SET(MACOSX_BUNDLE_SHORT_VERSION_STRING ${OpenTESArena_VERSION})

    SET(TES_MAC_ICON ${CMAKE_SOURCE_DIR}/macOS/OpenTESArena.icns)
    SET_SOURCE_FILES_PROPERTIES(${TES_MAC_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    SET(MACOSX_BUNDLE_ICON_FILE OpenTESArena.icns)
    
    ADD_EXECUTABLE(otesa MACOSX_BUNDLE ${TES_SOURCES} ${TES_MAC_ICON})
    SET(TES_EXECUTABLE_RESOURCES_FOLDER $<TARGET_BUNDLE_CONTENT_DIR:otesa>/Resources)
ENDIF()

TARGET_INCLUDE_DIRECTORIES(otesa PUBLIC "${JoltPhysics_SOURCE_DIR}/..")
TARGET_LINK_LIBRARIES(otesa Jolt components ${EXTERNAL_LIBS})
SET_TARGET_PROPERTIES(otesa PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OpenTESArena_BINARY_DIR})

ADD_CUSTOM_COMMAND(TARGET otesa POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${TES_DATA_FOLDER} ${TES_EXECUTABLE_RESOURCES_FOLDER}/data
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${TES_OPTIONS_FOLDER} ${TES_EXECUTABLE_RESOURCES_FOLDER}/options)

IF (WIN32)
    # @todo this doesn't seem to work because SDL2/OpenAL Soft/WildMIDI are not 'imported' libraries with a defined .dll
    #ADD_CUSTOM_COMMAND(TARGET otesa POST_BUILD
    #    COMMAND ${CMAKE_COMMAND} -E copy -t ${TES_EXECUTABLE_RESOURCES_FOLDER} $<TARGET_RUNTIME_DLLS:otesa>
    #    COMMAND_EXPAND_LISTS)
ELSEIF (APPLE)
    SET(TES_DYLIB_DESTINATION_FOLDER $<TARGET_BUNDLE_CONTENT_DIR:otesa>/Frameworks)
    ADD_CUSTOM_COMMAND(TARGET otesa POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${TES_DYLIB_DESTINATION_FOLDER})
    ADD_CUSTOM_COMMAND(TARGET otesa POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy /opt/homebrew/opt/sdl2/lib/libSDL2.dylib ${TES_DYLIB_DESTINATION_FOLDER})
    ADD_CUSTOM_COMMAND(TARGET otesa POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy /opt/homebrew/opt/openal-soft/lib/libopenal.dylib ${TES_DYLIB_DESTINATION_FOLDER})
    
    IF (WILDMIDI_FOUND)
        ADD_CUSTOM_COMMAND(TARGET otesa POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy /opt/homebrew/opt/wildmidi/lib/libWildMidi.dylib ${TES_DYLIB_DESTINATION_FOLDER})
    ENDIF()
	
    IF (VULKAN_FOUND)
        GET_FILENAME_COMPONENT(VULKAN_LIBRARY_DIR "${Vulkan_LIBRARY}" DIRECTORY)
        ADD_CUSTOM_COMMAND(TARGET otesa POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "${VULKAN_LIBRARY_DIR}/libvulkan.dylib" ${TES_DYLIB_DESTINATION_FOLDER})
        ADD_CUSTOM_COMMAND(TARGET otesa POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "${VULKAN_LIBRARY_DIR}/libMoltenVK.dylib" ${TES_DYLIB_DESTINATION_FOLDER})
    ENDIF()

    ADD_CUSTOM_COMMAND(TARGET otesa POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E env TES_APP_BUNDLE_PATH="$<TARGET_BUNDLE_DIR:otesa>" bash ${CMAKE_SOURCE_DIR}/macOS/fix_dylibs.sh)
ENDIF()

# Visual Studio filters.
SOURCE_GROUP(TREE ${CMAKE_SOURCE_DIR}/OpenTESArena FILES ${TES_SOURCES})

IF (MSVC)
	SET_TARGET_PROPERTIES(otesa PROPERTIES VS_DPI_AWARE "PerMonitor") # DPI awareness
    SET_PROPERTY(DIRECTORY PROPERTY VS_STARTUP_PROJECT "otesa") # Default startup project
ENDIF()
