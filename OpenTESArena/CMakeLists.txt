PROJECT(TESArena CXX)

SET(OpenTESArena_VERSION "0.13.0")

FIND_PACKAGE(SDL2 REQUIRED)
FIND_PACKAGE(OpenAL REQUIRED)
FIND_PACKAGE(WildMidi)

SET(EXTERNAL_LIBS ${OPENAL_LIBRARY} ${SDL2_LIBRARY})
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}" ${SDL2_INCLUDE_DIR} ${OPENAL_INCLUDE_DIR})
IF(WILDMIDI_FOUND)
    SET(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${WILDMIDI_LIBRARIES})
    INCLUDE_DIRECTORIES(${WILDMIDI_INCLUDE_DIRS})
    ADD_DEFINITIONS("-DHAVE_WILDMIDI=1")
ELSE(WILDMIDI_FOUND)
    MESSAGE(STATUS "WildMIDI not found, no MIDI support!")
ENDIF(WILDMIDI_FOUND)

SET(SRC_ROOT ${TESArena_SOURCE_DIR})

FILE(GLOB TES_ASSETS
    ${SRC_ROOT}/src/Assets/*.h* 
    ${SRC_ROOT}/src/Assets/*.c*)

FILE(GLOB TES_AUDIO
    ${SRC_ROOT}/src/Audio/*.h* 
    ${SRC_ROOT}/src/Audio/*.c*)

FILE(GLOB TES_ENTITIES
    ${SRC_ROOT}/src/Entities/*.h* 
    ${SRC_ROOT}/src/Entities/*.c*)

FILE(GLOB TES_GAME
    ${SRC_ROOT}/src/Game/*.h* 
    ${SRC_ROOT}/src/Game/*.c*)

FILE(GLOB TES_INPUT
    ${SRC_ROOT}/src/Input/*.h* 
    ${SRC_ROOT}/src/Input/*.c*)

FILE(GLOB TES_INTERFACE
    ${SRC_ROOT}/src/Interface/*.h* 
    ${SRC_ROOT}/src/Interface/*.c*)

FILE(GLOB TES_ITEMS
    ${SRC_ROOT}/src/Items/*.h* 
    ${SRC_ROOT}/src/Items/*.c*)

FILE(GLOB TES_MATH
    ${SRC_ROOT}/src/Math/*.h* 
    ${SRC_ROOT}/src/Math/*.c*)

FILE(GLOB TES_MEDIA
    ${SRC_ROOT}/src/Media/*.h* 
    ${SRC_ROOT}/src/Media/*.c*)

FILE(GLOB TES_RENDERING
    ${SRC_ROOT}/src/Rendering/*.h* 
    ${SRC_ROOT}/src/Rendering/*.c*)

FILE(GLOB TES_UI
    ${SRC_ROOT}/src/UI/*.h* 
    ${SRC_ROOT}/src/UI/*.c*)

FILE(GLOB TES_UTILITIES
    ${SRC_ROOT}/src/Utilities/*.h* 
    ${SRC_ROOT}/src/Utilities/*.c*)

FILE(GLOB TES_WORLD
    ${SRC_ROOT}/src/World/*.h* 
    ${SRC_ROOT}/src/World/*.c*)

FILE(GLOB TES_WORLD_MAP
    ${SRC_ROOT}/src/WorldMap/*.h* 
    ${SRC_ROOT}/src/WorldMap/*.c*)

SET(TES_MAIN ${SRC_ROOT}/src/Main.cpp)

SET(TES_SOURCES 
    ${TES_ASSETS}
    ${TES_AUDIO}
    ${TES_ENTITIES}
    ${TES_GAME}
    ${TES_INPUT}
    ${TES_INTERFACE}
    ${TES_ITEMS}
    ${TES_MATH}
    ${TES_MEDIA}
    ${TES_RENDERING}
    ${TES_UI}
    ${TES_UTILITIES}
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
    # Copy over required files
    FILE(COPY ${TES_DATA_FOLDER} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    FILE(COPY ${TES_OPTIONS_FOLDER} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

    # Add the rest
    ADD_EXECUTABLE(TESArena ${TES_SOURCES} ${TES_WIN32_RESOURCES})
ELSE (APPLE)
    # Info.plist properties
    SET(MACOSX_BUNDLE_LONG_VERSION_STRING ${OpenTESArena_VERSION})
    SET(MACOSX_BUNDLE_SHORT_VERSION_STRING ${OpenTESArena_VERSION})

    # Icon
    ## Find the file in source code
    SET(TES_MAC_ICON ${CMAKE_SOURCE_DIR}/macOS/OpenTESArena.icns)
    ## Set the destination location to the standard .app location
    SET_SOURCE_FILES_PROPERTIES(${TES_MAC_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    ## Write the filename to the Info.plist file
    SET(MACOSX_BUNDLE_ICON_FILE OpenTESArena.icns)

    # Copy over required files
    # Using direct copy workaround because SET_SOURCE_FILES_PROPERTIES doesn't work recursively due to bug in CMake.
    # See https://gitlab.kitware.com/cmake/cmake/issues/16886
    FILE(COPY ${TES_DATA_FOLDER} DESTINATION ../TESArena.app/Contents/Resources)
    FILE(COPY ${TES_OPTIONS_FOLDER} DESTINATION ../TESArena.app/Contents/Resources)

    # Add the rest
    ADD_EXECUTABLE(TESArena MACOSX_BUNDLE ${TES_SOURCES} ${TES_MAC_ICON})
ENDIF()

TARGET_LINK_LIBRARIES(TESArena components ${EXTERNAL_LIBS})
SET_TARGET_PROPERTIES(TESArena PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OpenTESArena_BINARY_DIR})

# Visual Studio filters.
SOURCE_GROUP(TREE ${CMAKE_SOURCE_DIR}/OpenTESArena FILES ${TES_SOURCES})
